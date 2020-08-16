#include <mutex>
#include <condition_variable>
#include <sqlite3.h>

#include "DbDriver.h"

class SqliteDriver : public MSF::DB::DBDriver {
  private:
    sqlite3 *conn_;

    bool initMutiThread();
    bool initDataBase();
    bool openDataBase();

  public:
    SqliteDriver();
    ~SqliteDriver();
};
