#include "console.h"

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <array>
#include <algorithm>

#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
#include <cstdio>
#elif defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

namespace MSF {

bool Console::IsInTerminal(FILE* file) noexcept {
#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
  bool const is_atty = ::_isatty(_fileno(file)) != 0;

  // ::GetConsoleMode() should return 0 if file is redirected or does not point
  // to the actual console
  DWORD console_mode;
  bool const is_console =
      ::GetConsoleMode(reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(file))),
                       &console_mode) != 0;

  return is_atty && is_console;
#else
  return ::isatty(fileno(file)) != 0;
#endif
}

bool Console::IsColorTerminal() {
#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
  // On Windows the TERM variable is usually not set, but the
  // console there does support colors.
  return 0 != _isatty(_fileno(stdout));
#else
  // On non-Windows platforms, we rely on the TERM variable. This list of
  // supported TERM values is copied from Google Test:
  // <https://github.com/google/googletest/blob/master/googletest/src/gtest.cc#L2925>.
  // Get term from env
  auto* env_ptr = ::getenv("TERM");

  if (env_ptr == nullptr) {
    return false;
  }

  static constexpr std::array<char const*, 24> supported_terms = {
      {"xterm",                 "xterm-color", "xterm-256color", "screen",
       "screen-256color",       "tmux",        "tmux-256color",  "rxvt-unicode",
       "rxvt-unicode-256color", "linux",       "cygwin",         "ansi",
       "color",                 "console",     "gnome",          "konsole",
       "kterm",                 "msys",        "putty",          "rxvt",
       "vt100"}};

  return IsInTerminal(stdout) &&
         (std::any_of(supported_terms.begin(), supported_terms.end(),
                      [&](char const* term) {
           return ::strstr(env_ptr, term) != nullptr;
         }));
#endif
}

void Console::SetColor(Color color, Color background) {
#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
  const char* colors[] = {"\033[22;30m",  // Black color
                          "\033[22;34m",  // Blue color
                          "\033[22;32m",  // Green color
                          "\033[22;36m",  // Cyan color
                          "\033[22;31m",  // Red color
                          "\033[22;35m",  // Magenta color
                          "\033[22;33m",  // Brown color
                          "\033[22;37m",  // Grey color
                          "\033[01;30m",  // Dark grey color
                          "\033[01;34m",  // Light blue color
                          "\033[01;32m",  // Light green color
                          "\033[01;36m",  // Light cyan color
                          "\033[01;31m",  // Light red color
                          "\033[01;35m",  // Light magenta color
                          "\033[01;33m",  // Yellow color
                          "\033[01;37m"   // White color
  };
  const char* backgrounds[] = {"\033[00000m",  // Black color
                               "\033[02;44m",  // Blue color
                               "\033[02;42m",  // Green color
                               "\033[02;46m",  // Cyan color
                               "\033[02;41m",  // Red color
                               "\033[02;45m",  // Magenta color
                               "\033[02;43m",  // Brown color
                               "\033[02;47m",  // Grey color
                               "\033[00;40m",  // Dark grey color
                               "\033[00;44m",  // Light blue color
                               "\033[00;42m",  // Light green color
                               "\033[00;46m",  // Light cyan color
                               "\033[00;41m",  // Light red color
                               "\033[00;45m",  // Light magenta color
                               "\033[00;43m",  // Yellow color
                               "\033[00;47m"   // White color
  };
  std::fwrite(backgrounds[(int)background - (int)Color::BLACK], 1, 8, stdout);
  std::fwrite(colors[(int)color - (int)Color::BLACK], 1, 8, stdout);
#elif defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
  const HANDLE stdout_handle = ::GetStdHandle(STD_OUTPUT_HANDLE);

  // Gets the current text color.
  CONSOLE_SCREEN_BUFFER_INFO buffer_info;
  GetConsoleScreenBufferInfo(stdout_handle, &buffer_info);
  const WORD old_color_attrs = buffer_info.wAttributes;

  ::SetConsoleTextAttribute(
      hConsole, (((WORD)color) & 0x0F) + ((((WORD)background) & 0x0F) << 4));

  // We need to flush the stream buffers into the console before each
  // SetConsoleTextAttribute call lest it affect the text that is already
  // printed but has not yet reached the console.
  ::fflush(stdout);
  // ::SetConsoleTextAttribute(stdout_handle,
  //                         GetPlatformColorCode(color) |
  // FOREGROUND_INTENSITY);
  ::SetConsoleTextAttribute(
      stdout_handle,
      (((WORD)color) & 0x0F) + ((((WORD)background) & 0x0F) << 4));
  // ::vprintf(fmt, args);

  ::fflush(stdout);
  // Restores the text color.
  ::SetConsoleTextAttribute(stdout_handle, old_color_attrs);
#endif
}

}  // namespace MSF
