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

// CORSRuleType presents costom type：CORSRule.
class QS_SDK_API CORSRuleType: QsBaseType
{
public:
	CORSRuleType(){};
	CORSRuleType(std::string serializedStr);// Allowed headers
	inline void SetAllowedHeaders(
	std::vector<std::string > AllowedHeaders)
	{
		m_propsHasBeenSet.push_back("allowed_headers");
		m_AllowedHeaders = AllowedHeaders;
	};

	inline 
	std::vector<std::string > GetAllowedHeaders(){return m_AllowedHeaders;};

	// Allowed methods
	inline void SetAllowedMethods(
	std::vector<std::string > AllowedMethods)
	{
		m_propsHasBeenSet.push_back("allowed_methods");
		m_AllowedMethods = AllowedMethods;
	};

	inline 
	std::vector<std::string > GetAllowedMethods(){return m_AllowedMethods;};

	// Allowed origin
	inline void SetAllowedOrigin(std::string  AllowedOrigin)
	{
		m_propsHasBeenSet.push_back("allowed_origin");
		m_AllowedOrigin = AllowedOrigin;
	};

	inline std::string  GetAllowedOrigin(){return m_AllowedOrigin;};

	// Expose headers
	inline void SetExposeHeaders(
	std::vector<std::string > ExposeHeaders)
	{
		m_propsHasBeenSet.push_back("expose_headers");
		m_ExposeHeaders = ExposeHeaders;
	};

	inline 
	std::vector<std::string > GetExposeHeaders(){return m_ExposeHeaders;};

	// Max age seconds
	inline void SetMaxAgeSeconds(int  MaxAgeSeconds)
	{
		m_propsHasBeenSet.push_back("max_age_seconds");
		m_MaxAgeSeconds = MaxAgeSeconds;
	};

	inline int  GetMaxAgeSeconds(){return m_MaxAgeSeconds;};

	std::string Serialize();

private:		
	
	// Allowed headers
	
	std::vector<std::string > m_AllowedHeaders;
	
	// Allowed methods
	
	std::vector<std::string > m_AllowedMethods;// Required
	
	// Allowed origin
	std::string  m_AllowedOrigin;// Required
	
	// Expose headers
	
	std::vector<std::string > m_ExposeHeaders;
	
	// Max age seconds
	int  m_MaxAgeSeconds;
	

};












































