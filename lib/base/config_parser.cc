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

#include "config_parser.h"

#include <base/logging.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fstream>
#include <iostream>

using namespace MSF;

namespace MSF {

// 构造函数，会初始化注释字符集合flags_（容器），目前只使用#和;作为注释前缀
ConfigParser::ConfigParser() : commentDelimiter("#") {}

//解析一行数据，得到键值
bool ConfigParser::parse(const std::string &content, std::string *key,
                         std::string *value) {
  return split(content, "=", key, value);
}

int ConfigParser::UpdateSection(const std::string &cleanLine,
                                const std::string &comment,
                                const std::string &rightComment,
                                IniSection **section) {
  IniSection *newSection;
  // 查找右中括号
  size_t index = cleanLine.find_first_of(']');
  if (index == std::string::npos) {
    LOG(ERROR) << "No matched ] found.";
    return ERR_UNMATCHED_BRACKETS;
  }

  int len = index - 1;
  // 若段名为空，继续下一行
  if (len <= 0) {
    LOG(ERROR) << "Section name is empty";
    return ERR_SECTION_EMPTY;
  }

  // 取段名
  std::string s(cleanLine, 1, len);

  trim(s);

  //检查段是否已存在
  if (getSection(s) != NULL) {
    LOG(ERROR) << "Section " << s << "already exist";
    return ERR_SECTION_ALREADY_EXISTS;
  }

  // 申请一个新段，由于map容器会自动排序，打乱原有顺序，因此改用vector存储（sections_vt）
  newSection = new IniSection();
  // 填充段名
  newSection->name = s;
  // 填充段开头的注释
  newSection->comment = comment;
  newSection->rightComment = rightComment;

  sections_vt.push_back(newSection);

  *section = newSection;

  return 0;
}

int ConfigParser::AddKeyValuePair(const std::string &cleanLine,
                                  const std::string &comment,
                                  const std::string &rightComment,
                                  IniSection *section) {
  std::string key, value;

  if (!parse(cleanLine, &key, &value)) {
    LOG(ERROR) << "Parse line failed: " << cleanLine;
    return ERR_PARSE_KEY_VALUE_FAILED;
  }

  IniItem item;
  item.key = key;
  item.value = value;
  item.comment = comment;
  item.rightComment = rightComment;

  section->items.push_back(item);

  return 0;
}

int ConfigParser::Load(const std::string &filePath) {
  int err;
  std::string line;       // 带注释的行
  std::string cleanLine;  // 去掉注释后的行
  std::string comment;
  std::string rightComment;
  IniSection *currSection = NULL;  // 初始化一个字段指针

  release();

  ConfigParserPath = filePath;
  std::ifstream ifs(ConfigParserPath);
  if (!ifs.is_open()) {
    LOG(ERROR) << "Failed to load config file: " << ConfigParserPath;
    return ERR_OPEN_FILE_FAILED;
  }

  //增加默认段，即 无名段""
  currSection = new IniSection();
  currSection->name = "";
  sections_vt.push_back(currSection);

  // 每次读取一行内容到line
  while (std::getline(ifs, line)) {
    trim(line);

    // step
    // 0，空行处理，如果长度为0，说明是空行，添加到comment，当作是注释的一部分
    if (line.length() <= 0) {
      comment += delim;
      continue;
    }

    // step 1
    // 如果行首不是注释，查找行尾是否存在注释
    // 如果该行以注释开头，添加到comment，跳过当前循环，continue
    if (IsCommentLine(line)) {
      comment += line + delim;
      continue;
    }

    // 如果行首不是注释，查找行尾是否存在注释，若存在，切割该行，将注释内容添加到rightComment
    split(line, commentDelimiter, &cleanLine, &rightComment);

    // step 2，判断line内容是否为段或键
    //段开头查找 [
    if (cleanLine[0] == '[') {
      err = UpdateSection(cleanLine, comment, rightComment, &currSection);
    } else {
      // 如果该行是键值，添加到section段的items容器
      err = AddKeyValuePair(cleanLine, comment, rightComment, currSection);
    }

    if (err != 0) {
      ifs.close();
      LOG(ERROR) << "Failed to load config file: " << ConfigParserPath;
      return err;
    }

    // comment清零
    comment = "";
    rightComment = "";
  }

  ifs.close();

  return 0;
}

int ConfigParser::Save() { return SaveAs(ConfigParserPath); }

int ConfigParser::SaveAs(const std::string &filePath) {
  std::string data = "";

  /* 载入section数据 */
  for (IniSection_it sect = sections_vt.begin(); sect != sections_vt.end();
       ++sect) {
    if ((*sect)->comment != "") {
      data += (*sect)->comment;
    }

    if ((*sect)->name != "") {
      data += std::string("[") + (*sect)->name + std::string("]");
      data += delim;
    }

    if ((*sect)->rightComment != "") {
      data += " " + commentDelimiter + (*sect)->rightComment;
    }

    /* 载入item数据 */
    for (IniSection::IniItem_it item = (*sect)->items.begin();
         item != (*sect)->items.end(); ++item) {
      if (item->comment != "") {
        data += item->comment;
        if (data[data.length() - 1] != '\n') {
          data += delim;
        }
      }

      data += item->key + "=" + item->value;

      if (item->rightComment != "") {
        data += " " + commentDelimiter + item->rightComment;
      }

      if (data[data.length() - 1] != '\n') {
        data += delim;
      }
    }
  }

  std::ofstream ofs(filePath);
  ofs << data;
  ofs.close();
  return 0;
}

IniSection *ConfigParser::getSection(const std::string &section /*=""*/) {
  for (IniSection_it it = sections_vt.begin(); it != sections_vt.end(); ++it) {
    if ((*it)->name == section) {
      return *it;
    }
  }

  return NULL;
}

int ConfigParser::GetSections(std::vector<std::string> *sections) {
  for (IniSection_it it = sections_vt.begin(); it != sections_vt.end(); ++it) {
    sections->push_back((*it)->name);
  }

  return sections->size();
}

int ConfigParser::GetSectionNum() { return sections_vt.size(); }

int ConfigParser::GetStringValue(const std::string &section,
                                 const std::string &key, std::string *value) {
  return getValue(section, key, value);
}

int ConfigParser::GetIntValue(const std::string &section,
                              const std::string &key, int *intValue) {
  int err;
  std::string strValue;

  err = getValue(section, key, &strValue);

  *intValue = atoi(strValue.c_str());

  return err;
}

int ConfigParser::GetDoubleValue(const std::string &section,
                                 const std::string &key, double *value) {
  int err;
  std::string strValue;

  err = getValue(section, key, &strValue);

  *value = atof(strValue.c_str());

  return err;
}

int ConfigParser::GetBoolValue(const std::string &section,
                               const std::string &key, bool *value) {
  int err;
  std::string strValue;

  err = getValue(section, key, &strValue);

  if (StringCmpIgnoreCase(strValue, "true") ||
      StringCmpIgnoreCase(strValue, "1")) {
    *value = true;
  } else if (StringCmpIgnoreCase(strValue, "false") ||
             StringCmpIgnoreCase(strValue, "0")) {
    *value = false;
  }

  return err;
}

/* 获取section段第一个键为key的std::string值，成功返回获取的值，否则返回默认值
 */
void ConfigParser::GetStringValueOrDefault(const std::string &section,
                                           const std::string &key,
                                           std::string *value,
                                           const std::string &defaultValue) {
  if (GetStringValue(section, key, value) != 0) {
    *value = defaultValue;
  }

  return;
}

/* 获取section段第一个键为key的int值，成功返回获取的值，否则返回默认值 */
void ConfigParser::GetIntValueOrDefault(const std::string &section,
                                        const std::string &key, int *value,
                                        int defaultValue) {
  if (GetIntValue(section, key, value) != 0) {
    *value = defaultValue;
  }

  return;
}

/* 获取section段第一个键为key的double值，成功返回获取的值，否则返回默认值 */
void ConfigParser::GetDoubleValueOrDefault(const std::string &section,
                                           const std::string &key,
                                           double *value, double defaultValue) {
  if (GetDoubleValue(section, key, value) != 0) {
    *value = defaultValue;
  }

  return;
}

/* 获取section段第一个键为key的bool值，成功返回获取的值，否则返回默认值 */
void ConfigParser::GetBoolValueOrDefault(const std::string &section,
                                         const std::string &key, bool *value,
                                         bool defaultValue) {
  if (GetBoolValue(section, key, value) != 0) {
    *value = defaultValue;
  }

  return;
}

/* 获取注释，如果key=""则获取段注释 */
int ConfigParser::GetComment(const std::string &section, const std::string &key,
                             std::string *comment) {
  IniSection *sect = getSection(section);

  if (sect == NULL) {
    errMsg = std::string("not find the section ") + section;
    return ERR_NOT_FOUND_SECTION;
  }

  if (key == "") {
    *comment = sect->comment;
    return RET_OK;
  }

  for (IniSection::IniItem_it it = sect->begin(); it != sect->end(); ++it) {
    if (it->key == key) {
      *comment = it->comment;
      return RET_OK;
    }
  }

  errMsg = std::string("not find the key ") + section;
  return ERR_NOT_FOUND_KEY;
}

/* 获取行尾注释，如果key=""则获取段的行尾注释 */
int ConfigParser::GetRightComment(const std::string &section,
                                  const std::string &key,
                                  std::string *rightComment) {
  IniSection *sect = getSection(section);

  if (sect == NULL) {
    errMsg = std::string("not find the section ") + section;
    return ERR_NOT_FOUND_SECTION;
  }

  if (key == "") {
    *rightComment = sect->rightComment;
    return RET_OK;
  }

  for (IniSection::IniItem_it it = sect->begin(); it != sect->end(); ++it) {
    if (it->key == key) {
      *rightComment = it->rightComment;
      return RET_OK;
    }
  }
  LOG(ERROR) << "Not find the key: " << key;
  return ERR_NOT_FOUND_KEY;
}

int ConfigParser::getValue(const std::string &section, const std::string &key,
                           std::string *value) {
  std::string comment;
  return getValue(section, key, value, &comment);
}

int ConfigParser::getValue(const std::string &section, const std::string &key,
                           std::string *value, std::string *comment) {
  IniSection *sect = getSection(section);

  if (sect == NULL) {
    LOG(ERROR) << "Not find the section: " << section;
    return ERR_NOT_FOUND_SECTION;
  }

  for (IniSection::IniItem_it it = sect->begin(); it != sect->end(); ++it) {
    if (it->key == key) {
      *value = it->value;
      *comment = it->comment;
      return RET_OK;
    }
  }

  LOG(ERROR) << "Not find the key: " << key;
  return ERR_NOT_FOUND_KEY;
}

int ConfigParser::GetValues(const std::string &section, const std::string &key,
                            std::vector<std::string> *values) {
  std::vector<std::string> comments;
  return GetValues(section, key, values, &comments);
}

int ConfigParser::GetValues(const std::string &section, const std::string &key,
                            std::vector<std::string> *values,
                            std::vector<std::string> *comments) {
  std::string value, comment;

  values->clear();
  comments->clear();

  IniSection *sect = getSection(section);

  if (sect == NULL) {
    LOG(ERROR) << "Not find the section: " << section;
    return ERR_NOT_FOUND_SECTION;
  }

  for (IniSection::IniItem_it it = sect->begin(); it != sect->end(); ++it) {
    if (it->key == key) {
      value = it->value;
      comment = it->comment;

      values->push_back(value);
      comments->push_back(comment);
    }
  }

  if (values->size() == 0) {
    LOG(ERROR) << "Not find the key: " << key;
    return ERR_NOT_FOUND_KEY;
  }

  return RET_OK;
}

bool ConfigParser::HasSection(const std::string &section) {
  return (getSection(section) != NULL);
}

bool ConfigParser::HasKey(const std::string &section, const std::string &key) {
  IniSection *sect = getSection(section);

  if (sect != NULL) {
    for (IniSection::IniItem_it it = sect->begin(); it != sect->end(); ++it) {
      if (it->key == key) {
        return true;
      }
    }
  }

  return false;
}

int ConfigParser::setValue(const std::string &section, const std::string &key,
                           const std::string &value,
                           const std::string &comment /*=""*/) {
  IniSection *sect = getSection(section);

  std::string comt = comment;

  if (comt != "") {
    comt = commentDelimiter + comt;
  }

  if (sect == NULL) {
    //如果段不存在，新建一个
    sect = new IniSection();
    if (sect == NULL) {
      LOG(ERROR) << "No enough memory.";
      return ERR_NO_ENOUGH_MEMORY;
    }

    sect->name = section;
    if (sect->name == "") {
      // 确保空section在第一个
      sections_vt.insert(sections_vt.begin(), sect);
    } else {
      sections_vt.push_back(sect);
    }
  }

  for (IniSection::IniItem_it it = sect->begin(); it != sect->end(); ++it) {
    if (it->key == key) {
      it->value = value;
      it->comment = comt;
      return RET_OK;
    }
  }

  // not found key
  IniItem item;
  item.key = key;
  item.value = value;
  item.comment = comt;

  sect->items.push_back(item);

  return RET_OK;
}

int ConfigParser::SetStringValue(const std::string &section,
                                 const std::string &key,
                                 const std::string &value) {
  return setValue(section, key, value);
}

int ConfigParser::SetIntValue(const std::string &section,
                              const std::string &key, int value) {
  char buf[64] = {0};
  snprintf(buf, sizeof(buf), "%d", value);
  return setValue(section, key, buf);
}

int ConfigParser::SetDoubleValue(const std::string &section,
                                 const std::string &key, double value) {
  char buf[64] = {0};
  snprintf(buf, sizeof(buf), "%f", value);
  return setValue(section, key, buf);
}

int ConfigParser::SetBoolValue(const std::string &section,
                               const std::string &key, bool value) {
  if (value) {
    return setValue(section, key, "true");
  } else {
    return setValue(section, key, "false");
  }
}

int ConfigParser::SetComment(const std::string &section, const std::string &key,
                             const std::string &comment) {
  IniSection *sect = getSection(section);

  if (sect == NULL) {
    LOG(ERROR) << "Not find the section: " << section;
    return ERR_NOT_FOUND_SECTION;
  }

  if (key == "") {
    sect->comment = comment;
    return RET_OK;
  }

  for (IniSection::IniItem_it it = sect->begin(); it != sect->end(); ++it) {
    if (it->key == key) {
      it->comment = comment;
      return RET_OK;
    }
  }

  LOG(ERROR) << "Not find the key: " << key;
  return ERR_NOT_FOUND_KEY;
}

int ConfigParser::SetRightComment(const std::string &section,
                                  const std::string &key,
                                  const std::string &rightComment) {
  IniSection *sect = getSection(section);

  if (sect == NULL) {
    LOG(ERROR) << "Not find the section: " << section;
    return ERR_NOT_FOUND_SECTION;
  }

  if (key == "") {
    sect->rightComment = rightComment;
    return RET_OK;
  }

  for (IniSection::IniItem_it it = sect->begin(); it != sect->end(); ++it) {
    if (it->key == key) {
      it->rightComment = rightComment;
      return RET_OK;
    }
  }

  LOG(ERROR) << "Not find the key: " << key;
  return ERR_NOT_FOUND_KEY;
}

void ConfigParser::SetCommentDelimiter(const std::string &delimiter) {
  commentDelimiter = delimiter;
}

void ConfigParser::DeleteSection(const std::string &section) {
  for (IniSection_it it = sections_vt.begin(); it != sections_vt.end();) {
    if ((*it)->name == section) {
      delete *it;
      it = sections_vt.erase(it);
      break;
    } else {
      it++;
    }
  }
}

void ConfigParser::DeleteKey(const std::string &section,
                             const std::string &key) {
  IniSection *sect = getSection(section);

  if (sect != NULL) {
    for (IniSection::IniItem_it it = sect->begin(); it != sect->end();) {
      if (it->key == key) {
        it = sect->items.erase(it);
        break;
      } else {
        it++;
      }
    }
  }
}

/*-------------------------------------------------------------------------*/
/**
  @brief    release: 释放全部资源，清空容器
  @param    none
  @return   none
 */
/*--------------------------------------------------------------------------*/
void ConfigParser::release() {
  ConfigParserPath = "";

  for (IniSection_it it = sections_vt.begin(); it != sections_vt.end(); ++it) {
    delete (*it);  // 清除section
  }

  sections_vt.clear();
}

/*-------------------------------------------------------------------------*/
/**
  @brief    判断是否是注释
  @param    str 一个std::string变量
  @return   如果是注释则为真
 */
/*--------------------------------------------------------------------------*/
bool ConfigParser::IsCommentLine(const std::string &str) {
  return StartWith(str, commentDelimiter);
}

/*-------------------------------------------------------------------------*/
/**
  @brief    print for debug
  @param    none
  @return   none
 */
/*--------------------------------------------------------------------------*/
void ConfigParser::debug() {
  // LOG(DEBUG) << "############ print start ############";
  // LOG(DEBUG) << "filePath: " << ConfigParserPath;
  // LOG(DEBUG) << "commentDelimiter:" << commentDelimiter;

  for (IniSection_it it = sections_vt.begin(); it != sections_vt.end(); ++it) {
    // LOG(DEBUG) << "comment : " << (*it)->comment;
    // LOG(DEBUG) << "section : " << (*it)->name;
    if ((*it)->rightComment != "") {
      // LOG(DEBUG) << "rightComment:" << (*it)->rightComment;
    }

    for (IniSection::IniItem_it i = (*it)->items.begin();
         i != (*it)->items.end(); ++i) {
      // LOG(DEBUG) << "    comment : " << i->comment;
      // LOG(DEBUG) << "    parm    :%" << i->key.c_str() << "="
      //           << i->value;
      if (i->rightComment != "") {
        // LOG(DEBUG) << "    rcomment: " << i->rightComment;
      }
    }
  }

  // LOG(DEBUG) << "############ print end ############";
  return;
}

const std::string &ConfigParser::GetErrMsg() { return errMsg; }

bool ConfigParser::StartWith(const std::string &str,
                             const std::string &prefix) {
  if (::strncmp(str.c_str(), prefix.c_str(), prefix.size()) == 0) {
    return true;
  }

  return false;
}

void ConfigParser::trimleft(std::string &str, char c /*=' '*/) {
  int len = str.length();

  int i = 0;

  while (str[i] == c && str[i] != '\0') {
    i++;
  }

  if (i != 0) {
    str = std::string(str, i, len - i);
  }
}

void ConfigParser::trimright(std::string &str, char c /*=' '*/) {
  int i = 0;
  int len = str.length();

  for (i = len - 1; i >= 0; --i) {
    if (str[i] != c) {
      break;
    }
  }

  str = std::string(str, 0, i + 1);
}

/*-------------------------------------------------------------------------*/
/**
  @brief    trim，整理一行字符串，去掉首尾空格
  @param    str std::string变量
 */
/*--------------------------------------------------------------------------*/
void ConfigParser::trim(std::string &str) {
  int len = str.length();

  int i = 0;

  while ((i < len) && isspace(str[i]) && (str[i] != '\0')) {
    i++;
  }

  if (i != 0) {
    str = std::string(str, i, len - i);
  }

  len = str.length();

  for (i = len - 1; i >= 0; --i) {
    if (!isspace(str[i])) {
      break;
    }
  }

  str = std::string(str, 0, i + 1);
}

/*-------------------------------------------------------------------------*/
/**
  @brief    split，用分隔符切割字符串
  @param    str 输入字符串
  @param    left_str 分隔后得到的左字符串
  @param    right_str 分隔后得到的右字符串（不包含seperator）
  @param    seperator 分隔符
  @return   pos
 */
/*--------------------------------------------------------------------------*/
bool ConfigParser::split(const std::string &str, const std::string &sep,
                         std::string *pleft, std::string *pright) {
  size_t pos = str.find(sep);
  std::string left, right;

  if (pos != std::string::npos) {
    left = std::string(str, 0, pos);
    right = std::string(str, pos + 1);

    trim(left);
    trim(right);

    *pleft = left;
    *pright = right;
    return true;
  } else {
    left = str;
    right = "";

    trim(left);

    *pleft = left;
    *pright = right;
    return false;
  }
}

bool ConfigParser::StringCmpIgnoreCase(const std::string &str1,
                                       const std::string &str2) {
  std::string a = str1;
  std::string b = str2;
  std::transform(a.begin(), a.end(), a.begin(), towupper);
  std::transform(b.begin(), b.end(), b.begin(), towupper);

  return (a == b);
}
}  // namespace MSF