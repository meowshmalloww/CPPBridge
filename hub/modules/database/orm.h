#pragma once
// =============================================================================
// ORM.H - SQLite Object-Relational Mapper
// =============================================================================
// Complete ORM with:
// - Query builder (fluent API)
// - Model mapping
// - Schema migrations
// - Transactions
// - Prepared statements
// =============================================================================

#include "../database/database.h"
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

namespace Hub::ORM {

// =============================================================================
// COLUMN VALUE TYPES
// =============================================================================
using Value = std::variant<std::monostate, int64_t, double, std::string,
                           std::vector<uint8_t>>;

inline std::string valueToSql(const Value &v) {
  if (std::holds_alternative<std::monostate>(v))
    return "NULL";
  if (std::holds_alternative<int64_t>(v))
    return std::to_string(std::get<int64_t>(v));
  if (std::holds_alternative<double>(v))
    return std::to_string(std::get<double>(v));
  if (std::holds_alternative<std::string>(v)) {
    std::string s = std::get<std::string>(v);
    // Escape quotes
    size_t pos = 0;
    while ((pos = s.find('\'', pos)) != std::string::npos) {
      s.replace(pos, 1, "''");
      pos += 2;
    }
    return "'" + s + "'";
  }
  return "NULL";
}

// =============================================================================
// ROW RESULT
// =============================================================================
class Row {
public:
  void set(const std::string &col, const Value &val) { columns_[col] = val; }

  Value get(const std::string &col) const {
    auto it = columns_.find(col);
    if (it == columns_.end())
      return std::monostate{};
    return it->second;
  }

  int64_t getInt(const std::string &col, int64_t def = 0) const {
    auto v = get(col);
    if (std::holds_alternative<int64_t>(v))
      return std::get<int64_t>(v);
    return def;
  }

  double getDouble(const std::string &col, double def = 0.0) const {
    auto v = get(col);
    if (std::holds_alternative<double>(v))
      return std::get<double>(v);
    if (std::holds_alternative<int64_t>(v))
      return static_cast<double>(std::get<int64_t>(v));
    return def;
  }

  std::string getString(const std::string &col,
                        const std::string &def = "") const {
    auto v = get(col);
    if (std::holds_alternative<std::string>(v))
      return std::get<std::string>(v);
    return def;
  }

  bool has(const std::string &col) const {
    return columns_.find(col) != columns_.end();
  }

private:
  std::map<std::string, Value> columns_;
};

// =============================================================================
// QUERY BUILDER
// =============================================================================
class QueryBuilder {
public:
  QueryBuilder() = default;

  // SELECT
  QueryBuilder &select(const std::string &cols = "*") {
    type_ = "SELECT";
    columns_ = cols;
    return *this;
  }

  // FROM
  QueryBuilder &from(const std::string &table) {
    table_ = table;
    return *this;
  }

  // INSERT
  QueryBuilder &insert(const std::string &table) {
    type_ = "INSERT";
    table_ = table;
    return *this;
  }

  // UPDATE
  QueryBuilder &update(const std::string &table) {
    type_ = "UPDATE";
    table_ = table;
    return *this;
  }

  // DELETE
  QueryBuilder &deleteFrom(const std::string &table) {
    type_ = "DELETE";
    table_ = table;
    return *this;
  }

  // SET (for INSERT/UPDATE)
  QueryBuilder &set(const std::string &col, const Value &val) {
    values_[col] = val;
    return *this;
  }

  // WHERE
  QueryBuilder &where(const std::string &col, const std::string &op,
                      const Value &val) {
    if (!where_.empty())
      where_ += " AND ";
    where_ += col + " " + op + " " + valueToSql(val);
    return *this;
  }

  QueryBuilder &whereEq(const std::string &col, const Value &val) {
    return where(col, "=", val);
  }

  QueryBuilder &whereLike(const std::string &col, const std::string &pattern) {
    return where(col, "LIKE", pattern);
  }

  QueryBuilder &whereIn(const std::string &col,
                        const std::vector<Value> &vals) {
    std::string list;
    for (const auto &v : vals) {
      if (!list.empty())
        list += ", ";
      list += valueToSql(v);
    }
    if (!where_.empty())
      where_ += " AND ";
    where_ += col + " IN (" + list + ")";
    return *this;
  }

  QueryBuilder &whereNull(const std::string &col) {
    if (!where_.empty())
      where_ += " AND ";
    where_ += col + " IS NULL";
    return *this;
  }

  QueryBuilder &whereNotNull(const std::string &col) {
    if (!where_.empty())
      where_ += " AND ";
    where_ += col + " IS NOT NULL";
    return *this;
  }

  QueryBuilder &orWhere(const std::string &col, const std::string &op,
                        const Value &val) {
    if (!where_.empty())
      where_ += " OR ";
    where_ += col + " " + op + " " + valueToSql(val);
    return *this;
  }

  // ORDER BY
  QueryBuilder &orderBy(const std::string &col, bool ascending = true) {
    if (!orderBy_.empty())
      orderBy_ += ", ";
    orderBy_ += col + (ascending ? " ASC" : " DESC");
    return *this;
  }

  // LIMIT
  QueryBuilder &limit(int count) {
    limit_ = count;
    return *this;
  }

  // OFFSET
  QueryBuilder &offset(int count) {
    offset_ = count;
    return *this;
  }

  // JOIN
  QueryBuilder &join(const std::string &table, const std::string &on) {
    joins_ += " INNER JOIN " + table + " ON " + on;
    return *this;
  }

  QueryBuilder &leftJoin(const std::string &table, const std::string &on) {
    joins_ += " LEFT JOIN " + table + " ON " + on;
    return *this;
  }

  // GROUP BY
  QueryBuilder &groupBy(const std::string &cols) {
    groupBy_ = cols;
    return *this;
  }

  // HAVING
  QueryBuilder &having(const std::string &condition) {
    having_ = condition;
    return *this;
  }

  // Build SQL string
  std::string toSql() const {
    std::ostringstream sql;

    if (type_ == "SELECT") {
      sql << "SELECT " << columns_ << " FROM " << table_;
      sql << joins_;
      if (!where_.empty())
        sql << " WHERE " << where_;
      if (!groupBy_.empty())
        sql << " GROUP BY " << groupBy_;
      if (!having_.empty())
        sql << " HAVING " << having_;
      if (!orderBy_.empty())
        sql << " ORDER BY " << orderBy_;
      if (limit_ > 0)
        sql << " LIMIT " << limit_;
      if (offset_ > 0)
        sql << " OFFSET " << offset_;
    } else if (type_ == "INSERT") {
      std::string cols, vals;
      for (const auto &[k, v] : values_) {
        if (!cols.empty()) {
          cols += ", ";
          vals += ", ";
        }
        cols += k;
        vals += valueToSql(v);
      }
      sql << "INSERT INTO " << table_ << " (" << cols << ") VALUES (" << vals
          << ")";
    } else if (type_ == "UPDATE") {
      sql << "UPDATE " << table_ << " SET ";
      bool first = true;
      for (const auto &[k, v] : values_) {
        if (!first)
          sql << ", ";
        sql << k << " = " << valueToSql(v);
        first = false;
      }
      if (!where_.empty())
        sql << " WHERE " << where_;
    } else if (type_ == "DELETE") {
      sql << "DELETE FROM " << table_;
      if (!where_.empty())
        sql << " WHERE " << where_;
    }

    return sql.str();
  }

  // Reset builder
  void clear() {
    type_.clear();
    table_.clear();
    columns_ = "*";
    values_.clear();
    where_.clear();
    orderBy_.clear();
    groupBy_.clear();
    having_.clear();
    joins_.clear();
    limit_ = 0;
    offset_ = 0;
  }

private:
  std::string type_;
  std::string table_;
  std::string columns_ = "*";
  std::map<std::string, Value> values_;
  std::string where_;
  std::string orderBy_;
  std::string groupBy_;
  std::string having_;
  std::string joins_;
  int limit_ = 0;
  int offset_ = 0;
};

// =============================================================================
// SCHEMA BUILDER (MIGRATIONS)
// =============================================================================
class Column {
public:
  Column(const std::string &name, const std::string &type)
      : name_(name), type_(type) {}

  Column &primaryKey() {
    primaryKey_ = true;
    return *this;
  }
  Column &autoIncrement() {
    autoIncrement_ = true;
    return *this;
  }
  Column &notNull() {
    notNull_ = true;
    return *this;
  }
  Column &unique() {
    unique_ = true;
    return *this;
  }
  Column &defaultValue(const std::string &val) {
    default_ = val;
    return *this;
  }
  Column &references(const std::string &table, const std::string &col) {
    fkTable_ = table;
    fkCol_ = col;
    return *this;
  }

  std::string toSql() const {
    std::string sql = name_ + " " + type_;
    if (primaryKey_)
      sql += " PRIMARY KEY";
    if (autoIncrement_)
      sql += " AUTOINCREMENT";
    if (notNull_)
      sql += " NOT NULL";
    if (unique_)
      sql += " UNIQUE";
    if (!default_.empty())
      sql += " DEFAULT " + default_;
    return sql;
  }

  std::string foreignKey() const {
    if (fkTable_.empty())
      return "";
    return "FOREIGN KEY (" + name_ + ") REFERENCES " + fkTable_ + "(" + fkCol_ +
           ")";
  }

private:
  std::string name_;
  std::string type_;
  bool primaryKey_ = false;
  bool autoIncrement_ = false;
  bool notNull_ = false;
  bool unique_ = false;
  std::string default_;
  std::string fkTable_;
  std::string fkCol_;
};

class Schema {
public:
  Schema &id() {
    columns_.emplace_back("id", "INTEGER");
    columns_.back().primaryKey().autoIncrement();
    return *this;
  }

  Column &integer(const std::string &name) {
    columns_.emplace_back(name, "INTEGER");
    return columns_.back();
  }

  Column &text(const std::string &name) {
    columns_.emplace_back(name, "TEXT");
    return columns_.back();
  }

  Column &real(const std::string &name) {
    columns_.emplace_back(name, "REAL");
    return columns_.back();
  }

  Column &blob(const std::string &name) {
    columns_.emplace_back(name, "BLOB");
    return columns_.back();
  }

  Schema &timestamps() {
    columns_.emplace_back("created_at", "TEXT");
    columns_.back().defaultValue("CURRENT_TIMESTAMP");
    columns_.emplace_back("updated_at", "TEXT");
    columns_.back().defaultValue("CURRENT_TIMESTAMP");
    return *this;
  }

  std::string toCreateSql(const std::string &table) const {
    std::string sql = "CREATE TABLE IF NOT EXISTS " + table + " (";
    std::vector<std::string> fks;

    for (size_t i = 0; i < columns_.size(); ++i) {
      if (i > 0)
        sql += ", ";
      sql += columns_[i].toSql();
      std::string fk = columns_[i].foreignKey();
      if (!fk.empty())
        fks.push_back(fk);
    }

    for (const auto &fk : fks) {
      sql += ", " + fk;
    }

    sql += ")";
    return sql;
  }

private:
  std::vector<Column> columns_;
};

// =============================================================================
// DATABASE CONNECTION
// =============================================================================
class Database {
public:
  Database() = default;
  ~Database() { close(); }

  bool open(const std::string &path) {
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
      return false;
    }
    path_ = path;
    return true;
  }

  void close() {
    if (db_) {
      sqlite3_close(db_);
      db_ = nullptr;
    }
  }

  bool isOpen() const { return db_ != nullptr; }

  // Execute raw SQL
  bool execute(const std::string &sql) {
    char *errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (errMsg) {
      lastError_ = errMsg;
      sqlite3_free(errMsg);
    }
    return rc == SQLITE_OK;
  }

  // Query with results
  std::vector<Row> query(const std::string &sql) {
    std::vector<Row> results;

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
      return results;
    }

    int colCount = sqlite3_column_count(stmt);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
      Row row;
      for (int i = 0; i < colCount; ++i) {
        std::string colName = sqlite3_column_name(stmt, i);
        int type = sqlite3_column_type(stmt, i);

        switch (type) {
        case SQLITE_INTEGER:
          row.set(colName, static_cast<int64_t>(sqlite3_column_int64(stmt, i)));
          break;
        case SQLITE_FLOAT:
          row.set(colName, sqlite3_column_double(stmt, i));
          break;
        case SQLITE_TEXT:
          row.set(colName, std::string(reinterpret_cast<const char *>(
                               sqlite3_column_text(stmt, i))));
          break;
        case SQLITE_BLOB: {
          const uint8_t *data =
              static_cast<const uint8_t *>(sqlite3_column_blob(stmt, i));
          int size = sqlite3_column_bytes(stmt, i);
          row.set(colName, std::vector<uint8_t>(data, data + size));
          break;
        }
        default:
          row.set(colName, std::monostate{});
        }
      }
      results.push_back(std::move(row));
    }

    sqlite3_finalize(stmt);
    return results;
  }

  // Query with builder
  std::vector<Row> query(const QueryBuilder &qb) { return query(qb.toSql()); }

  // Execute with builder
  bool execute(const QueryBuilder &qb) { return execute(qb.toSql()); }

  // Get first result
  std::optional<Row> first(const QueryBuilder &qb) {
    auto results = query(qb.toSql());
    if (results.empty())
      return std::nullopt;
    return results[0];
  }

  // Count
  int64_t count(const std::string &table, const std::string &where = "") {
    std::string sql = "SELECT COUNT(*) as cnt FROM " + table;
    if (!where.empty())
      sql += " WHERE " + where;
    auto results = query(sql);
    if (results.empty())
      return 0;
    return results[0].getInt("cnt");
  }

  // Last insert ID
  int64_t lastInsertId() { return sqlite3_last_insert_rowid(db_); }

  // Create table
  bool createTable(const std::string &name,
                   std::function<void(Schema &)> builder) {
    Schema schema;
    builder(schema);
    return execute(schema.toCreateSql(name));
  }

  // Drop table
  bool dropTable(const std::string &name) {
    return execute("DROP TABLE IF EXISTS " + name);
  }

  // Transactions
  bool beginTransaction() { return execute("BEGIN TRANSACTION"); }
  bool commit() { return execute("COMMIT"); }
  bool rollback() { return execute("ROLLBACK"); }

  template <typename F> bool transaction(F &&func) {
    if (!beginTransaction())
      return false;
    try {
      if (func()) {
        return commit();
      } else {
        rollback();
        return false;
      }
    } catch (...) {
      rollback();
      return false;
    }
  }

  std::string lastError() const { return lastError_; }

private:
  sqlite3 *db_ = nullptr;
  std::string path_;
  std::string lastError_;
};

// =============================================================================
// GLOBAL DATABASE
// =============================================================================
inline Database &globalDB() {
  static Database db;
  return db;
}

} // namespace Hub::ORM

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
// Database
__declspec(dllexport) int hub_orm_open(const char *path);
__declspec(dllexport) void hub_orm_close();
__declspec(dllexport) int hub_orm_execute(const char *sql);
__declspec(dllexport) const char *hub_orm_query_json(const char *sql);
__declspec(dllexport) long long hub_orm_last_insert_id();
__declspec(dllexport) long long hub_orm_count(const char *table,
                                              const char *where);

// Transactions
__declspec(dllexport) int hub_orm_begin();
__declspec(dllexport) int hub_orm_commit();
__declspec(dllexport) int hub_orm_rollback();

// Schema
__declspec(dllexport) int hub_orm_create_table(const char *name,
                                               const char *schema_json);
__declspec(dllexport) int hub_orm_drop_table(const char *name);
}
