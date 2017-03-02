# SQLite
*Simple SQLite C++ classes*

## Example

    #include "SQlite.hpp"
    
    SQLite database;
    
    int main () {
        if (SQLite::Initialize ()) {
            try {
                if (database.open (L"test.db")) {
                    
                    // simple statements
                    database.execute (L"CREATE TABLE `test` (`a` INTEGER PRIMARY KEY NOT NULL, `b` INTEGER NOT NULL)");
                    database.execute (L"INSERT INTO `test` VALUES (1, 112)");
                    
                    // prepared statements
                    auto insert = database.prepare (L"INSERT INTO `test` VALUES (?, ?)");
                    
                    // binding more than one parameter
                    insert.execute (2, 233);
    
                    // binding more than one parameter
                    insert.reset ();
                    insert.bind (3, 334);
                    insert.execute ();
                    
                    // binding each parameter with a call
                    insert.reset ();
                    insert.bind (4);
                    insert.bind (445);
                    insert.execute ();
    
                    // single value query, result is '3'
                    printf ("%d\n", database.query <int> (L"SELECT ?+?", 1, 2));
                    
                    // full query
                    auto query = database.prepare (L"SELECT `a`, `b` FROM `test` ORDER BY `a` ASC");
                    while (query.next ()) {
                        printf ("%d '%ls'\n",
                                query.get <int> (L"a"), // getting value of column 'a' as int
                                query.get <std::wstring> (1) .c_str ()); // getting value of second column as string
                    }
    
                    database.execute (L"DROP TABLE `test`");
                } else {
                    printf ("open error: \n");
                }
            } catch (const SQLite::Exception & x) {
                printf ("exception: %s\n", x.what ());
            }
            SQLite::Terminate ();
        }
        return 0;
    }
