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
#include "KeyType.h"

// KeyDeleteErrorType presents costom type：KeyDeleteError.
class QS_SDK_API KeyDeleteErrorType: QsBaseType
{
public:
	KeyDeleteErrorType(){};
	KeyDeleteErrorType(std::string serializedStr);// Error code
	inline void SetCode(std::string  Code)
	{
		m_propsHasBeenSet.push_back("code");
		m_Code = Code;
	};

	inline std::string  GetCode(){return m_Code;};

	// Object key
	inline void SetKey(std::string  Key)
	{
		m_propsHasBeenSet.push_back("key");
		m_Key = Key;
	};

	inline std::string  GetKey(){return m_Key;};

	// Error message
	inline void SetMessage(std::string  Message)
	{
		m_propsHasBeenSet.push_back("message");
		m_Message = Message;
	};

	inline std::string  GetMessage(){return m_Message;};

	std::string Serialize();

private:		
	
	// Error code
	std::string  m_Code;
	
	// Object key
	std::string  m_Key;
	
	// Error message
	std::string  m_Message;
	

};












































