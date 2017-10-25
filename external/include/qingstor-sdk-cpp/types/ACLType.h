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
#include "GranteeType.h"

// ACLType presents costom type：ACL.
class QS_SDK_API ACLType: QsBaseType
{
public:
	ACLType(){};
	ACLType(std::string serializedStr);
	inline void SetGrantee(
	GranteeType Grantee)
	{
		m_propsHasBeenSet.push_back("grantee");
		m_Grantee = Grantee;
	};

	inline 
	GranteeType GetGrantee(){return m_Grantee;};

	// Permission for this grantee
	// Permission's available values: READ, WRITE, FULL_CONTROL
	inline void SetPermission(std::string  Permission)
	{
		m_propsHasBeenSet.push_back("permission");
		m_Permission = Permission;
	};

	inline std::string  GetPermission(){return m_Permission;};

	std::string Serialize();

private:		
	
	
	GranteeType m_Grantee;// Required
	
	// Permission for this grantee
	// Permission's available values: READ, WRITE, FULL_CONTROL
	std::string  m_Permission;// Required
	

};












































