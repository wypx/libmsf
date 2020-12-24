#include "log_file.h"
#include <time.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <assert.h>

using namespace MSF;

LogFile::LogFile(const std::string& file_path_, int64_t rollSize,
                 bool threadSafe, int flushInterval, int checkEveryN)
    : file_path_(file_path_),
      flush_interval_(flushInterval),
      checkEveryN_(checkEveryN),
      roll_size_(rollSize),
      mutex_(threadSafe ? new std::mutex : NULL),
      file_(new AppendFile(file_path_)) {
  // assert(file_path_.find('/') == std::string::npos);
  rollFile();
}

LogFile::~LogFile() = default;

void LogFile::append(const char* logline, int len) {
  if (mutex_) {
    std::lock_guard<std::mutex> lock(*mutex_);
    append_unlocked(logline, len);
  } else {
    append_unlocked(logline, len);
  }
}

void LogFile::append_unlocked(const char* logline, int len) {
  file_->append(logline, len);
  if (file_->writtenBytes() > roll_size_) {
    rollFile();
  } else {
    ++count_;
    if (count_ >= checkEveryN_) {
      count_ = 0;
      time_t now = ::time(NULL);
      time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_;
      if (thisPeriod_ != startOfPeriod_) {
        rollFile();
      } else if (now - lastFlush_ > flush_interval_) {
        lastFlush_ = now;
        file_->flush();
      }
    }
  }
}

void LogFile::flush() {
  if (mutex_) {
    std::lock_guard<std::mutex> lock(*mutex_);
    file_->flush();
  } else {
    file_->flush();
  }
}

bool LogFile::rollFile() {
  time_t now = 0;
  std::string filename = GetLogFileName(basename_, &now);
  time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

  if (now > lastRoll_) {
    lastRoll_ = now;
    lastFlush_ = now;
    startOfPeriod_ = start;
    file_.reset(new AppendFile(filename));
    return true;
  }
  return false;
}

std::string LogFile::GetLogFileName(const std::string& basename, time_t* now) {
  std::string filename;
  filename.reserve(basename.size() + 64);
  filename = basename;

  char timebuf[32];
  struct tm tm;
  *now = time(NULL);
  gmtime_r(now, &tm);  // FIXME: localtime_r ?
  strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
  filename += timebuf;

  // filename += ProcessInfo::hostname();

  // char pidbuf[32];
  // snprintf(pidbuf, sizeof pidbuf, ".%d", ProcessInfo::pid());
  // filename += pidbuf;

  filename += ".log";

  return filename;
}
