
std::string SystemInspector::loadavg(HttpRequest::Method, const Inspector::ArgList&)
{
  string loadavg;
  FileUtil::readFile("/proc/loadavg", 65536, &loadavg);
  return loadavg;
}

std::string SystemInspector::version(HttpRequest::Method, const Inspector::ArgList&)
{
  string version;
  FileUtil::readFile("/proc/version", 65536, &version);
  return version;
}

std::string SystemInspector::cpuinfo(HttpRequest::Method, const Inspector::ArgList&)
{
  string cpuinfo;
  FileUtil::readFile("/proc/cpuinfo", 65536, &cpuinfo);
  return cpuinfo;
}

std::string SystemInspector::meminfo(HttpRequest::Method, const Inspector::ArgList&)
{
  string meminfo;
  FileUtil::readFile("/proc/meminfo", 65536, &meminfo);
  return meminfo;
}

std::string SystemInspector::stat(HttpRequest::Method, const Inspector::ArgList&)
{
  string stat;
  FileUtil::readFile("/proc/stat", 65536, &stat);
  return stat;
}

std::string SystemInspector::overview(HttpRequest::Method, const Inspector::ArgList&)
{
  string result;
  result.reserve(1024);
  Timestamp now = Timestamp::now();
  result += "Page generated at ";
  result += now.toFormattedString();
  result += " (UTC)\n";
  // Hardware and OS
  {
  struct utsname un;
  if (::uname(&un) == 0)
  {
    stringPrintf(&result, "Hostname: %s\n", un.nodename);
    stringPrintf(&result, "Machine: %s\n", un.machine);
    stringPrintf(&result, "OS: %s %s %s\n", un.sysname, un.release, un.version);
  }
  }
  string stat;
  FileUtil::readFile("/proc/stat", 65536, &stat);
  Timestamp bootTime(Timestamp::kMicroSecondsPerSecond * getLong(stat, "btime "));
  result += "Boot time: ";
  result += bootTime.toFormattedString(false /* show microseconds */);
  result += " (UTC)\n";
  result += "Up time: ";
  result += uptime(now, bootTime, false /* show microseconds */);
  result += "\n";

  // CPU load
  {
  string loadavg;
  FileUtil::readFile("/proc/loadavg", 65536, &loadavg);
  stringPrintf(&result, "Processes created: %ld\n", getLong(stat, "processes "));
  stringPrintf(&result, "Loadavg: %s\n", loadavg.c_str());
  }

  // Memory
  {
  string meminfo;
  FileUtil::readFile("/proc/meminfo", 65536, &meminfo);
  long total_kb = getLong(meminfo, "MemTotal:");
  long free_kb = getLong(meminfo, "MemFree:");
  long buffers_kb = getLong(meminfo, "Buffers:");
  long cached_kb = getLong(meminfo, "Cached:");

  stringPrintf(&result, "Total Memory: %6ld MiB\n", total_kb / 1024);
  stringPrintf(&result, "Free Memory:  %6ld MiB\n", free_kb / 1024);
  stringPrintf(&result, "Buffers:      %6ld MiB\n", buffers_kb / 1024);
  stringPrintf(&result, "Cached:       %6ld MiB\n", cached_kb / 1024);
  stringPrintf(&result, "Real Used:    %6ld MiB\n", (total_kb - free_kb - buffers_kb - cached_kb) / 1024);
  stringPrintf(&result, "Real Free:    %6ld MiB\n", (free_kb + buffers_kb + cached_kb) / 1024);

  // Swap
  }
  // Disk
  // Network
  return result;
}