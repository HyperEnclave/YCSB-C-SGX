//
//  sqlite_db.cc
//  YCSB-C
//

#include "sqlite_db.h"

#include <assert.h>
#include <cstring>

using namespace std;

extern "C" int printf(const char* fmt, ...);
extern "C" int puts(const char* str);

namespace ycsbc {

const int MAX_LEN = 4096 - 1;
static char stmt[MAX_LEN + 1];

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
    int i;
    for(i = 0; i < argc; i++){
        std::string azColName_str = azColName[i];
        std::string argv_str = (argv[i] ? argv[i] : "NULL");
        printf("%s", (azColName_str + " = " + argv_str + "\n").c_str());
    }
    puts("");
    return 0;
}

int SqliteDB::execute_sql_key(const char *sql, const std::string& key) {
    int rc = 0;
    sqlite3_stmt *pStmt = 0;    /* The current SQL statement */
    rc = sqlite3_prepare_v2(this->db, stmt, -1, &pStmt, NULL);
    assert( rc==SQLITE_OK || pStmt==0 );
    if (rc) return rc;

    rc = sqlite3_bind_text(pStmt, 1, key.c_str(), key.length(), SQLITE_STATIC);
    if (rc) return rc;

    while (sqlite3_step(pStmt) == SQLITE_ROW)
    {
        // printf("STEP: %s\n", sqlite3_column_text(pStmt, 1));
    }
    rc = sqlite3_finalize(pStmt);
    return rc;
}

int SqliteDB::execute_sql_args(const char *sql, const std::vector<const std::string*>& args) {
    int rc = 0;
    sqlite3_stmt *pStmt = 0;    /* The current SQL statement */
    rc = sqlite3_prepare_v2(this->db, stmt, -1, &pStmt, NULL);
    assert( rc==SQLITE_OK || pStmt==0 );
    if (rc) return rc;

    for (size_t i = 0; i < args.size(); i++)
    {
        rc = sqlite3_bind_text(pStmt, i + 1, args[i]->c_str(), args[i]->length(), SQLITE_STATIC);
        if (rc) return rc;
    }

    while (sqlite3_step(pStmt) == SQLITE_ROW)
    {
        printf("STEP: %s", sqlite3_column_text(pStmt, 0));
    }
    rc = sqlite3_finalize(pStmt);
    return rc;
}

int SqliteDB::execute_sql(const char *sql) {
    int rc;
    char *zErrMsg = 0;
    rc = sqlite3_exec(this->db, sql, callback, 0, &zErrMsg);
    if (rc) {
        printf("SQLite error: ");
        puts(sqlite3_errmsg(db));
        return rc;
    }
    return 0;
}

void SqliteDB::Init() {
    const char dbname[] = ":memory:";
    int rc; // For return status of SQLite
    rc = sqlite3_open(dbname, &this->db); // Opening database
    if (rc) {
        puts("SQLite error - can't open database connection: ");
        puts(sqlite3_errmsg(this->db));
        return;
    }
    printf("Enclave: Created database connection to ");
    puts(dbname);

    const char table_name[] = "usertable";
    snprintf(stmt, MAX_LEN, "DROP TABLE IF EXISTS %s; CREATE TABLE IF NOT EXISTS %s (YCSB_KEY VARCHAR(64) PRIMARY KEY", table_name, table_name);
    for (size_t i = 0; i < 10; i++) {
        char field[256];
        snprintf(field, 255, ", FIELD%ld TEXT", i);
        strncat(stmt, field, MAX_LEN);
    }
    strncat(stmt, ");", MAX_LEN);
    puts(stmt);
    assert(this->execute_sql(stmt) == 0);
}

void SqliteDB::Close() {
    sqlite3_close(this->db);
    puts("Enclave: Closed database connection");
}

int SqliteDB::Read(const std::string &table, const std::string &key,
        const std::vector<std::string> *fields,
        std::vector<KVPair> &result) {
    std::vector<const std::string*> args;
    if (!fields)
        snprintf(stmt, MAX_LEN, "SELECT * FROM %s WHERE YCSB_KEY = ?", table.c_str());
    else
    {
        strncpy(stmt, "SELECT ", MAX_LEN);
        bool first = true;
        for (size_t i = 0; i < fields->size(); i++)
        {
            if (first) first = false;
            else strncat(stmt, ",", MAX_LEN);
            strncat(stmt, (*fields)[i].c_str(), MAX_LEN);
        }
        snprintf(stmt, MAX_LEN, "SELECT * FROM %s WHERE YCSB_KEY = ?", table.c_str());
    }
    // puts(stmt);
    return this->execute_sql_key(stmt, key);
}

int SqliteDB::Scan(const std::string &table, const std::string &key,
        int len, const std::vector<std::string> *fields,
        std::vector<std::vector<KVPair>> &result) {
    // printf("SCAN %s %s %d %d\n", table.c_str(), key.c_str(), len, fields->size());
    return 0;
}

int SqliteDB::Update(const std::string &table, const std::string &key,
            std::vector<KVPair> &values) {
    snprintf(stmt, MAX_LEN, "UPDATE %s SET ", table.c_str());
    bool first = true;
    std::vector<const std::string*> args(values.size() + 1);
    for (size_t i = 0; i < values.size(); i++)
    {
        if (first) first = false;
        else strncat(stmt, ", ", MAX_LEN);
        strncat(stmt, values[i].first.c_str(), MAX_LEN);
        strncat(stmt, "=?", MAX_LEN);
        args[i] = &values[i].second;
    }
    strncat(stmt, " WHERE YCSB_KEY = ?;", MAX_LEN);
    args[values.size()] = &key;
    // puts(stmt);

    return this->execute_sql_args(stmt, args);
}

int SqliteDB::Insert(const std::string &table, const std::string &key,
            std::vector<KVPair> &values) {
    snprintf(stmt, MAX_LEN, "INSERT INTO %s (YCSB_KEY", table.c_str());
    for (auto kv : values)
    {
        strncat(stmt, ", ", MAX_LEN);
        strncat(stmt, kv.first.c_str(), MAX_LEN);
    }
    strncat(stmt, ") VALUES (?", MAX_LEN);

    std::vector<const std::string*> args(values.size() + 1);
    args[0] = &key;
    for (size_t i = 0; i < values.size(); i++)
    {
        strncat(stmt, ", ?", MAX_LEN);
        args[i + 1] = &values[i].second;
    }
    strncat(stmt, ");", MAX_LEN);
    // puts(stmt);

    return this->execute_sql_args(stmt, args);
}

int SqliteDB::Delete(const std::string &table, const std::string &key) {
    snprintf(stmt, 256, "DELETE FROM %s WHERE YCSB_KEY = ?;", table.c_str());
    puts(stmt);
    return this->execute_sql_key(stmt, key);
}


} // namespace ycsbc
