#ifndef SQLITE_HPP
#define SQLITE_HPP

/* SQLite 
// SQLite.hpp
// Version: 0.1
//
// Changelog:
//      20.10.2015 - initial version
*/

#include <stdexcept>
#include <string>
#include <vector>

struct sqlite3;
struct sqlite3_stmt;

class SQLite {
    public:
        enum Type {
            Integer = 1,
            Float = 2,
            Text = 3,
            Blob = 4,
            Null = 5,
        };
        
        class Exception : public std::runtime_error {
            public:
                const int error;
                const int extended;
                const std::string query;
            public:
                Exception (const std::string &, sqlite3 * db, std::string = std::string ());
                Exception (const std::string &, sqlite3 * db, std::wstring);
                Exception (const std::string &, sqlite3_stmt * stmt);
        };
        
        class Statement {
            friend class SQLite;
            private:
                sqlite3_stmt * stmt;
                int            bi;
                Statement (sqlite3_stmt *);
            public:
                Statement (Statement &&);
                Statement & operator = (Statement &&);
                ~Statement ();
                
                // empty
                //  - true if query is empty, other operations will throw
                
                bool empty () const;
                
                // insert/update/delete
                //  - binds to 1st, 2nd, 3rd, etc... parameters
                //  - call reset to start at 1st again
                
                void bind (int);
                void bind (double);
                void bind (long long);
                void bind (unsigned long long);
                void bind (const std::wstring & string);
                void bind (const std::vector <unsigned char> & blob);
                void bind (std::nullptr_t);
                
                // forwards

                void bind (unsigned  int v) { this->bind ((long long) v); };
                void bind (         long v) { this->bind ((long long) v); };
                void bind (unsigned long v) { this->bind ((long long) v); };
                void bind (  long double v) { this->bind ((double) v); };

                // binding variable number of arguments

                template <typename A, typename B, typename... Args>
                void bind (A a, B b, Args... args) {
                    this->bind (a);
                    this->bind (b);
                    this->bind (args...);
                };
                
                void execute (); // fails for select
                
                // select
                
                bool next (); // true if next row loaded, false at the end
                void reset (); // reset to run again (call .next)
                
                int          width () const; // number of columns
                std::wstring name (int) const; // column name
                bool         null (int) const; // is column NULL
                Type         type (int) const; // column type
                
                // get
                //  - int, double, wstring, vector <unsigned char>
                
                template <typename T> T get (int) const;
                template <typename T> T get (const std::wstring & column) const {
                    const auto n = this->width ();
                    for (int i = 0; i != n; ++i) {
                        if (column == this->name (i))
                            return this->get <T> (i);
                    };
                    throw SQLite::Exception ("no such column", this->stmt);
                };
                
                // query

                template <typename R, typename... Args>
                R query (Args... args) {
                    this->reset ();
                    this->bind (args...);
                    if (this->next ()) {
                        return this->get <R> (0);
                    } else
                        throw SQLite::Exception ("no data", this->stmt);
                };
                
            private:
                void bind () {};
        };
    
    public:
        static bool Initialize ();
        static void Terminate ();
    
    private:
        sqlite3 * db = nullptr;
    
    public:
        
        // open/close
        
        bool        open (const std::wstring &);
        void        close ();
        
        // prepare
        //  - creates new statement based on SQL query
        
        Statement   prepare (const std::wstring &);
        
        // execute
        //  - executes string query
        //  - returns number of changes
        
        template <typename... Args>
        std::size_t execute (const std::wstring & string, Args... args) {
            auto q = this->prepare (string);

            q.bind (args...);
            q.execute ();
            
            return this->changes ();
        };

        // query
        //  - executes string query
        //  - returns value of the first column of the first row
        
        template <typename R, typename... Args>
        R query (const std::wstring & string, Args... args) {
            return this->prepare (string) .query <R> (args...);
        };

        // changes
        //  - returns number of affected rows for INSERT, UPDATE and DELETE
        
        std::size_t changes () const;
        
        // error/errmsg
        //  - 
        
        int          error () const;
        std::wstring errmsg () const;
    
};

#endif

