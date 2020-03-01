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
#ifndef __MSF_VERSION_H__
#define __MSF_VERSION_H__

#include <base/Logger.h>

#include <iomanip>

using namespace MSF::BASE;

namespace MSF {

/* 两次转换将宏的值转成字符串 */
#define _MSF_MACROSTR(a) (#a)
#define MSF_MACROSTR(a) (_MSF_MACROSTR(a))

/* 用于标识版本号 可以用nm -C查看 */
#ifdef MSF_VERSION
const MSF_S8 *MSF_VERSION = MSF_MACROSTR(MSF_VERSION);
#else
const char *MSF_VERSION = "Micro Service Framework 0.1";
#endif

static const std::string MSF_GITHUB = "https://github.com/wypx/libmsf";

static void BuildProject() {
  std::cout << std::endl
            << std::endl
            << "          Build systime  : " << __TIME__ << " " << __DATE__
            << std::endl
            << "          Build version  : " << MSF_VERSION << std::endl
            << "          Project github : " << MSF_GITHUB << std::endl
            << std::endl
            << "          Welcome to join the conference at http://luotang.me "
            << std::endl
            << std::endl;
}

static void BuildLogo(void) {
  std::cout << std::endl
            << "                       .::::.                                  "
            << std::endl
            << "                     .::::::::.                                "
            << std::endl
            << "                    :::::::::::                                "
            << std::endl
            << "                 ..:::::::::::'                                "
            << std::endl
            << "               '::::::::::::'                                  "
            << std::endl
            << "                .::::::::::                                    "
            << std::endl
            << "           '::::::::::::::..                                   "
            << std::endl
            << "                ..::::::::::::.                                "
            << std::endl
            << "              ``::::::::::::::::                               "
            << std::endl
            << "               ::::``:::::::::'        .:::.                   "
            << std::endl
            << "              ::::'   ':::::'       .::::::::.                 "
            << std::endl
            << "            .::::'      ::::     .:::::::'::::.                "
            << std::endl
            << "           .:::'       :::::  .:::::::::' ':::::.              "
            << std::endl
            << "          .::'        :::::.:::::::::'      ':::::.            "
            << std::endl
            << "         .::'         ::::::::::::::'         ``::::.          "
            << std::endl
            << "     ...:::           ::::::::::::'              ``::.         "
            << std::endl
            << "    ```` ':.          ':::::::::'                  ::::..      "
            << std::endl
            << "                       '.:::::'                    ':'````..   "
            << std::endl
            << std::endl;
}

static void BuildStart() {
  std::cout << std::endl
            << "******************** Micro Service Framework Starting "
               "*****************************"
            << std::endl
            << std::endl;
}

static void BuildInfo(void) {
  BuildProject();
  BuildLogo();
  BuildStart();
}

}  // namespace MSF
#endif