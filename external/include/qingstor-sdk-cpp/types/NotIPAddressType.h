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

#include "QsBaseType.h"
#include <string>

// Headers of CustomizedType.

// NotIPAddressType presents costom type：NotIPAddress.
class QS_SDK_API NotIPAddressType: QsBaseType
{
public:
	NotIPAddressType(){};
	NotIPAddressType(std::string serializedStr);// Source IP
	inline void SetSourceIP(
	std::vector<std::string > SourceIP)
	{
		m_propsHasBeenSet.push_back("source_ip");
		m_SourceIP = SourceIP;
	};

	inline 
	std::vector<std::string > GetSourceIP(){return m_SourceIP;};

	std::string Serialize();

private:		
	
	// Source IP
	
	std::vector<std::string > m_SourceIP;
	

};












































