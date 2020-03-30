/**************************************************************************
 *
 * Copyright (c) 2017-2018, luotang.me <wypx520@gmail.com>, China.
 * All rights reserved.
 *
 * Distributed under the terms of the GNU General Public License v2.
 *
 * This software is provided 'as is' with no explicit or implied warranties
 * in respect of its properties, including, but not limited to, correctness
 * and/or fitness for purpose.
 *
 **************************************************************************/
#ifndef DB_DBDRIVER_H
#define DB_DBDRIVER_H

#include <cassert>
#include <string>
#include <memory>
#include <functional>

namespace MSF {
namespace DB {

enum DbType {
  DB_TYPE_UNKNOWN = -1,
  DB_TYPE_SQLITE  = 0,
  DB_TYPE_POSTSQL = 1,
  DB_TYPE_MYSQL = 2,
  DB_TYPE_MONGO = 3,
  DB_TYPE_REDIS = 4,
};

class DBDriver {
  protected:
    enum DbType dbType_;
    bool connected_;
    std::string dbPath_;
  public:
    DBDriver(const enum DbType dbType)
     : dbType_(dbType) {

    }
    virtual ~DBDriver() { }

    const enum DbType dbType() const { return dbType_; }
    const bool connected() const { return connected_; }

    virtual bool connect() {
      return true;
    }
    virtual bool disconn() {
      return true;
    }

    virtual bool addUser(const std::string & user) {
      return true;
    }
    virtual bool delUser(const std::string & user) {
      return true;
    }
    virtual bool findUser(const std::string & user) {
      return true;
    }
    virtual bool listUser() {
      return true;
    }
};


}
}
#endif