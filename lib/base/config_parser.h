/* Copyright (c) 2014-2019 WinnerHust
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * The names of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef BASE_CONFIG_PARSER_H_
#define BASE_CONFIG_PARSER_H_

/* libiray from https://github.com/Winnerhust/ConfigParser2 */

#include <algorithm>
#include <string>
#include <vector>

namespace MSF {

#define RET_OK 0
// 没有找到匹配的]
#define ERR_UNMATCHED_BRACKETS 2
// 段为空
#define ERR_SECTION_EMPTY 3
// 段名已经存在
#define ERR_SECTION_ALREADY_EXISTS 4
// 解析key value失败
#define ERR_PARSE_KEY_VALUE_FAILED 5
// 打开文件失败
#define ERR_OPEN_FILE_FAILED 6
// 内存不足
#define ERR_NO_ENOUGH_MEMORY 7
// 没有找到对应的key
#define ERR_NOT_FOUND_KEY 8
// 没有找到对应的section
#define ERR_NOT_FOUND_SECTION 9

const char delim[] = "\n";
struct IniItem {
  std::string key;
  std::string value;
  std::string comment;  // 每个键的注释，都是指该行上方的内容
  std::string rightComment;
};

struct IniSection {
  typedef std::vector<IniItem>::iterator IniItem_it;
  IniItem_it begin() {
    return items.begin();  // 段结构体的begin元素指向items的头指针
  }

  IniItem_it end() {
    return items.end();  // 段结构体的begin元素指向items的尾指针
  }

  std::string name;
  std::string comment;  // 每个段的注释，都是指该行上方的内容
  std::string rightComment;
  std::vector<IniItem>
      items;  // 键值项数组，一个段可以有多个键值，所有用vector来储存
};

class ConfigParser {
 public:
  ConfigParser();
  ~ConfigParser() { release(); }

 public:
  // for debug
  void debug();
  /* 打开并解析一个名为fname的INI文件 */
  int Load(const std::string &fname);
  /* 将内容保存到当前文件 */
  int Save();
  /* 将内容另存到一个名为fname的文件 */
  int SaveAs(const std::string &fname);

  /* 获取section段第一个键为key的std::string值，成功返回0，否则返回错误码 */
  int GetStringValue(const std::string &section, const std::string &key,
                     std::string *value);
  /* 获取section段第一个键为key的int值，成功返回0，否则返回错误码 */
  int GetIntValue(const std::string &section, const std::string &key,
                  int *value);
  /* 获取section段第一个键为key的double值，成功返回0，否则返回错误码 */
  int GetDoubleValue(const std::string &section, const std::string &key,
                     double *value);
  /* 获取section段第一个键为key的bool值，成功返回0，否则返回错误码 */
  int GetBoolValue(const std::string &section, const std::string &key,
                   bool *value);
  /* 获取注释，如果key=""则获取段注释 */
  int GetComment(const std::string &section, const std::string &key,
                 std::string *comment);
  /* 获取行尾注释，如果key=""则获取段的行尾注释 */
  int GetRightComment(const std::string &section, const std::string &key,
                      std::string *rightComment);

  /* 获取section段第一个键为key的std::string值，成功返回获取的值，否则返回默认值
   */
  void GetStringValueOrDefault(const std::string &section,
                               const std::string &key, std::string *value,
                               const std::string &defaultValue);
  /* 获取section段第一个键为key的int值，成功返回获取的值，否则返回默认值 */
  void GetIntValueOrDefault(const std::string &section, const std::string &key,
                            int *value, int defaultValue);
  /* 获取section段第一个键为key的double值，成功返回获取的值，否则返回默认值 */
  void GetDoubleValueOrDefault(const std::string &section,
                               const std::string &key, double *value,
                               double defaultValue);
  /* 获取section段第一个键为key的bool值，成功返回获取的值，否则返回默认值 */
  void GetBoolValueOrDefault(const std::string &section, const std::string &key,
                             bool *value, bool defaultValue);

  /* 获取section段所有键为key的值,并将值赋到values的vector中 */
  int GetValues(const std::string &section, const std::string &key,
                std::vector<std::string> *values);

  /* 获取section列表,并返回section个数 */
  int GetSections(std::vector<std::string> *sections);
  /* 获取section个数，至少存在一个空section */
  int GetSectionNum();
  /* 是否存在某个section */
  bool HasSection(const std::string &section);
  /* 获取指定section的所有ken=value信息 */
  IniSection *getSection(const std::string &section = "");

  /* 是否存在某个key */
  bool HasKey(const std::string &section, const std::string &key);

  /* 设置字符串值 */
  int SetStringValue(const std::string &section, const std::string &key,
                     const std::string &value);
  /* 设置整形值 */
  int SetIntValue(const std::string &section, const std::string &key,
                  int value);
  /* 设置浮点数值 */
  int SetDoubleValue(const std::string &section, const std::string &key,
                     double value);
  /* 设置布尔值 */
  int SetBoolValue(const std::string &section, const std::string &key,
                   bool value);
  /* 设置注释，如果key=""则设置段注释 */
  int SetComment(const std::string &section, const std::string &key,
                 const std::string &comment);
  /* 设置行尾注释，如果key=""则设置段的行尾注释 */
  int SetRightComment(const std::string &section, const std::string &key,
                      const std::string &rightComment);

  /* 删除段 */
  void DeleteSection(const std::string &section);
  /* 删除特定段的特定参数 */
  void DeleteKey(const std::string &section, const std::string &key);
  /*设置注释分隔符，默认为"#" */
  void SetCommentDelimiter(const std::string &delimiter);

  const std::string &GetErrMsg();

 private:
  /* 获取section段所有键为key的值,并将值赋到values的vector中,,将注释赋到comments的vector中
   */
  int GetValues(const std::string &section, const std::string &key,
                std::vector<std::string> *value,
                std::vector<std::string> *comments);

  /* 同时设置值和注释 */
  int setValue(const std::string &section, const std::string &key,
               const std::string &value, const std::string &comment = "");

  /* 去掉str前面的c字符 */
  static void trimleft(std::string &str, char c = ' ');
  /* 去掉str后面的c字符 */
  static void trimright(std::string &str, char c = ' ');
  /* 去掉str前面和后面的空格符,Tab符等空白符 */
  static void trim(std::string &str);
  /* 将字符串str按分割符delim分割成多个子串 */
 private:
  int UpdateSection(const std::string &cleanLine, const std::string &comment,
                    const std::string &rightComment, IniSection **section);
  int AddKeyValuePair(const std::string &cleanLine, const std::string &comment,
                      const std::string &rightComment, IniSection *section);

  void release();
  bool split(const std::string &str, const std::string &sep, std::string *left,
             std::string *right);
  bool IsCommentLine(const std::string &str);
  bool parse(const std::string &content, std::string *key, std::string *value);

 private:
  /* 获取section段第一个键为key的值,并将值赋到value中 */
  int getValue(const std::string &section, const std::string &key,
               std::string *value);
  /* 获取section段第一个键为key的值,并将值赋到value中,将注释赋到comment中 */
  int getValue(const std::string &section, const std::string &key,
               std::string *value, std::string *comment);

  bool StringCmpIgnoreCase(const std::string &str1, const std::string &str2);
  bool StartWith(const std::string &str, const std::string &prefix);

 private:
  typedef std::vector<IniSection *>::iterator IniSection_it;
  std::vector<IniSection *> sections_vt;  // 用于存储段集合的vector容器
  std::string ConfigParserPath;
  std::string commentDelimiter;
  std::string errMsg;  // 保存出错时的错误信息
};

}  // namespace MSF

#endif
