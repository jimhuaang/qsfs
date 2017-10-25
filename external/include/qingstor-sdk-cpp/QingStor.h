// +-------------------------------------------------------------------------
// | Copyright (C) 2016 Yunify, Inc.
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

#include  "QsSdkConfig.h"
#include  "Bucket.h"
#include "HttpCommon.h" 
#include "QsErrors.h" 
#include "QsConfig.h" 
#include  <atomic>

namespace QingStor
{
// +--------------------------------------------------------------------
// |                     InputClassHeader                              
// +--------------------------------------------------------------------
// ListBucketsInput presents input for ListBuckets.
class QS_SDK_API ListBucketsInput:public QsInput
{	
public:
	inline bool CheckIfInputIsVaild()
	{
		return true;
	};
		// Limits results to buckets that in the location
	inline void SetLocation(std::string  Location)
	{
		m_propsHasBeenSet.push_back("Location");
		m_Location = Location;
	};

	inline std::string  GetLocation(){return m_Location;};
	
	private:
	// Limits results to buckets that in the location
	std::string  m_Location;
	

	};


// +--------------------------------------------------------------------
// |                     OutputClassHeader                              
// +--------------------------------------------------------------------

// ListBucketsOutput presents input for ListBuckets.
class QS_SDK_API ListBucketsOutput: public QsOutput
{	
public:
	ListBucketsOutput(QsError err, Http::HttpResponseCode responseCode):QsOutput(err, responseCode){};
	ListBucketsOutput(){};
		
	// Buckets information
	inline void SetBuckets(
	std::vector<BucketType > Buckets)
	{

		m_Buckets = Buckets;
	};

	inline 
	std::vector<BucketType > GetBuckets(){return m_Buckets;};
	
	// Bucket count
	inline void SetCount(int  Count)
	{

		m_Count = Count;
	};

	inline int  GetCount(){return m_Count;};
	
	private:
	// Buckets information
	
	std::vector<BucketType > m_Buckets;
	
	// Bucket count
	int  m_Count;
	

	
};

// +--------------------------------------------------------------------
// |                     QingStorService                              
// +--------------------------------------------------------------------

class QS_SDK_API QingStorService
{
public:
    QingStorService(const QsConfig qsConfig);

    virtual ~QingStorService(){};

    static void initService(const std::string logPath);
    static void shutdownService();

	QsConfig GetConfig(){return m_qsConfig;};

    Bucket GetBucket(std::string strBucketName, std::string strZone);

	// ListBuckets does Retrieve the bucket list.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/service/get.html
	QsError listBuckets(ListBucketsInput& input, ListBucketsOutput& output);
private:
	QsConfig m_qsConfig;
    Properties m_properties;
    static std::atomic<bool> isInit;
};

}// namespace QingStor













































