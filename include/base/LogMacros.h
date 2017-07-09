// +-------------------------------------------------------------------------
// | Copyright (C) 2017 Yunify, Inc.
// +-------------------------------------------------------------------------
// | Licensed under the Apache License, Version 2.0 (the "License");
// | You may not use this work except in compliance with the License.
// | You may obtain a copy of the License in the LICENSE file, or at:
// |
// | http://www.apache.org/licenses/LICENSE-2.0
// |
// | Unless required by applicable law or agreed to in writing, software
// | distributed under the License is distributed on an "AS IS" BASIS,
// | WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// | See the License for the specific language governing permissions and
// | limitations under the License.
// +-------------------------------------------------------------------------

#ifndef _QSFS_FUSE_INCLUDE_BASE_LOGMACROS_H_  // NOLINT
#define _QSFS_FUSE_INCLUDE_BASE_LOGMACROS_H_  // NOLINT

#include <glog/logging.h>  // TODO(Jim): Replace with "" after download glog

#include "base/LogLevel.h"
#include "base/Logging.h"

#ifdef DISABLE_QS_LOGGING
#define Info(msg)
#define Warning(msg)
#define Error(msg)
#define Fatal(msg)

#define InfoIf(condition, msg)
#define WarningIf(condition, msg)
#define ErrorIf(condition, msg)
#define FatalIf(condition, msg)

#define DebugInfo(msg)
#define DebugWarning(msg)
#define DebugError(msg)
#define DebugFatal(msg)

#define DebugInfoIf(condition, msg)
#define DebugWarningIf(condition, msg)
#define DebugErrorIf(condition, msg)
#define DebugFatalIf(condition, msg)

#else  // !DISABLE_QS_LOGGING
// It seems the google INFO stream needs to be flushed.
#define Info(msg)                                                              \
  {                                                                            \
    QS::Logging::Log *log = QS::Logging::GetLogInstance();                     \
    if (log) {                                                                 \
      LOG(INFO) << QS::Logging::GetLogLevelPrefix(QS::Logging::LogLevel::Info) \
                << msg;                                                        \
      google::FlushLogFiles(google::INFO);                                     \
    }                                                                          \
  }

#define Warning(msg)                                       \
  {                                                        \
    QS::Logging::Log *log = QS::Logging::GetLogInstance(); \
    if (log) {                                             \
      LOG(WARNING) << QS::Logging::GetLogLevelPrefix(      \
                          QS::Logging::LogLevel::Warn)     \
                   << msg;                                 \
    }                                                      \
  }

#define Error(msg)                                         \
  {                                                        \
    QS::Logging::Log *log = QS::Logging::GetLogInstance(); \
    if (log) {                                             \
      LOG(ERROR) << QS::Logging::GetLogLevelPrefix(        \
                        QS::Logging::LogLevel::Error)      \
                 << msg;                                   \
    }                                                      \
  }
#define Fatal(msg)                                         \
  {                                                        \
    QS::Logging::Log *log = QS::Logging::GetLogInstance(); \
    if (log) {                                             \
      LOG(FATAL) << QS::Logging::GetLogLevelPrefix(        \
                        QS::Logging::LogLevel::Fatal)      \
                 << msg;                                   \
    }                                                      \
  }

#define InfoIf(condition, msg)                                           \
  {                                                                      \
    QS::Logging::Log *log = QS::Logging::GetLogInstance();               \
    if (log) {                                                           \
      LOG_IF(INFO, condition)                                            \
          << QS::Logging::GetLogLevelPrefix(QS::Logging::LogLevel::Info) \
          << msg;                                                        \
      google::FlushLogFiles(google::INFO);                               \
    }                                                                    \
  }

#define WarningIf(condition, msg)                                        \
  {                                                                      \
    QS::Logging::Log *log = QS::Logging::GetLogInstance();               \
    if (log) {                                                           \
      LOG_IF(WARNING, condition)                                         \
          << QS::Logging::GetLogLevelPrefix(QS::Logging::LogLevel::Warn) \
          << msg;                                                        \
    }                                                                    \
  }

#define ErrorIf(condition, msg)                                           \
  {                                                                       \
    QS::Logging::Log *log = QS::Logging::GetLogInstance();                \
    if (log) {                                                            \
      LOG_IF(ERROR, condition)                                            \
          << QS::Logging::GetLogLevelPrefix(QS::Logging::LogLevel::Error) \
          << msg;                                                         \
    }                                                                     \
  }

#define FatalIf(condition, msg)                                           \
  {                                                                       \
    QS::Logging::Log *log = QS::Logging::GetLogInstance();                \
    if (log) {                                                            \
      LOG_IF(FATAL, condition)                                            \
          << QS::Logging::GetLogLevelPrefix(QS::Logging::LogLevel::Fatal) \
          << msg;                                                         \
    }                                                                     \
  }

// Log message in "debug mode" (i.e., there is no NDEBUG macro defined)
#define DebugInfo(msg)                                     \
  {                                                        \
    QS::Logging::Log *log = QS::Logging::GetLogInstance(); \
    if (log) {                                             \
      DLOG(INFO) << QS::Logging::GetLogLevelPrefix(        \
                        QS::Logging::LogLevel::Info)       \
                 << msg;                                   \
      google::FlushLogFiles(google::INFO);                 \
    }                                                      \
  }

#define DebugWarning(msg)                                  \
  {                                                        \
    QS::Logging::Log *log = QS::Logging::GetLogInstance(); \
    if (log) {                                             \
      DLOG(WARNING) << QS::Logging::GetLogLevelPrefix(     \
                           QS::Logging::LogLevel::Warn)    \
                    << msg;                                \
    }                                                      \
  }

#define DebugError(msg)                                    \
  {                                                        \
    QS::Logging::Log *log = QS::Logging::GetLogInstance(); \
    if (log) {                                             \
      DLOG(ERROR) << QS::Logging::GetLogLevelPrefix(       \
                         QS::Logging::LogLevel::Error)     \
                  << msg;                                  \
    }                                                      \
  }

#define DebugFatal(msg)                                    \
  {                                                        \
    QS::Logging::Log *log = QS::Logging::GetLogInstance(); \
    if (log) {                                             \
      DLOG(FATAL) << QS::Logging::GetLogLevelPrefix(       \
                         QS::Logging::LogLevel::Fatal)     \
                  << msg;                                  \
    }                                                      \
  }

#define DebugInfoIf(condition, msg)                                      \
  {                                                                      \
    QS::Logging::Log *log = QS::Logging::GetLogInstance();               \
    if (log) {                                                           \
      DLOG_IF(INFO, condition)                                           \
          << QS::Logging::GetLogLevelPrefix(QS::Logging::LogLevel::Info) \
          << msg;                                                        \
      google::FlushLogFiles(google::INFO);                               \
    }                                                                    \
  }

#define DebugWarningIf(condition, msg)                                   \
  {                                                                      \
    QS::Logging::Log *log = QS::Logging::GetLogInstance();               \
    if (log) {                                                           \
      DLOG_IF(WARNING, condition)                                        \
          << QS::Logging::GetLogLevelPrefix(QS::Logging::LogLevel::Warn) \
          << msg;                                                        \
    }                                                                    \
  }

#define DebugErrorIf(condition, msg)                                      \
  {                                                                       \
    QS::Logging::Log *log = QS::Logging::GetLogInstance();                \
    if (log) {                                                            \
      DLOG_IF(ERROR, condition)                                           \
          << QS::Logging::GetLogLevelPrefix(QS::Logging::LogLevel::Error) \
          << msg;                                                         \
    }                                                                     \
  }

#define DebugFatalIf(condition, msg)                                      \
  {                                                                       \
    QS::Logging::Log *log = QS::Logging::GetLogInstance();                \
    if (log) {                                                            \
      DLOG_IF(FATAL, condition)                                           \
          << QS::Logging::GetLogLevelPrefix(QS::Logging::LogLevel::Fatal) \
          << msg;                                                         \
    }                                                                     \
  }

#endif  // DISABLE_QS_LOGGING

// NOLINTNEXTLINE
#endif  // _QSFS_FUSE_INCLUDE_BASE_LOGMACROS_H_
