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

// UploadsType presents costom type：Uploads.
class QS_SDK_API UploadsType: QsBaseType
{
public:
	UploadsType(){};
	UploadsType(std::string serializedStr);// Object part created time
	inline void SetCreated(std::string  Created)
	{
		m_propsHasBeenSet.push_back("created");
		m_Created = Created;
	};

	inline std::string  GetCreated(){return m_Created;};

	// Object key
	inline void SetKey(std::string  Key)
	{
		m_propsHasBeenSet.push_back("key");
		m_Key = Key;
	};

	inline std::string  GetKey(){return m_Key;};

	// Object upload id
	inline void SetUploadID(std::string  UploadID)
	{
		m_propsHasBeenSet.push_back("upload_id");
		m_UploadID = UploadID;
	};

	inline std::string  GetUploadID(){return m_UploadID;};

	std::string Serialize();

private:		
	
	// Object part created time
	std::string  m_Created;
	
	// Object key
	std::string  m_Key;
	
	// Object upload id
	std::string  m_UploadID;
	

};












































