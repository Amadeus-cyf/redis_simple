#pragma once

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <mutex>
#include <string>
#include <vector>

#ifdef REDIS_SIMPLE_USE_SPDLOG
#include <spdlog/spdlog.h>
#endif

namespace redis_simple {
namespace logging {

enum class Level { Debug = 0, Info = 1, Warn = 2, Error = 3, Off = 4 };

inline const char* LevelName(Level level) {
  switch (level) {
    case Level::Debug:
      return "DEBUG";
    case Level::Info:
      return "INFO";
    case Level::Warn:
      return "WARN";
    case Level::Error:
      return "ERROR";
    case Level::Off:
      return "OFF";
  }
  return "UNKNOWN";
}

inline Level ParseLevel(const char* value) {
  if (!value) return Level::Debug;
  std::string level(value);
  for (char& ch : level) {
    if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
  }
  if (level == "debug") return Level::Debug;
  if (level == "info") return Level::Info;
  if (level == "warn" || level == "warning") return Level::Warn;
  if (level == "error") return Level::Error;
  if (level == "off") return Level::Off;
  return Level::Debug;
}

inline Level& MinLevel() {
  static Level level = ParseLevel(std::getenv("REDIS_SIMPLE_LOG_LEVEL"));
  return level;
}

inline void SetLevel(Level level) { MinLevel() = level; }

inline bool Enabled(Level level) {
  return static_cast<int>(level) >= static_cast<int>(MinLevel()) &&
         MinLevel() != Level::Off;
}

inline std::mutex& LogMutex() {
  static std::mutex mutex;
  return mutex;
}

#ifdef REDIS_SIMPLE_USE_SPDLOG
inline spdlog::level::level_enum ToSpdlogLevel(Level level) {
  switch (level) {
    case Level::Debug:
      return spdlog::level::debug;
    case Level::Info:
      return spdlog::level::info;
    case Level::Warn:
      return spdlog::level::warn;
    case Level::Error:
      return spdlog::level::err;
    case Level::Off:
      return spdlog::level::off;
  }
  return spdlog::level::info;
}
#endif

inline void VLogf(Level level, const char* file, int line, const char* fmt,
                  va_list args) {
  if (!Enabled(level)) return;

  char message[1024];
  va_list args_copy;
  va_copy(args_copy, args);
  int n = std::vsnprintf(message, sizeof(message), fmt, args_copy);
  va_end(args_copy);

  std::string rendered;
  if (n < 0) {
    rendered = fmt ? fmt : "";
  } else if (static_cast<size_t>(n) < sizeof(message)) {
    rendered.assign(message, static_cast<size_t>(n));
  } else {
    std::vector<char> large(static_cast<size_t>(n) + 1);
    std::vsnprintf(large.data(), large.size(), fmt, args);
    rendered.assign(large.data(), static_cast<size_t>(n));
  }

#ifdef REDIS_SIMPLE_USE_SPDLOG
  spdlog::set_level(ToSpdlogLevel(MinLevel()));
  spdlog::log(spdlog::source_loc{file, line, ""}, ToSpdlogLevel(level), "{}",
              rendered);
#else
  const auto now = std::chrono::system_clock::now();
  const std::time_t now_time = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
#if defined(_WIN32)
  localtime_s(&tm, &now_time);
#else
  localtime_r(&now_time, &tm);
#endif

  char timestamp[32];
  std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm);

  std::lock_guard<std::mutex> lock(LogMutex());
  std::fprintf(stderr, "[%s] [%s] [%s:%d] %s", timestamp, LevelName(level),
               file, line, rendered.c_str());
  if (rendered.empty() || rendered.back() != '\n') {
    std::fprintf(stderr, "\n");
  }
#endif
}

inline void Logf(Level level, const char* file, int line, const char* fmt,
                 ...) {
  va_list args;
  va_start(args, fmt);
  VLogf(level, file, line, fmt, args);
  va_end(args);
}

}  // namespace logging
}  // namespace redis_simple

#define RS_LOG_DEBUG(...)                                              \
  ::redis_simple::logging::Logf(::redis_simple::logging::Level::Debug, \
                                __FILE__, __LINE__, __VA_ARGS__)
#define RS_LOG_INFO(...)                                              \
  ::redis_simple::logging::Logf(::redis_simple::logging::Level::Info, \
                                __FILE__, __LINE__, __VA_ARGS__)
#define RS_LOG_WARN(...)                                              \
  ::redis_simple::logging::Logf(::redis_simple::logging::Level::Warn, \
                                __FILE__, __LINE__, __VA_ARGS__)
#define RS_LOG_ERROR(...)                                              \
  ::redis_simple::logging::Logf(::redis_simple::logging::Level::Error, \
                                __FILE__, __LINE__, __VA_ARGS__)
