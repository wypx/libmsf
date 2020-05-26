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

//https://www.cnblogs.com/oloroso/p/9365749.html
std::string GetVersion()
{
  std::string monthes[] =  
  {  
    "Jan",  
    "Feb",  
    "Mar",  
    "Apr",  
    "May",  
    "Jun",  
    "Jul",  
    "Aug",  
    "Sep",  
    "Oct",  
    "Nov",  
    "Dec",  
  };  
 
  std::string dateStr = __DATE__; 

  int year = atoi(dateStr.substr(dateStr.length() - 4).c_str());

  int month = 0;  
  for(int i = 0; i < sizeof(monthes); i++)  
  {  
    if(dateStr.find(monthes[i]) != std::string::npos)  
    {  
      month = i + 1;  
      break;  
    }  
  }  
 
  std::string dayStr = dateStr.substr(4, 2);
  int day = atoi(dayStr.c_str());

  std::string timeStr = __TIME__;  

  std::string hourStr = timeStr.substr(0, 2);
  int hour = atoi(hourStr.c_str());

  char version[20];
  sprintf(version, "1.0.%04d%02d%02d%02d", year, month, day, hour);

  return version;
}

void GetCompileTime(struct tm* lpCompileTime)
{
	char Mmm[4] = "Jan";
	sscanf(__DATE__, "%3s %d %d", Mmm,
				&lpCompileTime->tm_mday, &lpCompileTime->tm_year);
	lpCompileTime->tm_year -= 1900;

	switch (*(uint32_t*)Mmm) {
		case (uint32_t)('Jan'): lpCompileTime->tm_mon = 1; break;
		case (uint32_t)('Feb'): lpCompileTime->tm_mon = 2; break;
		case (uint32_t)('Mar'): lpCompileTime->tm_mon = 3; break;
		case (uint32_t)('Apr'): lpCompileTime->tm_mon = 4; break;
		case (uint32_t)('May'): lpCompileTime->tm_mon = 5; break;
		case (uint32_t)('Jun'): lpCompileTime->tm_mon = 6; break;
		case (uint32_t)('Jul'): lpCompileTime->tm_mon = 7; break;
		case (uint32_t)('Aug'): lpCompileTime->tm_mon = 8; break;
		case (uint32_t)('Sep'): lpCompileTime->tm_mon = 9; break;
		case (uint32_t)('Oct'): lpCompileTime->tm_mon = 10; break;
		case (uint32_t)('Nov'): lpCompileTime->tm_mon = 11; break;
		case (uint32_t)('Dec'): lpCompileTime->tm_mon = 12; break;
		default:lpCompileTime->tm_mon = 0;
	}
	sscanf(__TIME__, "%d:%d:%d", &lpCompileTime->tm_hour,
				&lpCompileTime->tm_min, &lpCompileTime->tm_sec);
	lpCompileTime->tm_isdst = lpCompileTime->tm_wday = lpCompileTime->tm_yday = 0;
}

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