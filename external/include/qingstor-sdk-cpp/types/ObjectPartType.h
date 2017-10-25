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


// ObjectPartType presents costom type：ObjectPart.
class QS_SDK_API ObjectPartType: QsBaseType
{
public:
	ObjectPartType(){};
	ObjectPartType(std::string serializedStr);// Object part created time
	inline void SetCreated(std::string  Created)
	{
		m_propsHasBeenSet.push_back("created");
		m_Created = Created;
	};

	inline std::string  GetCreated(){return m_Created;};

	// MD5sum of the object part
	inline void SetEtag(std::string  Etag)
	{
		m_propsHasBeenSet.push_back("etag");
		m_Etag = Etag;
	};

	inline std::string  GetEtag(){return m_Etag;};

	// Object part number
	inline void SetPartNumber(int  PartNumber)
	{
		m_propsHasBeenSet.push_back("part_number");
		m_PartNumber = PartNumber;
	};

	inline int  GetPartNumber(){return m_PartNumber;};

	// Object part size
	inline void SetSize(int64_t  Size)
	{
		m_propsHasBeenSet.push_back("size");
		m_Size = Size;
	};

	inline int64_t  GetSize(){return m_Size;};

	std::string Serialize();

private:		
	
	// Object part created time
	std::string  m_Created;
	
	// MD5sum of the object part
	std::string  m_Etag;
	
	// Object part number
	int  m_PartNumber;// Required
	
	// Object part size
	int64_t  m_Size;
	

};












































