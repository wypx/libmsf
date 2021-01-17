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
#ifndef BASE_VERSION_H_
#define BASE_VERSION_H_

#include <iomanip>
#include <iostream>

#include "utils.h"

using namespace MSF;

namespace MSF {

/* 两次转换将宏的值转成字符串 */
#define _MSF_MACROSTR(a) (#a)
#define MSF_MACROSTR(a) (_MSF_MACROSTR(a))

/* 用于标识版本号 可以用nm -C查看 */
#ifdef MSF_VERSION
const MSF_S8 *kVersionTag = MSF_MACROSTR(MSF_VERSION);
#else
const char *kVersionTag = "Micro Service Framework 0.1";
#endif

static const std::string kGithubProject = "https://github.com/wypx/libmsf";

std::string GetCompileTime() {
  uint32_t month;
  uint32_t day;
  uint32_t year;
  char month_str[8] = {0};
  sscanf(__DATE__, "%4s %d %d", month_str, &day, &year);

  static std::string kMonthes[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", };

  for (uint32_t i = 0; i < MSF_ARRAY_SIZE(kMonthes); ++i) {
    if (strstr(month_str, kMonthes[i].c_str())) {
      month = i + 1;
      break;
    }
  }

  std::stringstream build_time;
  build_time << std::setw(4) << std::setfill('0') << year << "-" << std::setw(2)
             << std::setfill('0') << month << "-" << std::setw(2)
             << std::setfill('0') << day << " " << __TIME__;

  return build_time.str();
}

static void BuildProject() {
  std::cout << "\n"
            << "          Build systime  : " << GetCompileTime() << "\n"
            << "          Build version  : " << kVersionTag << "\n"
            << "          Project github : " << kGithubProject << "\n"
            << "          Welcome to join the conference at http://luotang.me "
            << "\n";
}

static void BuildProject(const std::string &version,
                         const std::string &project) {
  std::cout << "\n"
            << "          Build systime  : " << GetCompileTime() << "\n"
            << "          Build version  : " << version << "\n"
            << "          Project github : " << project << "\n"
            << "          Welcome to join the conference at http://luotang.me "
            << "\n";
}

static void BuildLogo(void) {
  std::cout << "\n"
            << "                       .::::.                                  "
               "\n"
            << "                     .::::::::.                                "
               "\n"
            << "                    :::::::::::                                "
               "\n"
            << "                 ..:::::::::::'                                "
               "\n"
            << "               '::::::::::::'                                  "
               "\n"
            << "                .::::::::::                                    "
               "\n"
            << "           '::::::::::::::..                                   "
               "\n"
            << "                ..::::::::::::.                                "
               "\n"
            << "              ``::::::::::::::::                               "
               "\n"
            << "               ::::``:::::::::'        .:::.                   "
               "\n"
            << "              ::::'   ':::::'       .::::::::.                 "
               "\n"
            << "            .::::'      ::::     .:::::::'::::.                "
               "\n"
            << "           .:::'       :::::  .:::::::::' ':::::.              "
               "\n"
            << "          .::'        :::::.:::::::::'      ':::::.            "
               "\n"
            << "         .::'         ::::::::::::::'         ``::::.          "
               "\n"
            << "     ...:::           ::::::::::::'              ``::.         "
               "\n"
            << "    ```` ':.          ':::::::::'                  ::::..      "
               "\n"
            << "                       '.:::::'                    ':'````..   "
               "\n"
               "\n" << std::endl;
}

static void BuildInfo(void) {
  BuildProject();
  BuildLogo();
}

}  // namespace MSF
#endif