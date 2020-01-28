/**************************************************************************
*
* Copyright (c) 2017-2021, luotang.me <wypx520@gmail.com>, China.
* All rights reserved.
*
* Distributed under the terms of the GNU General Public License v2.
*
* This software is provided 'as is' with no explicit or implied warranties
* in respect of its properties, including, but not limited to, correctness
* and/or fitness for purpose.
*
**************************************************************************/
#ifndef __MSF_MONGO_H__
#define __MSF_MONGO_H__

#include <iostream>
#include <cstdlib>
#include <chrono>
#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/options/find.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/pipeline.hpp>

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_array;
using bsoncxx::builder::basic::make_document;

namespace MSF {
namespace MONGO {


class MSF_MONGO
{
private:
    std::string pstMongdbPath;
    mongocxx::pool pl;      //连接池
    mongocxx::client cli;
    mongocxx::database db;
    mongocxx::collection coll;
    
    //进程只初始化一次
    static void DbInit(void) __attribute__((constructor(101)));
    void DbConnect();
public:
    MSF_MONGO();
    ~MSF_MONGO();
    void DbInert(Ucloud::CreateUDiskRequest *pstMsg);
    void DbUpdate(Ucloud::CreateUDiskRequest *pstMsg);
    void DbUpdateOne(Ucloud::CreateUDiskRequest *pstMsg);
    void DbUpdateMany(Ucloud::CreateUDiskRequest *pstMsg);
    void DbReplaceOne(Ucloud::CreateUDiskRequest *pstMsg);
    void DbDeleteAny();
    void DbDeleteAll();
    void DbDeleteOne();
    void DbDrop();
    void DbQueryAll();
    void DbQueryOne();
    void DbAggregate();
    void DbLogout();
};

}
}

#endif