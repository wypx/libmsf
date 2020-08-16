#include <base/Logger.h>
#include <base/Thread.h>

#include "SqliteDriver.h"


bool SqliteDriver::initMutiThread() {
#if defined(SQLITE_CONFIG_MULTITHREAD)
  sqlite3_shutdown();
  if (sqlite3_threadsafe() > 0) {
    int retCode = sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
    if (retCode != SQLITE_OK) {
      retCode = sqlite3_config(SQLITE_CONFIG_SERIALIZED);
      if (retCode != SQLITE_OK) {
        LOG_ERROR << "Setting sqlite thread safe mode to serialized failed!!! ret:" << retCode;
        return false;
      }
    }
  } else {
    LOG_ERROR << "Your SQLite database is not compiled to be threadsafe.";
    return false;
  }
#endif
  return true;
}

bool SqliteDriver::initDataBase() {
  const char * statements[] = {
    "CREATE TABLE turnusers_lt ( realm varchar(127) default '', name varchar(512), hmackey char(128), PRIMARY KEY (realm,name))",
    "CREATE TABLE turn_secret (realm varchar(127) default '', value varchar(127), primary key (realm,value))",
    "CREATE TABLE allowed_peer_ip (realm varchar(127) default '', ip_range varchar(256), primary key (realm,ip_range))",
    "CREATE TABLE denied_peer_ip (realm varchar(127) default '', ip_range varchar(256), primary key (realm,ip_range))",
    "CREATE TABLE turn_origin_to_realm (origin varchar(127),realm varchar(127),primary key (origin))",
    "CREATE TABLE turn_realm_option (realm varchar(127) default '',	opt varchar(32),	value varchar(128),	primary key (realm,opt))",
    "CREATE TABLE oauth_key (kid varchar(128),ikm_key varchar(256),timestamp bigint default 0,lifetime integer default 0,as_rs_alg varchar(64) default '',realm varchar(127) default '',primary key (kid))",
    "CREATE TABLE admin_user (name varchar(32), realm varchar(127), password varchar(127), primary key (name))",
    NULL
  };

  int i = 0;
  while(statements[i]) {
    sqlite3_stmt *statement = NULL;
    int rc = 0;
    if ((rc = sqlite3_prepare(conn_, statements[i], -1, &statement, 0)) == SQLITE_OK) {
      sqlite3_step(statement);
    }
    sqlite3_finalize(statement);
    ++i;
  }
  return true;
}

bool SqliteDriver::openDataBase() {
  assert(!dbPath_.empty());
  initMutiThread();
  int rc = sqlite3_open(dbPath_.c_str(), &conn_);
  if(!conn_ || (rc != SQLITE_OK)) {
    const char* errmsg = sqlite3_errmsg(conn_);
    if(conn_) {
      sqlite3_close(conn_);
      conn_ = nullptr;
    }
    return false;
  }
  return initDataBase();
}