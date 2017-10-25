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

// OwnerType presents costom type：Owner.
class QS_SDK_API OwnerType: QsBaseType
{
public:
	OwnerType(){};
	OwnerType(std::string serializedStr);// User ID
	inline void SetID(std::string  ID)
	{
		m_propsHasBeenSet.push_back("id");
		m_ID = ID;
	};

	inline std::string  GetID(){return m_ID;};

	// Username
	inline void SetName(std::string  Name)
	{
		m_propsHasBeenSet.push_back("name");
		m_Name = Name;
	};

	inline std::string  GetName(){return m_Name;};

	std::string Serialize();

private:		
	
	// User ID
	std::string  m_ID;
	
	// Username
	std::string  m_Name;
	

};












































