#include "SQLite.hpp"

// SQLite 
// SQLite.cpp
// Version: 0.2

#include "sqlite3.h"
#include <cstdio>
#include <limits>

#include <windows.h> // for w2a only
#include <string> // for w2a only
#include <vector> // for w2a only

bool SQLite::Initialize () {
    return sqlite3_initialize () == SQLITE_OK;
};
void SQLite::Terminate () {
    sqlite3_shutdown ();
    return;
};

namespace {
std::string w2a (const std::wstring & w) {
    
    std::vector <char> s;
    if (auto n = WideCharToMultiByte (CP_UTF8, 0, w.c_str (), (int) w.length (), NULL, 0, NULL, NULL)) {
        s.resize (n);
        if (WideCharToMultiByte (CP_UTF8, 0, w.c_str (), (int) w.length (), &s[0], n, NULL, NULL)) {
            return std::string (s.begin (), s.end ());
        };
    };
    
    return std::string ();
};

std::string sql_text (sqlite3_stmt * stmt) {
    if (auto * sz = sqlite3_sql (stmt))
        return sz;
    else
        return "--empty--";
};
};

SQLite::Exception::Exception (const std::string & op, sqlite3 * db, std::string q)
    :   std::runtime_error (op + ": " + sqlite3_errmsg (db) + " IN " + q),
        error (sqlite3_errcode (db)),
        extended (sqlite3_extended_errcode (db)),
        query (q) {};

SQLite::Exception::Exception (const std::string & op, sqlite3 * db, std::wstring q)
    :   std::runtime_error (op + ": " + sqlite3_errmsg (db) + " IN " + w2a (q)),
        error (sqlite3_errcode (db)),
        extended (sqlite3_extended_errcode (db)),
        query (w2a (q)) {};

SQLite::Exception::Exception (const std::string & op, sqlite3_stmt * stmt)
    :   std::runtime_error (op + ": " + sqlite3_errmsg (sqlite3_db_handle (stmt)) + " IN " + sql_text (stmt)),
        error (sqlite3_errcode (sqlite3_db_handle (stmt))),
        extended (sqlite3_extended_errcode (sqlite3_db_handle (stmt))),
        query (sql_text (stmt)) {};

SQLite::Statement::Statement (sqlite3_stmt * stmt)
    : stmt (stmt),
      bi (0) {};

SQLite::Statement::Statement (SQLite::Statement && s)
    : stmt (s.stmt),
      bi (s.bi) {
    s.stmt = nullptr;
};

SQLite::Statement & SQLite::Statement::operator = (SQLite::Statement && s) {
    std::swap (this->stmt, s.stmt);
    std::swap (this->bi, s.bi);
    return *this;
};

SQLite::Statement::~Statement () {
    sqlite3_finalize (this->stmt);
};

bool SQLite::Statement::empty () const {
    return sqlite3_sql (this->stmt) == nullptr;
};

void SQLite::Statement::execute () {
    if (sqlite3_step (this->stmt) != SQLITE_DONE)
        throw SQLite::Exception ("step !done", this->stmt);
};

bool SQLite::Statement::next () {
    switch (sqlite3_step (this->stmt)) {
        case SQLITE_ROW:
            return true;
        case SQLITE_DONE:
            this->reset ();
            return false;
        default:
            throw SQLite::Exception ("step", this->stmt);
    };
};

void SQLite::Statement::reset () {
    this->bi = 0;
    if (sqlite3_reset (this->stmt) != SQLITE_OK)
        throw SQLite::Exception ("reset", this->stmt);
};

int SQLite::Statement::width () const {
    auto n = sqlite3_data_count(this->stmt);
    return n ? n : sqlite3_column_count (this->stmt);
};
SQLite::Type SQLite::Statement::type (int i) const {
    return (SQLite::Type) sqlite3_column_type (this->stmt, i);
};
std::wstring SQLite::Statement::name (int i) const {
//    if (auto psz = static_cast <const wchar_t *> (sqlite3_column_origin_name16 (this->stmt, i)))
//        return psz;
//    else
    if (auto psz = static_cast <const wchar_t *> (sqlite3_column_name16 (this->stmt, i)))
        return psz;
    else
        throw std::bad_alloc ();
};
bool SQLite::Statement::null (int i) const {
    return sqlite3_column_type (this->stmt, i) == SQLITE_NULL;
};

template <> int SQLite::Statement::get <int> (int column) const {
    return sqlite3_column_int (this->stmt, column);
};
template <> double SQLite::Statement::get <double> (int column) const {
    return sqlite3_column_double (this->stmt, column);
};
template <> long long SQLite::Statement::get <long long> (int column) const {
    return sqlite3_column_int64 (this->stmt, column);
};
template <> std::wstring SQLite::Statement::get <std::wstring> (int column) const {
    if (auto ptr = sqlite3_column_text16 (this->stmt, column)) {
        auto size = sqlite3_column_bytes16 (this->stmt, column) / 2;
        auto data = static_cast <const wchar_t *> (ptr);
        
        return std::wstring (data, data + size);
    } else
        return std::wstring ();
};
/*
template <> std::u8string SQLite::Statement::get <std::u8string> (int column) const {
    if (auto ptr = sqlite3_column_text (this->stmt, column)) {
        auto size = sqlite3_column_bytes (this->stmt, column);
        auto data = static_cast <const char *> (ptr);
        
        return std::u8string (data, data + size);
    } else
        return std::u8string ();
};*/

void SQLite::Statement::bind (int value) {
    if (sqlite3_bind_int (this->stmt, ++this->bi, value) != SQLITE_OK)
        throw SQLite::Exception ("bind", this->stmt);
};
void SQLite::Statement::bind (double value) {
    if (sqlite3_bind_double (this->stmt, ++this->bi, value) != SQLITE_OK)
        throw SQLite::Exception ("bind", this->stmt);
};
void SQLite::Statement::bind (long long value) {
    if (sqlite3_bind_int64 (this->stmt, ++this->bi, value) != SQLITE_OK)
        throw SQLite::Exception ("bind", this->stmt);
};
void SQLite::Statement::bind (unsigned long long value) {
    if (sqlite3_bind_int64 (this->stmt, ++this->bi, (long long) value) != SQLITE_OK)
        throw SQLite::Exception ("bind", this->stmt);
};
void SQLite::Statement::bind (const std::wstring & value) {
    if (sqlite3_bind_text16 (this->stmt, ++this->bi, value.data (), (int) (value.size () * sizeof (wchar_t)), SQLITE_TRANSIENT) != SQLITE_OK)
        throw SQLite::Exception ("bind", this->stmt);
};
void SQLite::Statement::bind (const std::vector <unsigned char> & value) {
    static const unsigned char empty [1] = { 0 };
    if (sqlite3_bind_blob (this->stmt, ++this->bi,
                           value.empty () ? empty : value.data (), (int) value.size (),
                           SQLITE_TRANSIENT) != SQLITE_OK)
        throw SQLite::Exception ("bind", this->stmt);
};
/*void SQLite::Statement::bind (const void * blob, std::size_t size) {
    if (size > std::numeric_limits <int>::max ())
        throw std::out_of_range ("bind: blob size");
    
    if (sqlite3_bind_blob (this->stmt, ++this->bi, blob, size, SQLITE_TRANSIENT) != SQLITE_OK)
        throw SQLite::Exception ("bind", this->stmt);
};*/
void SQLite::Statement::bind (std::nullptr_t) {
    if (sqlite3_bind_null (this->stmt, ++this->bi) != SQLITE_OK)
        throw SQLite::Exception ("bind", this->stmt);
};

template <> std::vector <unsigned char> SQLite::Statement::get <std::vector <unsigned char>> (int column) const {
    if (auto ptr = sqlite3_column_blob (this->stmt, column)) {
        auto size = sqlite3_column_bytes (this->stmt, column);
        auto data = static_cast <const unsigned char *> (ptr);
    
        return std::vector <unsigned char> (data, data + size);
    } else
        return std::vector <unsigned char> ();
};

bool SQLite::open (const std::wstring & filename) {
    sqlite3 * newdb = nullptr;
    if (sqlite3_open16 (filename.c_str (), &newdb) == SQLITE_OK) {
        this->close ();
        this->db = newdb;
        return true;
    } else
        return false;
};
void SQLite::close () {
    if (this->db) {
        sqlite3_close (this->db);
        this->db = nullptr;
    }
};

SQLite::Statement SQLite::prepare (const std::wstring & query) {
    sqlite3_stmt * statement;
    if (sqlite3_prepare16_v2 (this->db, query.c_str (), -1, &statement, nullptr) == SQLITE_OK)
        return statement;
    else
        throw SQLite::Exception ("prepare", this->db, w2a (query));
};

std::size_t SQLite::changes () const {
    return sqlite3_changes (this->db);
};

long long SQLite::last_insert_rowid () const {
    return sqlite3_last_insert_rowid (this->db);
};

int SQLite::error () const {
    return sqlite3_errcode (this->db);
};
std::wstring SQLite::errmsg () const {
    wchar_t msg [256];
    _snwprintf (msg, sizeof msg / sizeof msg [0], L"%02X:%02X %s",
                sqlite3_errcode (this->db),
                sqlite3_extended_errcode (this->db) >> 8,
                (const wchar_t *) sqlite3_errmsg16 (this->db));
    return msg;
};

