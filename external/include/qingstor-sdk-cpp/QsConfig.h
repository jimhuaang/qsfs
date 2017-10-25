// +-------------------------------------------------------------------------
// | Copyright (C) 2017 Yunify, Inc.
// +-------------------------------------------------------------------------
// | Licensed under the Apache License, Version 2.0 (the "License");
// | you may not use this work except in compliance with the License.
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

#pragma once

#include "QsErrors.h"
#include <string>

namespace QingStor {

	class QsConfig {
	public:
		QsConfig(std::string access_key_id, std::string secret_access_key);

		QsConfig();

		QsError loadConfigFile(std::string config_file);

	public:
		std::string m_LogLevel;
		std::string m_AdditionalUserAgent;
		std::string m_AccessKeyId;
		std::string m_SecretAccessKey;
		std::string m_Host;
		std::string m_Protocol;
		int m_Port;
		int m_ConnectionRetries;
	private:
		void InitLogLevel(std::string logLevel);
	};


}
