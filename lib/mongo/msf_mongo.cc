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
#include <base/mongodb/msf_mongo.h>

using namespace MSF::MONGO;


namespace MSF {
namespace MONGO {

MSF_MONGO::MSF_MONGO(/* args */)
{
    pstMongdbPath = "mongodb://localhost:27017";
    DbConnect();    
}

MSF_MONGO::~MSF_MONGO()
{

}

void MSF_MONGO::DbInit() 
{
    std::cout << "Db Enter init"<<std::endl;
    // The mongocxx::instance constructor and destructor initialize and shut down the driver,
    // respectively. Therefore, a mongocxx::instance must be created before using the driver and
    // must remain alive for as long as the driver is in use.
    mongocxx::instance instance{}; // This should be done only once.
}

void MSF_MONGO::DbConnect()
{
    mongocxx::uri uri(pstMongdbPath);
    cli = mongocxx::client(uri);

    /* Access a Database
     * Once you have a mongocxx::client instance connected to a MongoDB deployment, 
     * use either the database() method or operator[] to obtain a mongocxx::database instance.
     * */
    db = cli.database("Ucloud");
    
    /* Access a Collection
     * Once you have a mongocxx::database instance, 
     * use either the collection() method or operator[] to obtain a mongocxx::collection instance.
     * */
    coll = db.collection("UDiskRequest");
}


void MSF_MONGO::DbInert(Ucloud::CreateUDiskRequest *pstMsg)
{
    try {
        // Currently, result will always be true (or an exception will be
        // thrown).  Eventually, unacknowledged writes will give a false
        // result. See https://jira.mongodb.org/browse/CXX-894

        auto result = coll.insert_one(make_document(
            kvp("top_id", (int)pstMsg->top_oid()),
            kvp("oid", (int)pstMsg->oid()),
            kvp("extern_id", pstMsg->extern_id()),
            kvp("disk_name", pstMsg->name()),
            kvp("size", (int)pstMsg->size()),
            kvp("disk_type", (int)pstMsg->disk_type())
            ));
        if (!result) {
            std::cout << "Unacknowledged write. No id available." << std::endl;
            return;
        }

        if (result->inserted_id().type() == bsoncxx::type::k_oid) {
            bsoncxx::oid id = result->inserted_id().get_oid().value;
            std::string id_str = id.to_string();
            std::cout << "Inserted id: " << id_str << std::endl;
        } else {
            std::cout << "Inserted id was not an OID type" << std::endl;
        }
    } catch (const mongocxx::exception& e) {
        std::cout << "An exception occurred: " << e.what() << std::endl;
        return;
    }
    DbAggregate();
}

void MSF_MONGO::DbUpdate(Ucloud::CreateUDiskRequest *pstMsg)
{
 
}
void UMoMSF_MONGOngdb::DbUpdateOne(Ucloud::CreateUDiskRequest *pstMsg)
{
    auto result = coll.update_one(
        make_document(kvp("top_id", (int)pstMsg->top_oid())),
        make_document(kvp("top_id", 11),
            kvp("oid", (int)pstMsg->oid()),
            kvp("extern_id", pstMsg->extern_id()),
            kvp("disk_nametype", pstMsg->name()),
            kvp("size", (int)pstMsg->size()),
            kvp("disk_type", (int)pstMsg->disk_type())
            ));
}
void MSF_MONGO::DbUpdateMany(Ucloud::CreateUDiskRequest *pstMsg)
{
    // Update multiple documents.
    auto result = coll.update_many(
        make_document(kvp("top_id", (int)pstMsg->top_oid())),
        make_document(kvp("top_id", 12),
            kvp("oid", (int)pstMsg->oid()),
            kvp("extern_id", pstMsg->extern_id()),
            kvp("disk_nametype", pstMsg->name()),
            kvp("size", (int)pstMsg->size()),
            kvp("disk_type", (int)pstMsg->disk_type())
            ));
}

 void MSF_MONGO::DbReplaceOne(Ucloud::CreateUDiskRequest *pstMsg)
 {
    // Replace the contents of a single document.
    auto result = coll.replace_one(
        make_document(kvp("top_id", (int)pstMsg->top_oid())),
        make_document(kvp("top_id", 13),
            kvp("oid", (int)pstMsg->oid()),
            kvp("extern_id", pstMsg->extern_id()),
            kvp("disk_nametype", pstMsg->name()),
            kvp("size", (int)pstMsg->size()),
            kvp("disk_type", (int)pstMsg->disk_type())
            ));
 }

void MSF_MONGO::DbDeleteAny()
{
    // Remove all documents in a collection.
    {
        coll.delete_many({});
    }
}
void MSF_MONGO::DbDeleteAll()
{
    // Remove all documents that match a condition.
    {
       coll.delete_many(make_document(kvp("top_id", 10)));
    }
}
void MSF_MONGO::DbDeleteOne()
{
     // Remove one document that matches a condition.
    {
        coll.delete_one(make_document(kvp("top_id", 10)));
    }
}

 void MSF_MONGO::DbDrop()
 {
     // Drop a collection.
     coll.drop();
 }
void MSF_MONGO::DbQueryAll()
{
    std::cout << "Db Enter DbQueryAll"<<std::endl;
    // Query for all the documents in a collection.
    {
        // @begin: cpp-query-all
        auto cursor = coll.find({});
        for (auto&& doc : cursor) {
            std::cout << bsoncxx::to_json(doc) << std::endl;
        }
        // @end: cpp-query-all
    }
}
void MSF_MONGO::DbQueryOne()
{
    std::cout << "Db Enter DbQueryOne"<<std::endl;
    // Query for equality on a top level field.
    {
        // @begin: cpp-query-top-level-field
        auto cursor = coll.find(make_document(kvp("disk_type", 70)));

        for (auto&& doc : cursor) {
            std::cout << bsoncxx::to_json(doc) << std::endl;
        }
        // @end: cpp-query-top-level-field
    }

    std::cout << "Db Enter DbQueryOne gt"<<std::endl;
    // Query with the greater-than operator ($gt).
    {
        auto cursor = coll.find(
            make_document(kvp("disk_type", make_document(kvp("$gt", 100)))));
        for (auto&& doc : cursor) {
            std::cout << bsoncxx::to_json(doc) << std::endl;
        }
    }

    std::cout << "Db Enter DbQueryOne lt"<<std::endl;
    // Query with the less-than operator ($lt).
    {
        auto cursor = coll.find(
            make_document(kvp("disk_type", make_document(kvp("$lt", 1)))));
        for (auto&& doc : cursor) {
            std::cout << bsoncxx::to_json(doc) << std::endl;
        }
    }

    std::cout << "Db Enter DbQueryOne and"<<std::endl;
    // Query with a logical conjunction (AND) of query conditions.
    {
        auto cursor = coll.find(
            make_document(kvp("disk_type",70), kvp("top_id", 10)));
        for (auto&& doc : cursor) {
            std::cout << bsoncxx::to_json(doc) << std::endl;
        }
    }

    std::cout << "Db Enter DbQueryOne or"<<std::endl;
    // Query with a logical disjunction (OR) of query conditions.
    {
        auto cursor = coll.find(
            make_document(kvp("$or",
                              make_array(make_document(kvp("disk_type", "70")),
                                         make_document(kvp("top_id", 10))))));
        for (auto&& doc : cursor) {
            std::cout << bsoncxx::to_json(doc) << std::endl;
        }
    }

    std::cout << "Db Enter DbQueryOne sort"<<std::endl;
    // Sort query results.
    {
        mongocxx::options::find opts;
        opts.sort(make_document(kvp("disk_type", 70), kvp("top_id", 10)));

        auto cursor = coll.find({}, opts);
        for (auto&& doc : cursor) {
            std::cout << bsoncxx::to_json(doc) << std::endl;
        }
    }
}

/* MongoDB 聚合
 * https://www.runoob.com/mongodb/mongodb-aggregate.html
 * 
 * MongoDB中聚合(aggregate)主要用于处理数据(诸如统计平均值,求和等),
 * 并返回计算后的数据结果。有点类似sql语句中的 count(*)。
 * 
 * */

void MSF_MONGO::DbAggregate()
{
    std::cout << "Db Enter DbAggregate by filed"<<std::endl;
    // Group documents by field and calculate count.
    {
        mongocxx::pipeline stages;

        stages.group(
            make_document(kvp("_id", "$top_id"), kvp("count", make_document(kvp("$sum", 1)))));

        auto cursor = coll.aggregate(stages);

        for (auto&& doc : cursor) {
            std::cout << bsoncxx::to_json(doc) << std::endl;
        }
    }

    std::cout << "Db Enter DbAggregate by group"<<std::endl;
    // Filter and group documents.
    {
        mongocxx::pipeline stages;

        stages.match(make_document(kvp("top_id", 10), kvp("disk_type", 70)))
            .group(make_document(kvp("_id", "$disk_name"),
                                 kvp("count", make_document(kvp("$sum", 1)))));

        auto cursor = coll.aggregate(stages);

        for (auto&& doc : cursor) {
            std::cout << bsoncxx::to_json(doc) << std::endl;
        }
    }
}
void MSF_MONGO::DbLogout()
{
    
}

}
}