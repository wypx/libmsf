// Only linux

#ifndef BASE_COROUTINE_H_
#define BASE_COROUTINE_H_

#include <ucontext.h>
// https://www.jianshu.com/p/4f7d3aa83088
// https://blog.csdn.net/qq910894904/article/details/41911175
// https://blog.csdn.net/daaikuaichuan/article/details/82951084
// /https://www.cnblogs.com/chenny7/p/4027852.html
// https://blog.csdn.net/qq_42381849/article/details/90341580
// https://blog.csdn.net/zzhongcy/article/details/89515037
// /https://blog.csdn.net/HiZhanYue/article/details/85112463

// https://github.com/loveyacper/ananas/tree/master/coroutine

#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace MSF {

using AnyPointer = std::shared_ptr<void>;

class Coroutine;
using CoroutinePtr = std::shared_ptr<Coroutine>;

class Coroutine {
  enum class State {
    Init,
    Running,
    Finish,
  };

 public:
  // works like python decorator: convert the func to a coroutine
  template <typename F, typename... Args>
  static CoroutinePtr CreateCoroutine(F&& f, Args&&... args) {
    return std::make_shared<Coroutine>(std::forward<F>(f),
                                       std::forward<Args>(args)...);
  }

  // Below three static functions for schedule coroutine

  // like python generator's send method
  static AnyPointer Send(const CoroutinePtr& crt,
                         AnyPointer = AnyPointer(nullptr));
  static AnyPointer Yield(const AnyPointer& = AnyPointer(nullptr));
  static AnyPointer Next(const CoroutinePtr& crt);

 public:
  // !!!
  // NEVER define coroutine object, please use CreateCoroutine.
  // Coroutine constructor should be private,
  // BUT compilers demand template constructor must be public...
  explicit Coroutine(std::size_t stackSize = 0);

  // if F return void
  template <typename F, typename... Args,
            typename = typename std::enable_if<
                std::is_void<typename std::result_of<F(Args...)>::type>::value,
                void>::type,
            typename Dummy = void>
  Coroutine(F&& f, Args&&... args)
      : Coroutine(kDefaultStackSize) {
    func_ = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
  }

  // if F return non-void
  template <typename F, typename... Args,
            typename = typename std::enable_if<
                !std::is_void<typename std::result_of<F(Args...)>::type>::value,
                void>::type>
  Coroutine(F&& f, Args&&... args)
      : Coroutine(kDefaultStackSize) {
    using ResultType = typename std::result_of<F(Args...)>::type;

    auto me = this;
    auto temp = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    func_ = [temp, me]() mutable {
      me->result_ = std::make_shared<ResultType>(temp());
    };
  }

  ~Coroutine();

  // no copyable
  Coroutine(const Coroutine&) = delete;
  void operator=(const Coroutine&) = delete;
  // no movable
  Coroutine(Coroutine&&) = delete;
  void operator=(Coroutine&&) = delete;

  unsigned int GetID() const { return id_; }
  static unsigned int GetCurrentID() { return current_->id_; }

 private:
  AnyPointer _Send(Coroutine* crt, AnyPointer = AnyPointer(nullptr));
  AnyPointer _Yield(const AnyPointer& = AnyPointer(nullptr));
  static void _Run(Coroutine* cxt);

  unsigned int id_;  // 1: main
  State state_;
  AnyPointer yieldValue_;

  typedef ucontext_t HANDLE;

  static const std::size_t kDefaultStackSize = 8 * 1024;
  std::vector<char> stack_;

  HANDLE handle_;
  std::function<void()> func_;
  AnyPointer result_;

  static Coroutine main_;
  static Coroutine* current_;
  static unsigned int sid_;
};
}  // namespace MSF
#endif