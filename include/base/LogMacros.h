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

#ifndef LOGMACROS_H_INCLUDED
#define LOGMACROS_H_INCLUDED

#include <base/LogLevel.h>
#include <base/Logging.h>

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

#else  //! DISABLE_QS_LOGGING
  #define Info(msg) { \
    QS::Logging::Log *log = QS::Logging::GetLogInstance(); \
    if(log) { \
      log->LogMessage(QS::Logging::LogLevel::Info, msg); \
    } \
  } \

  #define Warning(msg) { \
    QS::Logging::Log *log = QS::Logging::GetLogInstance(); \
    if(log) { \
      log->LogMessage(QS::Logging::LogLevel::Warn, msg); \
    } \
  } 

  #define Error(msg) { \
    QS::Logging::Log *log = QS::Logging::GetLogInstance(); \
    if(log) { \
      log->LogMessage(QS::Logging::LogLevel::Error, msg); \
    } \
  }
  #define Fatal(msg) { \
    QS::Logging::Log *log = QS::Logging::GetLogInstance(); \
    if(log) { \
      log->LogMessage(QS::Logging::LogLevel::Fatal, msg); \
    } \
  }

  #define InfoIf(condition, msg) { \
    QS::Logging::Log *log = QS::Logging::GetLogInstance(); \
    if(log) { \
      log->LogMessageIf(QS::Logging::LogLevel::Info, condition, msg); \
    } \
  } \

  #define WarningIf(condition, msg) { \
    QS::Logging::Log *log = QS::Logging::GetLogInstance(); \
    if(log) { \
      log->LogMessageIf(QS::Logging::LogLevel::Warn, condition, msg); \
    } \
  } \

  #define ErrorIf(condition, msg) { \
    QS::Logging::Log *log = QS::Logging::GetLogInstance(); \
    if(log) { \
      log->LogMessageIf(QS::Logging::LogLevel::Error, condition, msg); \
    } \
  } \

  #define FatalIf(condition, msg) { \
    QS::Logging::Log *log = QS::Logging::GetLogInstance(); \
    if(log) { \
      log->LogMessageIf(QS::Logging::LogLevel::Fatal, condition, msg); \
    } \
  } \

  #define DebugInfo(msg) { \
    QS::Logging::Log *log = QS::Logging::GetLogInstance(); \
    if(log) { \
      log->DebugLogMessage(QS::Logging::LogLevel::Info, msg); \
    } \
  } \

  #define DebugWarning(msg) { \
    QS::Logging::Log *log = QS::Logging::GetLogInstance(); \
    if(log) { \
      log->DebugLogMessage(QS::Logging::LogLevel::Warn, msg); \
    } \
  } \

  #define DebugError(msg) { \
    QS::Logging::Log *log = QS::Logging::GetLogInstance(); \
    if(log) { \
      log->DebugLogMessage(QS::Logging::LogLevel::Error, msg); \
    } \
  } \

  #define DebugFatal(msg) { \
    QS::Logging::Log *log = QS::Logging::GetLogInstance(); \
    if(log) { \
      log->DebugLogMessage(QS::Logging::LogLevel::Fatal, msg); \
    } \
  } \

  #define DebugInfoIf(condition, msg) { \
    QS::Logging::Log *log = QS::Logging::GetLogInstance(); \
    if(log) { \
      log->DebugLogMessageIf(QS::Logging::LogLevel::Info, condition, msg); \
    } \
  } \

  #define DebugWarningIf(condition, msg) { \
    QS::Logging::Log *log = QS::Logging::GetLogInstance(); \
    if(log) { \
      log->DebugLogMessageIf(QS::Logging::LogLevel::Warn, condition, msg); \
    } \
  } \

  #define DebugErrorIf(condition, msg) { \
    QS::Logging::Log *log = QS::Logging::GetLogInstance(); \
    if(log) { \
      log->DebugLogMessageIf(QS::Logging::LogLevel::Error, condition, msg); \
    } \
  } \

  #define DebugFatalIf(condition, msg) { \
    QS::Logging::Log *log = QS::Logging::GetLogInstance(); \
    if(log) { \
      log->DebugLogMessageIf(QS::Logging::LogLevel::Fatal, condition, msg); \
    } \
  } \

#endif  // DISABLE_QS_LOGGING

#endif  // LOGMACROS_H_INCLUDED
