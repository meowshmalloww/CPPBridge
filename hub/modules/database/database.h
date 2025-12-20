#pragma once
// =============================================================================
// DATABASE.H - SQLite Database Module
// =============================================================================
// Simple SQLite wrapper with query execution, prepared statements,
// and transaction support.
// =============================================================================

#include <atomic>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>


// SQLite header (compiled as C)
extern "C" {
#include "sqlite3.h"
}

namespace Hub::Database {

// =============================================================================
// QUERY RESULT TYPES
// =============================================================================
using Row = std::vector<std::string>;
using ColumnNames = std::vector<std::string>;

struct QueryResult {
  bool success = false;
  std::string error;
  ColumnNames columns;
  std::vector<Row> rows;
  int affected_rows = 0;
  int64_t last_insert_id = 0;

  bool is_ok() const { return success; }
  size_t row_count() const { return rows.size(); }
  size_t column_count() const { return columns.size(); }
};

// =============================================================================
// DATABASE CONNECTION
// =============================================================================
class Database {
public:
  Database() = default;
  ~Database() { close(); }

  // Open database (creates if doesn't exist)
  bool open(const std::string &path) {
    if (db_)
      close();

    std::cout << "[database] Opening: " << path << "\n";

    int rc = sqlite3_open(path.c_str(), &db_);
    if (rc != SQLITE_OK) {
      last_error_ = sqlite3_errmsg(db_);
      sqlite3_close(db_);
      db_ = nullptr;
      return false;
    }

    path_ = path;
    std::cout << "[database] Opened successfully\n";
    return true;
  }

  // Open in-memory database
  bool openMemory() { return open(":memory:"); }

  void close() {
    if (db_) {
      sqlite3_close(db_);
      db_ = nullptr;
      path_.clear();
    }
  }

  // Execute SQL query
  QueryResult execute(const std::string &sql) {
    QueryResult result;

    if (!db_) {
      result.error = "Database not open";
      return result;
    }

    char *errMsg = nullptr;
    std::vector<std::string> *colNamesPtr = &result.columns;
    std::vector<Row> *rowsPtr = &result.rows;

    int rc = sqlite3_exec(
        db_, sql.c_str(),
        [](void *data, int argc, char **argv, char **colNames) -> int {
          auto *params = static_cast<
              std::pair<std::vector<std::string> *, std::vector<Row> *> *>(
              data);

          // Capture column names on first row
          if (params->first->empty()) {
            for (int i = 0; i < argc; i++) {
              params->first->push_back(colNames[i] ? colNames[i] : "");
            }
          }

          // Capture row data
          Row row;
          for (int i = 0; i < argc; i++) {
            row.push_back(argv[i] ? argv[i] : "");
          }
          params->second->push_back(row);

          return 0;
        },
        new std::pair<std::vector<std::string> *, std::vector<Row> *>(
            colNamesPtr, rowsPtr),
        &errMsg);

    if (rc != SQLITE_OK) {
      result.error = errMsg ? errMsg : "Unknown error";
      sqlite3_free(errMsg);
      return result;
    }

    result.success = true;
    result.affected_rows = sqlite3_changes(db_);
    result.last_insert_id = sqlite3_last_insert_rowid(db_);

    return result;
  }

  // Execute query with parameter binding (prevents SQL injection)
  QueryResult executeParams(const std::string &sql,
                            const std::vector<std::string> &params) {
    QueryResult result;

    if (!db_) {
      result.error = "Database not open";
      return result;
    }

    sqlite3_stmt *stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
      result.error = sqlite3_errmsg(db_);
      return result;
    }

    // Bind parameters
    for (size_t i = 0; i < params.size(); i++) {
      sqlite3_bind_text(stmt, static_cast<int>(i + 1), params[i].c_str(), -1,
                        SQLITE_TRANSIENT);
    }

    // Execute and collect results
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
      // Get column names on first row
      if (result.columns.empty()) {
        int colCount = sqlite3_column_count(stmt);
        for (int i = 0; i < colCount; i++) {
          const char *name = sqlite3_column_name(stmt, i);
          result.columns.push_back(name ? name : "");
        }
      }

      // Get row data
      Row row;
      int colCount = sqlite3_column_count(stmt);
      for (int i = 0; i < colCount; i++) {
        const unsigned char *text = sqlite3_column_text(stmt, i);
        row.push_back(text ? reinterpret_cast<const char *>(text) : "");
      }
      result.rows.push_back(row);
    }

    if (rc != SQLITE_DONE) {
      result.error = sqlite3_errmsg(db_);
      sqlite3_finalize(stmt);
      return result;
    }

    sqlite3_finalize(stmt);
    result.success = true;
    result.affected_rows = sqlite3_changes(db_);
    result.last_insert_id = sqlite3_last_insert_rowid(db_);

    return result;
  }

  // Transaction helpers
  bool beginTransaction() { return execute("BEGIN TRANSACTION").is_ok(); }

  bool commit() { return execute("COMMIT").is_ok(); }

  bool rollback() { return execute("ROLLBACK").is_ok(); }

  // Table exists check
  bool tableExists(const std::string &tableName) {
    auto result = executeParams(
        "SELECT name FROM sqlite_master WHERE type='table' AND name=?",
        {tableName});
    return result.is_ok() && result.row_count() > 0;
  }

  bool isOpen() const { return db_ != nullptr; }
  const std::string &getLastError() const { return last_error_; }
  const std::string &getPath() const { return path_; }

private:
  sqlite3 *db_ = nullptr;
  std::string path_;
  std::string last_error_;
};

// =============================================================================
// DATABASE STORAGE
// =============================================================================
inline std::mutex dbMutex;
inline std::map<int, std::unique_ptr<Database>> databases;
inline std::atomic<int> dbNextHandle{1};

inline int createDatabase() {
  std::lock_guard<std::mutex> lock(dbMutex);
  int handle = dbNextHandle++;
  databases[handle] = std::make_unique<Database>();
  return handle;
}

inline Database *getDatabase(int handle) {
  std::lock_guard<std::mutex> lock(dbMutex);
  auto it = databases.find(handle);
  return (it != databases.end()) ? it->second.get() : nullptr;
}

inline void releaseDatabase(int handle) {
  std::lock_guard<std::mutex> lock(dbMutex);
  databases.erase(handle);
}

// Query result storage for FFI
inline std::mutex resultMutex;
inline std::map<int, QueryResult> queryResults;
inline std::atomic<int> resultNextHandle{1};

inline int storeResult(QueryResult &&result) {
  std::lock_guard<std::mutex> lock(resultMutex);
  int handle = resultNextHandle++;
  queryResults[handle] = std::move(result);
  return handle;
}

inline QueryResult *getResult(int handle) {
  std::lock_guard<std::mutex> lock(resultMutex);
  auto it = queryResults.find(handle);
  return (it != queryResults.end()) ? &it->second : nullptr;
}

inline void releaseResult(int handle) {
  std::lock_guard<std::mutex> lock(resultMutex);
  queryResults.erase(handle);
}

} // namespace Hub::Database

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
__declspec(dllexport) int hub_db_create();
__declspec(dllexport) int hub_db_open(int handle, const char *path);
__declspec(dllexport) int hub_db_open_memory(int handle);
__declspec(dllexport) int hub_db_execute(int handle, const char *sql);
__declspec(dllexport) int hub_db_is_open(int handle);
__declspec(dllexport) void hub_db_close(int handle);

// Result access
__declspec(dllexport) int hub_db_result_ok(int result_handle);
__declspec(dllexport) int hub_db_result_row_count(int result_handle);
__declspec(dllexport) int hub_db_result_col_count(int result_handle);
__declspec(dllexport) const char *hub_db_result_get(int result_handle, int row,
                                                    int col);
__declspec(dllexport) const char *hub_db_result_error(int result_handle);
__declspec(dllexport) void hub_db_result_release(int result_handle);

__declspec(dllexport) const char *hub_db_get_error(int handle);
}
