#include "log_file.h"
#include <time.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <assert.h>

using namespace MSF;

#ifdef __APPLE__
#define fwrite_unlocked fwrite
#endif

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

LogFile::~LogFile() {
  flush();
};

void LogFile::append(const char* logline, int len) {
  if (mutex_) {
    std::lock_guard<std::mutex> lock(*mutex_);
    append_unlocked(logline, len);
  } else {
    append_unlocked(logline, len);
  }
}

void LogFile::appendBatch(const std::vector<T>& buffers) {
  constexpr size_t batch = 32;
  size_t len = buffers.size();

  size_t i = 0;
  for (; i + batch < len; i += batch) {
    file_->appendBatch(buffers.begin(), buffers.begin() + batch);
  }

  file_->appendBatch(buffers.begin() + i, buffers.end());

  afterAppend();
}

void LogFile::afterAppend() {
  if (file_->writtenBytes() > roll_size_) {
    rollFile();
  } else {
    if (count_ > kCheckTimeRoll_) {
      count_ = 0;
      time_t now = ::time(NULL);
      time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_;
      if (thisPeriod_ != startOfPeriod_) {
        rollFile();
      } else if (now - lastFlush_ > flush_interval_) {
        lastFlush_ = now;
        file_->flush();
      }
    } else {
      ++count_;
    }
  }
}

void LogFile::append_unlocked(const char* logline, int len) {
  file_->append(logline, len);
  afterAppend();
}

void LogFile::flush() {
  if (mutex_) {
    std::lock_guard<std::mutex> lock(*mutex_);
    file_->flush();
  } else {
    file_->flush();
  }
}

long FileUtil::GetAvailableSpace(const char* path) {
  struct statvfs stat;
  if (statvfs(path, &stat) != 0) {
    return -1;
  }
  // the available size is f_bsize * f_bavail
  return stat.f_bsize * stat.f_bavail;
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
    long available_size = GetAvailableSpace(filename.c_str());
    if (available_size < (300 << 20)) {  // 小于300M 不会再写文件
      forbit_write_ = true;
      fprintf(stderr,
              "filesystem available size less than 300M, forbit write\n");
    } else {
      forbit_write_ = false;
    }
    // 将STDOUT_FILENO和STDERR_FILENO也重定向到这个文件
    FILE* fp = file_->fp();
    if (dupStd_) {
      ::dup2(fileno(fp), STDOUT_FILENO);
      ::dup2(fileno(fp), STDERR_FILENO);
    }
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

  // std::string filename;
  // filename.reserve(basename.size() + 32);
  // filename = basename;

  // char timebuf[32];
  // char pidbuf[32];
  // struct tm tm;
  // *now = time(NULL);
  // gmtime_r(now, &tm); // FIXME: localtime_r ?
  // strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S", &tm);
  // filename += timebuf;
  // snprintf(pidbuf, sizeof pidbuf, ".%d", ::getpid()); // FIXME:
  // ProcessInfo::pid();
  // filename += pidbuf;
  // filename += ".log";

  return filename;
}
