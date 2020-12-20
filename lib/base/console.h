
#ifndef BASE_DATE_H_
#define BASE_DATE_H_

#include "Noncopyable.h"

namespace MSF {

//! Supported console colors
enum class Color {
  BLACK,         //!< Black color
  BLUE,          //!< Blue color
  GREEN,         //!< Green color
  CYAN,          //!< Cyan color
  RED,           //!< Red color
  MAGENTA,       //!< Magenta color
  BROWN,         //!< Brown color
  GREY,          //!< Grey color
  DARKGREY,      //!< Dark grey color
  LIGHTBLUE,     //!< Light blue color
  LIGHTGREEN,    //!< Light green color
  LIGHTCYAN,     //!< Light cyan color
  LIGHTRED,      //!< Light red color
  LIGHTMAGENTA,  //!< Light magenta color
  YELLOW,        //!< Yellow color
  WHITE          //!< White color
};

//! Console management static class
/*!
    Provides console management functionality such as setting
    text and background colors.

    Thread-safe.
*/
class Console {
 public:
  Console() = delete;
  Console(const Console&) = delete;
  Console(Console&&) = delete;
  ~Console() = delete;

  Console& operator=(const Console&) = delete;
  Console& operator=(Console&&) = delete;

  //! Set console text color
  /*!
      \param color - Console text color
      \param background - Console background color (default is Color::BLACK)
  */
  static void SetColor(Color color, Color background = Color::BLACK);
};

}  // namespace MSF

#endif