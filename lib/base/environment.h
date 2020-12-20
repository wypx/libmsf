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
#ifndef BASE_ENV_H_
#define BASE_ENV_H_

#include <stdlib.h>
#include <string>
#include <memory>

namespace MSF {

struct ScopedSet {
  ScopedSet(const ScopedSet&) = delete;
  ScopedSet& operator=(const ScopedSet&) = delete;

  ScopedSet(ScopedSet&&) = default;
  ScopedSet& operator=(ScopedSet&&) = default;

 public:
  /** Default ctor (NOOP). */
  ScopedSet() {}

  /** Set \a var_r to \a val_r (unsets \a var_r if \a val_r is a \c nullptr). */
  ScopedSet(std::string var_r, const char* val_r) : var_{std::move(var_r)} {
    if (!var_.empty()) {
      if (const char* orig = ::getenv(var_.c_str()))
        val_.reset(new std::string(orig));
      setval(val_r);
    }
  }

  /** Restore the original setting. */
  ~ScopedSet() {
    if (!var_.empty()) setval(val_ ? val_->c_str() : nullptr);
  }

 private:
  void setval(const char* val_r) {
    if (val_r)
      ::setenv(var_.c_str(), val_r, 1);
    else
      ::unsetenv(var_.c_str());
  }

 private:
  std::string var_;
  std::unique_ptr<std::string> val_;
};

}  // namespace MSF
#endif
