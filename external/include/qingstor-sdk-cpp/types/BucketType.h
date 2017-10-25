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

// BucketType presents costom type：Bucket.
class QS_SDK_API BucketType: QsBaseType
{
public:
	BucketType(){};
	BucketType(std::string serializedStr);// Created time of the bucket
	inline void SetCreated(std::string  Created)
	{
		m_propsHasBeenSet.push_back("created");
		m_Created = Created;
	};

	inline std::string  GetCreated(){return m_Created;};

	// QingCloud Zone ID
	inline void SetLocation(std::string  Location)
	{
		m_propsHasBeenSet.push_back("location");
		m_Location = Location;
	};

	inline std::string  GetLocation(){return m_Location;};

	// Bucket name
	inline void SetName(std::string  Name)
	{
		m_propsHasBeenSet.push_back("name");
		m_Name = Name;
	};

	inline std::string  GetName(){return m_Name;};

	// URL to access the bucket
	inline void SetURL(std::string  URL)
	{
		m_propsHasBeenSet.push_back("url");
		m_URL = URL;
	};

	inline std::string  GetURL(){return m_URL;};

	std::string Serialize();

private:		
	
	// Created time of the bucket
	std::string  m_Created;
	
	// QingCloud Zone ID
	std::string  m_Location;
	
	// Bucket name
	std::string  m_Name;
	
	// URL to access the bucket
	std::string  m_URL;
	

};












































