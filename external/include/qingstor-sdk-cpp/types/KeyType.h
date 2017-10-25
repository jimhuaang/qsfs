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


// KeyType presents costom type：Key.
class QS_SDK_API KeyType: QsBaseType
{
public:
	KeyType(){};
	KeyType(std::string serializedStr);// Object created time
	inline void SetCreated(std::string  Created)
	{
		m_propsHasBeenSet.push_back("created");
		m_Created = Created;
	};

	inline std::string  GetCreated(){return m_Created;};

	// Whether this key is encrypted
	inline void SetEncrypted(bool  Encrypted)
	{
		m_propsHasBeenSet.push_back("encrypted");
		m_Encrypted = Encrypted;
	};

	inline bool  GetEncrypted(){return m_Encrypted;};

	// MD5sum of the object
	inline void SetEtag(std::string  Etag)
	{
		m_propsHasBeenSet.push_back("etag");
		m_Etag = Etag;
	};

	inline std::string  GetEtag(){return m_Etag;};

	// Object key
	inline void SetKey(std::string  Key)
	{
		m_propsHasBeenSet.push_back("key");
		m_Key = Key;
	};

	inline std::string  GetKey(){return m_Key;};

	// MIME type of the object
	inline void SetMimeType(std::string  MimeType)
	{
		m_propsHasBeenSet.push_back("mime_type");
		m_MimeType = MimeType;
	};

	inline std::string  GetMimeType(){return m_MimeType;};

	// Last modified time in unix time format
	inline void SetModified(int  Modified)
	{
		m_propsHasBeenSet.push_back("modified");
		m_Modified = Modified;
	};

	inline int  GetModified(){return m_Modified;};

	// Object content size
	inline void SetSize(int64_t  Size)
	{
		m_propsHasBeenSet.push_back("size");
		m_Size = Size;
	};

	inline int64_t  GetSize(){return m_Size;};

	std::string Serialize();

private:		
	
	// Object created time
	std::string  m_Created;
	
	// Whether this key is encrypted
	bool  m_Encrypted;
	
	// MD5sum of the object
	std::string  m_Etag;
	
	// Object key
	std::string  m_Key;
	
	// MIME type of the object
	std::string  m_MimeType;
	
	// Last modified time in unix time format
	int  m_Modified;
	
	// Object content size
	int64_t  m_Size;
	

};












































