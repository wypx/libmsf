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
#ifndef __MSF_LOG_STREAM_H__
#define __MSF_LOG_STREAM_H__

#include <string.h>

#include <iostream>

namespace MSF {
namespace BASE {

// class MSF_LogStream {
//     public:
//         MSF_LogStream() = default;
//         virtual ~MSF_LogStream();

//         std::ostream& stream() {
//             return fmtStr;
//         }
//     private:
//         enum LogLevel   iLogLevel;  /* min level of log */

//         std::ostringstream fmtStr;

//         /* String data generated in the constructor,
//          * that should be appended to the message before output. */
//         std::ostringstream extraStr;
// };

}  // namespace BASE
}  // namespace MSF
#endif
