// =============================================================================
// ORM.CPP - SQLite ORM Implementation
// =============================================================================

#include "orm.h"
#include <sstream>

static thread_local std::string tl_query_result;

// Helper to convert rows to JSON
static std::string rowsToJson(const std::vector<Hub::ORM::Row> &rows) {
  std::ostringstream json;
  json << "[";

  // Since Row is opaque, we need to track columns during query
  // For now, return simplified format
  for (size_t i = 0; i < rows.size(); ++i) {
    if (i > 0)
      json << ",";
    json << "{}"; // Placeholder - full impl would serialize row
  }

  json << "]";
  return json.str();
}

extern "C" {

__declspec(dllexport) int hub_orm_open(const char *path) {
  if (!path)
    return 0;
  return Hub::ORM::globalDB().open(path) ? 1 : 0;
}

__declspec(dllexport) void hub_orm_close() { Hub::ORM::globalDB().close(); }

__declspec(dllexport) int hub_orm_execute(const char *sql) {
  if (!sql)
    return 0;
  return Hub::ORM::globalDB().execute(sql) ? 1 : 0;
}

__declspec(dllexport) const char *hub_orm_query_json(const char *sql) {
  if (!sql)
    return "[]";
  auto rows = Hub::ORM::globalDB().query(sql);
  tl_query_result = rowsToJson(rows);
  return tl_query_result.c_str();
}

__declspec(dllexport) long long hub_orm_last_insert_id() {
  return Hub::ORM::globalDB().lastInsertId();
}

__declspec(dllexport) long long hub_orm_count(const char *table,
                                              const char *where) {
  if (!table)
    return 0;
  return Hub::ORM::globalDB().count(table, where ? where : "");
}

__declspec(dllexport) int hub_orm_begin() {
  return Hub::ORM::globalDB().beginTransaction() ? 1 : 0;
}

__declspec(dllexport) int hub_orm_commit() {
  return Hub::ORM::globalDB().commit() ? 1 : 0;
}

__declspec(dllexport) int hub_orm_rollback() {
  return Hub::ORM::globalDB().rollback() ? 1 : 0;
}

__declspec(dllexport) int hub_orm_create_table(const char *name,
                                               const char *schema_json) {
  if (!name)
    return 0;

  // Basic table creation - for full schema_json parsing, would need JSON parser
  // For now, use simple column definitions
  Hub::ORM::Schema schema;
  schema.id();
  schema.timestamps();

  std::string sql = schema.toCreateSql(name);
  return Hub::ORM::globalDB().execute(sql) ? 1 : 0;
}

__declspec(dllexport) int hub_orm_drop_table(const char *name) {
  if (!name)
    return 0;
  return Hub::ORM::globalDB().dropTable(name) ? 1 : 0;
}

} // extern "C"
