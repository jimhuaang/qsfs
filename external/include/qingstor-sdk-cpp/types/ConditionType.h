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
#include "IPAddressType.h"
#include "IsNullType.h"
#include "NotIPAddressType.h"
#include "StringLikeType.h"
#include "StringNotLikeType.h"

// ConditionType presents costom type：Condition.
class QS_SDK_API ConditionType: QsBaseType
{
public:
	ConditionType(){};
	ConditionType(std::string serializedStr);
	inline void SetIPAddress(
	IPAddressType IPAddress)
	{
		m_propsHasBeenSet.push_back("ip_address");
		m_IPAddress = IPAddress;
	};

	inline 
	IPAddressType GetIPAddress(){return m_IPAddress;};

	
	inline void SetIsNull(
	IsNullType IsNull)
	{
		m_propsHasBeenSet.push_back("is_null");
		m_IsNull = IsNull;
	};

	inline 
	IsNullType GetIsNull(){return m_IsNull;};

	
	inline void SetNotIPAddress(
	NotIPAddressType NotIPAddress)
	{
		m_propsHasBeenSet.push_back("not_ip_address");
		m_NotIPAddress = NotIPAddress;
	};

	inline 
	NotIPAddressType GetNotIPAddress(){return m_NotIPAddress;};

	
	inline void SetStringLike(
	StringLikeType StringLike)
	{
		m_propsHasBeenSet.push_back("string_like");
		m_StringLike = StringLike;
	};

	inline 
	StringLikeType GetStringLike(){return m_StringLike;};

	
	inline void SetStringNotLike(
	StringNotLikeType StringNotLike)
	{
		m_propsHasBeenSet.push_back("string_not_like");
		m_StringNotLike = StringNotLike;
	};

	inline 
	StringNotLikeType GetStringNotLike(){return m_StringNotLike;};

	std::string Serialize();

private:		
	
	
	IPAddressType m_IPAddress;
	
	
	IsNullType m_IsNull;
	
	
	NotIPAddressType m_NotIPAddress;
	
	
	StringLikeType m_StringLike;
	
	
	StringNotLikeType m_StringNotLike;
	

};












































