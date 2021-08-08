#ifndef BASE_CPU_H_
#define BASE_CPU_H_

#include <string>

namespace MSF {

class CPU {
 public:
  CPU() = delete;
  CPU(const CPU&) = delete;
  CPU(CPU&&) = delete;
  ~CPU() = delete;

  CPU& operator=(const CPU&) = delete;
  CPU& operator=(CPU&&) = delete;

  //! CPU architecture string
  static std::string Architecture();
  //! CPU affinity count
  static int Affinity();
  //! CPU logical cores count
  static int LogicalCores();
  //! CPU physical cores count
  static int PhysicalCores();
  //! CPU total cores count
  static std::pair<int, int> TotalCores();
  //! CPU clock speed in Hz
  static int64_t ClockSpeed();
  //! Is CPU Hyper-Threading enabled?
  static bool HyperThreading();
};

}  // namespace MSF
#endif