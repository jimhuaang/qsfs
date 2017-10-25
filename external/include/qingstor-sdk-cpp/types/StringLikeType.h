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

// StringLikeType presents costom type：StringLike.
class QS_SDK_API StringLikeType: QsBaseType
{
public:
	StringLikeType(){};
	StringLikeType(std::string serializedStr);// Refer url
	inline void SetReferer(
	std::vector<std::string > Referer)
	{
		m_propsHasBeenSet.push_back("Referer");
		m_Referer = Referer;
	};

	inline 
	std::vector<std::string > GetReferer(){return m_Referer;};

	std::string Serialize();

private:		
	
	// Refer url
	
	std::vector<std::string > m_Referer;
	

};












































