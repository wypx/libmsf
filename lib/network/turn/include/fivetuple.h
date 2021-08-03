///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <https://sourcey.com>
//
// SPDX-License-Identifier:    LGPL-2.1+
//
/// @addtogroup turn
/// @{


#ifndef SCY_TURN_FiveTuple_H
#define SCY_TURN_FiveTuple_H


#include "scy/turn/turn.h"
#include "scy/net/socket.h"

#include <sstream>


namespace scy {
namespace turn {


/// The 5-TUPLE consists of a local, a remote address, and the
/// transport protocol used by the client to communicate with the server.
///
///                                                               +---------+
///                                                               |         |
///                                                               | External|
///                                                             / | Client  |
///                                                           //  |         |
///                                                          /    |         |
///                                                        //     +---------+
///                                                       /
///                                                     //
///                     +-+                            /
///                     | |                           /
///                     | |                         //
///      +---------+    | |          +---------+   /              +---------+
///      |         |    |N|          |         | //               |         |
///      | TURN    |    | |          |         |/                 | External|
///      | Client  |----|A|----------|   TURN  |------------------| Client  |
///      |         |    | |^        ^|  Server |^                ^|         |
///      |         |    |T||        ||         ||                ||         |
///      +---------+    | ||        |+---------+|                |+---------+
///         ^           | ||        |           |                |
///         |           | ||        |           |                |
///         |           +-+|        |           |                |
///         |              |        |           |                |
///         |
///                    Internal     Internal    External         External
///     Client         Remote       Local       Local            Remote
///     Performing     Transport    Transport   Transport        Transport
///     Allocations    Address      Address     Address          Address
///
///                        |          |            |                |
///                        +-----+----+            +--------+-------+
///                              |                          |
///                              |                          |
///
///                            Internal                External
///                            5-Tuple                 5-tuple
///
class TURN_API FiveTuple
{
public:
    FiveTuple();
    FiveTuple(const net::Address& remote, const net::Address& local,
              net::TransportType transport);
    FiveTuple(const FiveTuple& r);

    const net::Address& remote() const { return _remote; }
    const net::Address& local() const { return _local; }
    const net::TransportType& transport() const { return _transport; }

    void remote(const net::Address& remote) { _remote = remote; }
    void local(const net::Address& local) { _local = local; }
    void transport(const net::TransportType& transport)
    {
        _transport = transport;
    }

    bool operator==(const FiveTuple& r) const;
    bool operator<(const FiveTuple& r) const;

    std::string toString() const;

    friend std::ostream& operator<<(std::ostream& stream,
                                    const FiveTuple& tuple)
    {
        stream << tuple.toString();
        return stream;
    }

private:
    net::Address _remote;
    net::Address _local;
    net::TransportType _transport;
};


} // namespace turn
} // namespace scy


#endif // SCY_TURN_FiveTuple_H


/// @\}
