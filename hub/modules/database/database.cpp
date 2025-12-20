// =============================================================================
// DATABASE.CPP - SQLite Database Implementation
// =============================================================================

#include "database.h"
#include <cstring>

// Thread-local error storage
static thread_local std::string tl_last_error;
static thread_local std::string tl_cell_value;

extern "C" {

__declspec(dllexport) int hub_db_create() {
  return Hub::Database::createDatabase();
}

__declspec(dllexport) int hub_db_open(int handle, const char *path) {
  if (!path) {
    tl_last_error = "Path is null";
    return -1;
  }

  auto *db = Hub::Database::getDatabase(handle);
  if (!db) {
    tl_last_error = "Invalid handle";
    return -1;
  }

  if (db->open(path)) {
    return 0;
  }

  tl_last_error = db->getLastError();
  return -1;
}

__declspec(dllexport) int hub_db_open_memory(int handle) {
  auto *db = Hub::Database::getDatabase(handle);
  if (!db) {
    tl_last_error = "Invalid handle";
    return -1;
  }

  if (db->openMemory()) {
    return 0;
  }

  tl_last_error = db->getLastError();
  return -1;
}

__declspec(dllexport) int hub_db_execute(int handle, const char *sql) {
  if (!sql) {
    tl_last_error = "SQL is null";
    return -1;
  }

  auto *db = Hub::Database::getDatabase(handle);
  if (!db) {
    tl_last_error = "Invalid handle";
    return -1;
  }

  auto result = db->execute(sql);
  return Hub::Database::storeResult(std::move(result));
}

__declspec(dllexport) int hub_db_is_open(int handle) {
  auto *db = Hub::Database::getDatabase(handle);
  return (db && db->isOpen()) ? 1 : 0;
}

__declspec(dllexport) void hub_db_close(int handle) {
  auto *db = Hub::Database::getDatabase(handle);
  if (db) {
    db->close();
  }
  Hub::Database::releaseDatabase(handle);
}

__declspec(dllexport) int hub_db_result_ok(int result_handle) {
  auto *result = Hub::Database::getResult(result_handle);
  return (result && result->is_ok()) ? 1 : 0;
}

__declspec(dllexport) int hub_db_result_row_count(int result_handle) {
  auto *result = Hub::Database::getResult(result_handle);
  return result ? static_cast<int>(result->row_count()) : 0;
}

__declspec(dllexport) int hub_db_result_col_count(int result_handle) {
  auto *result = Hub::Database::getResult(result_handle);
  return result ? static_cast<int>(result->column_count()) : 0;
}

__declspec(dllexport) const char *hub_db_result_get(int result_handle, int row,
                                                    int col) {
  auto *result = Hub::Database::getResult(result_handle);
  if (!result || row < 0 || col < 0)
    return "";

  if (static_cast<size_t>(row) >= result->rows.size())
    return "";
  if (static_cast<size_t>(col) >= result->rows[row].size())
    return "";

  tl_cell_value = result->rows[row][col];
  return tl_cell_value.c_str();
}

__declspec(dllexport) const char *hub_db_result_error(int result_handle) {
  auto *result = Hub::Database::getResult(result_handle);
  if (result) {
    tl_last_error = result->error;
  }
  return tl_last_error.c_str();
}

__declspec(dllexport) void hub_db_result_release(int result_handle) {
  Hub::Database::releaseResult(result_handle);
}

__declspec(dllexport) const char *hub_db_get_error(int handle) {
  auto *db = Hub::Database::getDatabase(handle);
  if (db) {
    tl_last_error = db->getLastError();
  }
  return tl_last_error.c_str();
}

} // extern "C"
