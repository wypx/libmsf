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
#ifndef BASE_NOCPYABLE_H_
#define BASE_NOCPYABLE_H_

namespace MSF {

/**
 * 在C++11中,如果想要禁止类的拷贝行为只需要把相应的函数设为 =
 * delete即可,参见标准库的std::unique_ptr 设计思想是让子类继承,
 * 但是阻止子类调用赋值和copy构造函数
 * http://www.cppblog.com/luke/archive/2009/03/13/76411.html
 * https://blog.csdn.net/littleflypig/article/details/88799617
 * https://blog.csdn.net/zzhongcy/article/details/85165242
 * https://blog.csdn.net/weixin_30586257/article/details/98313444
 * C++容器与noncopyable
 * https://blog.csdn.net/zhangxiao93/article/details/72651711
 * */
class Noncopyable {
 protected:
  Noncopyable() = default;
  ~Noncopyable() = default;

 private:
  Noncopyable(const Noncopyable &) = delete;
  void operator=(const Noncopyable &) = delete;
};

class Copyable {
 protected:
  Copyable() = default;
  ~Copyable() = default;
};

}  // namespace MSF
#endif