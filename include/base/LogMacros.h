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

#ifndef INCLUDE_BASE_LOGMACROS_H_
#define INCLUDE_BASE_LOGMACROS_H_

#include "glog/logging.h"

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
// Google INFO stream needs to be flushed. So in oreder to get latest log,
// we always flush INFO stream for all non-fatal level stream. This will help
// to prevent failures for unit test due to the uncompleted log.
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
      google::FlushLogFiles(google::INFO);                 \
    }                                                      \
  }

#define Error(msg)                                         \
  {                                                        \
    QS::Logging::Log *log = QS::Logging::GetLogInstance(); \
    if (log) {                                             \
      LOG(ERROR) << QS::Logging::GetLogLevelPrefix(        \
                        QS::Logging::LogLevel::Error)      \
                 << msg;                                   \
      google::FlushLogFiles(google::INFO);                 \
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
      LOG_IF(INFO, (condition))                                          \
          << QS::Logging::GetLogLevelPrefix(QS::Logging::LogLevel::Info) \
          << msg;                                                        \
      google::FlushLogFiles(google::INFO);                               \
    }                                                                    \
  }

#define WarningIf(condition, msg)                                        \
  {                                                                      \
    QS::Logging::Log *log = QS::Logging::GetLogInstance();               \
    if (log) {                                                           \
      LOG_IF(WARNING, (condition))                                       \
          << QS::Logging::GetLogLevelPrefix(QS::Logging::LogLevel::Warn) \
          << msg;                                                        \
      google::FlushLogFiles(google::INFO);                               \
    }                                                                    \
  }

#define ErrorIf(condition, msg)                                           \
  {                                                                       \
    QS::Logging::Log *log = QS::Logging::GetLogInstance();                \
    if (log) {                                                            \
      LOG_IF(ERROR, (condition))                                          \
          << QS::Logging::GetLogLevelPrefix(QS::Logging::LogLevel::Error) \
          << msg;                                                         \
      google::FlushLogFiles(google::INFO);                                \
    }                                                                     \
  }

#define FatalIf(condition, msg)                                           \
  {                                                                       \
    QS::Logging::Log *log = QS::Logging::GetLogInstance();                \
    if (log) {                                                            \
      LOG_IF(FATAL, (condition))                                          \
          << QS::Logging::GetLogLevelPrefix(QS::Logging::LogLevel::Fatal) \
          << msg;                                                         \
    }                                                                     \
  }

#define DebugInfo(msg)                                                         \
  {                                                                            \
    QS::Logging::Log *log = QS::Logging::GetLogInstance();                     \
    if (log && log->IsDebug()) {                                               \
      LOG(INFO) << QS::Logging::GetLogLevelPrefix(QS::Logging::LogLevel::Info) \
                << msg;                                                        \
      google::FlushLogFiles(google::INFO);                                     \
    }                                                                          \
  }

#define DebugWarning(msg)                                  \
  {                                                        \
    QS::Logging::Log *log = QS::Logging::GetLogInstance(); \
    if (log && log->IsDebug()) {                           \
      LOG(WARNING) << QS::Logging::GetLogLevelPrefix(      \
                          QS::Logging::LogLevel::Warn)     \
                   << msg;                                 \
      google::FlushLogFiles(google::INFO);                 \
    }                                                      \
  }

#define DebugError(msg)                                    \
  {                                                        \
    QS::Logging::Log *log = QS::Logging::GetLogInstance(); \
    if (log && log->IsDebug()) {                           \
      LOG(ERROR) << QS::Logging::GetLogLevelPrefix(        \
                        QS::Logging::LogLevel::Error)      \
                 << msg;                                   \
      google::FlushLogFiles(google::INFO);                 \
    }                                                      \
  }

#define DebugFatal(msg)                                    \
  {                                                        \
    QS::Logging::Log *log = QS::Logging::GetLogInstance(); \
    if (log && log->IsDebug()) {                           \
      LOG(FATAL) << QS::Logging::GetLogLevelPrefix(        \
                        QS::Logging::LogLevel::Fatal)      \
                 << msg;                                   \
    }                                                      \
  }

#define DebugInfoIf(condition, msg)                                      \
  {                                                                      \
    QS::Logging::Log *log = QS::Logging::GetLogInstance();               \
    if (log && log->IsDebug()) {                                         \
      LOG_IF(INFO, (condition))                                          \
          << QS::Logging::GetLogLevelPrefix(QS::Logging::LogLevel::Info) \
          << msg;                                                        \
      google::FlushLogFiles(google::INFO);                               \
    }                                                                    \
  }

#define DebugWarningIf(condition, msg)                                   \
  {                                                                      \
    QS::Logging::Log *log = QS::Logging::GetLogInstance();               \
    if (log && log->IsDebug()) {                                         \
      LOG_IF(WARNING, (condition))                                       \
          << QS::Logging::GetLogLevelPrefix(QS::Logging::LogLevel::Warn) \
          << msg;                                                        \
      google::FlushLogFiles(google::INFO);                               \
    }                                                                    \
  }

#define DebugErrorIf(condition, msg)                                      \
  {                                                                       \
    QS::Logging::Log *log = QS::Logging::GetLogInstance();                \
    if (log && log->IsDebug()) {                                          \
      LOG_IF(ERROR, (condition))                                          \
          << QS::Logging::GetLogLevelPrefix(QS::Logging::LogLevel::Error) \
          << msg;                                                         \
      google::FlushLogFiles(google::INFO);                                \
    }                                                                     \
  }

#define DebugFatalIf(condition, msg)                                      \
  {                                                                       \
    QS::Logging::Log *log = QS::Logging::GetLogInstance();                \
    if (log && log->IsDebug()) {                                          \
      LOG_IF(FATAL, (condition))                                          \
          << QS::Logging::GetLogLevelPrefix(QS::Logging::LogLevel::Fatal) \
          << msg;                                                         \
    }                                                                     \
  }

#endif  // DISABLE_QS_LOGGING


#endif  // INCLUDE_BASE_LOGMACROS_H_
