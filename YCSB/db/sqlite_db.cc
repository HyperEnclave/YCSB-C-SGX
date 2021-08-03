//
//  sqlite_db.cc
//  YCSB-C
//

#include "sqlite_db.h"

#include <assert.h>
#include <cstring>
#include <iostream>

using namespace std;

namespace ycsbc {

// std::mutex mutex_;

const int MAX_LEN = 4096 - 10;
static char stmt[MAX_LEN + 10];

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
    int i;
    for(i = 0; i < argc; i++){
        std::string azColName_str = azColName[i];
        std::string argv_str = (argv[i] ? argv[i] : "NULL");
        ocall_print_string((azColName_str + " = " + argv_str + "\n").c_str());
    }
    ocall_print_string("\n");
    return 0;
}

int SqliteDB::execute_sql_key(const char *sql, const std::string& key) {
    int rc = 0;
    char *zErrMsg = 0;
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
    char *zErrMsg = 0;
    sqlite3_stmt *pStmt = 0;    /* The current SQL statement */
    rc = sqlite3_prepare_v2(this->db, stmt, -1, &pStmt, NULL);
    assert( rc==SQLITE_OK || pStmt==0 );
    if (rc) return rc;

    // rc = sqlite3_bind_text(pStmt, 1, key.c_str(), key.length(), SQLITE_STATIC);
    // if (rc) return rc;
    for (int i = 0; i < args.size(); i++)
    {
        // cout<< i<<' '<<*args[i]<<endl;
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
        ocall_print_string("SQLite error: ");
        ocall_println_string(sqlite3_errmsg(db));
        return rc;
    }
    return 0;
}

SqliteDB::SqliteDB() {
    const char dbname[] = ":memory:";
    int rc; // For return status of SQLite
    rc = sqlite3_open(dbname, &this->db); // Opening database
    if (rc) {
        ocall_println_string("SQLite error - can't open database connection: ");
        ocall_println_string(sqlite3_errmsg(this->db));
        return;
    }
    ocall_print_string("Enclave: Created database connection to ");
    ocall_println_string(dbname);

    const char table_name[] = "usertable";
    snprintf(stmt, MAX_LEN, "DROP TABLE IF EXISTS %s; CREATE TABLE IF NOT EXISTS %s (YCSB_KEY VARCHAR(64) PRIMARY KEY", table_name, table_name);
    for (int i = 0; i < 10; i++) {
        char field[256];
        sprintf(field, ", FIELD%d TEXT", i);
        strncat(stmt, field, MAX_LEN);
    }
    strncat(stmt, ");", MAX_LEN);
    puts(stmt);
    assert(this->execute_sql(stmt) == 0);
}

SqliteDB::~SqliteDB() {
    sqlite3_close(this->db);
    ocall_println_string("Enclave: Closed database connection");
}

int SqliteDB::Read(const std::string &table, const std::string &key,
        const std::vector<std::string> *fields,
        std::vector<KVPair> &result) {
    std::vector<const std::string*> args;
    if (!fields)
        snprintf(stmt, MAX_LEN, "SELECT * FROM %s WHERE YCSB_KEY = ?", table.c_str());
    else
    {
        strcpy(stmt, "SELECT ");
        bool first = true;
        for (int i = 0; i < fields->size(); i++)
        {
            if (first) first = false;
            else strncat(stmt, ",", MAX_LEN);
            strncat(stmt, (*fields)[i].c_str(), MAX_LEN);
        }
        snprintf(stmt, MAX_LEN, "SELECT * FROM %s WHERE YCSB_KEY = ?", table.c_str());
    }
    // puts(stmt);
    return this->execute_sql_key(stmt, key);
    // exit(0);
    // snprintf(cmd, 256, "DROP TABLE IF EXISTS %s; CREATE TABLE IF NOT EXISTS %s (YCSB_KEY VARCHAR(64) PRIMARY KEY", table_name, table_name);

    // int rc;
    // char *zErrMsg = 0;
    // rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    // if (rc) {
    //     ocall_print_string("SQLite error: ");
    //     ocall_println_string(sqlite3_errmsg(db));
    //     return;
    // }
    // return 0;
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
    for (int i = 0; i < values.size(); i++)
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
    for (int i = 0; i < values.size(); i++)
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
