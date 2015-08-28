#include "sqlite3.hpp"

namespace sqlite {
sDatabase mk_db(const char* dbName) {
  sqlite3 *db;
  
  if(sqlite3_open(dbName, &db) != SQLITE_OK) {
    err::set(sqlite3_errmsg(db));
  }
  
  return sDatabase { db };
}

sStatement mk_statement(sqlite3 *db, const char *sql) {
  sqlite3_stmt *statement;
  
  if(sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK) {
    err::set(sqlite3_errmsg(db));
  };
  
  return sStatement { statement };
}

Database::Database(const char* dbName) : _db(mk_db(dbName)) {}
Database::Database(const std::string& dbName) : Database(dbName.c_str()) {}

const char *comp_to_string[] {
  "==",
  "!=",
  ">",
  "<",
  "<=",
  ">="
};
}