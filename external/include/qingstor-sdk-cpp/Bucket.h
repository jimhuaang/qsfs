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

#include <iostream>
#include <sstream>
#include <memory.h>
#include "QsSdkConfig.h"
#include "Types.h" 
#include "QsErrors.h" 
#include "QsConfig.h" 


namespace QingStor
{
// +--------------------------------------------------------------------
// |                     InputClassHeader                              
// +--------------------------------------------------------------------
        // DeleteBucketInput presents input for DeleteBucket.
typedef QsInput DeleteBucketInput;
        // DeleteBucketCORSInput presents input for DeleteBucketCORS.
typedef QsInput DeleteBucketCORSInput;
        // DeleteBucketExternalMirrorInput presents input for DeleteBucketExternalMirror.
typedef QsInput DeleteBucketExternalMirrorInput;
        // DeleteBucketPolicyInput presents input for DeleteBucketPolicy.
typedef QsInput DeleteBucketPolicyInput;
        // DeleteMultipleObjectsInput presents input for DeleteMultipleObjects.
class QS_SDK_API DeleteMultipleObjectsInput:public QsInput
{	
public:
	inline bool CheckIfInputIsVaild()
	{
		return IsPropHasBeenSet("Content-MD5") && IsPropHasBeenSet("objects") && true;
	};
		// Object MD5sum
	inline void SetContentMD5(std::string  ContentMD5)
	{
		m_propsHasBeenSet.push_back("Content-MD5");
		m_ContentMD5 = ContentMD5;
	};

	inline std::string  GetContentMD5(){return m_ContentMD5;};
	
	
		// A list of keys to delete
	inline void SetObjects(
	std::vector<KeyType > Objects)
	{
		m_propsHasBeenSet.push_back("objects");
		m_Objects = Objects;
	};

	inline 
	std::vector<KeyType > GetObjects(){return m_Objects;};
	// Whether to return the list of deleted objects
	inline void SetQuiet(bool  Quiet)
	{
		m_propsHasBeenSet.push_back("quiet");
		m_Quiet = Quiet;
	};

	inline bool  GetQuiet(){return m_Quiet;};
	
	private:
	// Object MD5sum
	std::string  m_ContentMD5;// Required
	

	
	// A list of keys to delete
	
	std::vector<KeyType > m_Objects;// Required
	
	// Whether to return the list of deleted objects
	bool  m_Quiet;
	

	};
        // GetBucketACLInput presents input for GetBucketACL.
typedef QsInput GetBucketACLInput;
        // GetBucketCORSInput presents input for GetBucketCORS.
typedef QsInput GetBucketCORSInput;
        // GetBucketExternalMirrorInput presents input for GetBucketExternalMirror.
typedef QsInput GetBucketExternalMirrorInput;
        // GetBucketPolicyInput presents input for GetBucketPolicy.
typedef QsInput GetBucketPolicyInput;
        // GetBucketStatisticsInput presents input for GetBucketStatistics.
typedef QsInput GetBucketStatisticsInput;
        // HeadBucketInput presents input for HeadBucket.
typedef QsInput HeadBucketInput;
        // ListMultipartUploadsInput presents input for ListMultipartUploads.
class QS_SDK_API ListMultipartUploadsInput:public QsInput
{	
public:
	inline bool CheckIfInputIsVaild()
	{
		return true;
	};
		// Put all keys that share a common prefix into a list
	inline void SetDelimiter(std::string  Delimiter)
	{
		m_propsHasBeenSet.push_back("delimiter");
		m_Delimiter = Delimiter;
	};

	inline std::string  GetDelimiter(){return m_Delimiter;};
	// Results count limit
	inline void SetLimit(int  Limit)
	{
		m_propsHasBeenSet.push_back("limit");
		m_Limit = Limit;
	};

	inline int  GetLimit(){return m_Limit;};
	// Limit results to keys that start at this marker
	inline void SetMarker(std::string  Marker)
	{
		m_propsHasBeenSet.push_back("marker");
		m_Marker = Marker;
	};

	inline std::string  GetMarker(){return m_Marker;};
	// Limits results to keys that begin with the prefix
	inline void SetPrefix(std::string  Prefix)
	{
		m_propsHasBeenSet.push_back("prefix");
		m_Prefix = Prefix;
	};

	inline std::string  GetPrefix(){return m_Prefix;};
	
	private:
		
	// Put all keys that share a common prefix into a list
	std::string  m_Delimiter;
	
	// Results count limit
	int  m_Limit;
	
	// Limit results to keys that start at this marker
	std::string  m_Marker;
	
	// Limits results to keys that begin with the prefix
	std::string  m_Prefix;
	

	};
        // ListObjectsInput presents input for ListObjects.
class QS_SDK_API ListObjectsInput:public QsInput
{	
public:
	inline bool CheckIfInputIsVaild()
	{
		return true;
	};
		// Put all keys that share a common prefix into a list
	inline void SetDelimiter(std::string  Delimiter)
	{
		m_propsHasBeenSet.push_back("delimiter");
		m_Delimiter = Delimiter;
	};

	inline std::string  GetDelimiter(){return m_Delimiter;};
	// Results count limit
	inline void SetLimit(int  Limit)
	{
		m_propsHasBeenSet.push_back("limit");
		m_Limit = Limit;
	};

	inline int  GetLimit(){return m_Limit;};
	// Limit results to keys that start at this marker
	inline void SetMarker(std::string  Marker)
	{
		m_propsHasBeenSet.push_back("marker");
		m_Marker = Marker;
	};

	inline std::string  GetMarker(){return m_Marker;};
	// Limits results to keys that begin with the prefix
	inline void SetPrefix(std::string  Prefix)
	{
		m_propsHasBeenSet.push_back("prefix");
		m_Prefix = Prefix;
	};

	inline std::string  GetPrefix(){return m_Prefix;};
	
	private:
		
	// Put all keys that share a common prefix into a list
	std::string  m_Delimiter;
	
	// Results count limit
	int  m_Limit;
	
	// Limit results to keys that start at this marker
	std::string  m_Marker;
	
	// Limits results to keys that begin with the prefix
	std::string  m_Prefix;
	

	};
        // PutBucketInput presents input for PutBucket.
typedef QsInput PutBucketInput;
        // PutBucketACLInput presents input for PutBucketACL.
class QS_SDK_API PutBucketACLInput:public QsInput
{	
public:
	inline bool CheckIfInputIsVaild()
	{
		return IsPropHasBeenSet("acl") && true;
	};
		// Bucket ACL rules
	inline void SetACL(
	std::vector<ACLType > ACL)
	{
		m_propsHasBeenSet.push_back("acl");
		m_ACL = ACL;
	};

	inline 
	std::vector<ACLType > GetACL(){return m_ACL;};
	
	private:
	// Bucket ACL rules
	
	std::vector<ACLType > m_ACL;// Required
	

	};
        // PutBucketCORSInput presents input for PutBucketCORS.
class QS_SDK_API PutBucketCORSInput:public QsInput
{	
public:
	inline bool CheckIfInputIsVaild()
	{
		return IsPropHasBeenSet("cors_rules") && true;
	};
		// Bucket CORS rules
	inline void SetCORSRules(
	std::vector<CORSRuleType > CORSRules)
	{
		m_propsHasBeenSet.push_back("cors_rules");
		m_CORSRules = CORSRules;
	};

	inline 
	std::vector<CORSRuleType > GetCORSRules(){return m_CORSRules;};
	
	private:
	// Bucket CORS rules
	
	std::vector<CORSRuleType > m_CORSRules;// Required
	

	};
        // PutBucketExternalMirrorInput presents input for PutBucketExternalMirror.
class QS_SDK_API PutBucketExternalMirrorInput:public QsInput
{	
public:
	inline bool CheckIfInputIsVaild()
	{
		return IsPropHasBeenSet("source_site") && true;
	};
		// Source site url
	inline void SetSourceSite(std::string  SourceSite)
	{
		m_propsHasBeenSet.push_back("source_site");
		m_SourceSite = SourceSite;
	};

	inline std::string  GetSourceSite(){return m_SourceSite;};
	
	private:
	// Source site url
	std::string  m_SourceSite;// Required
	

	};
        // PutBucketPolicyInput presents input for PutBucketPolicy.
class QS_SDK_API PutBucketPolicyInput:public QsInput
{	
public:
	inline bool CheckIfInputIsVaild()
	{
		return IsPropHasBeenSet("statement") && true;
	};
		// Bucket policy statement
	inline void SetStatement(
	std::vector<StatementType > Statement)
	{
		m_propsHasBeenSet.push_back("statement");
		m_Statement = Statement;
	};

	inline 
	std::vector<StatementType > GetStatement(){return m_Statement;};
	
	private:
	// Bucket policy statement
	
	std::vector<StatementType > m_Statement;// Required
	

	};
// +--------------------------------------------------------------------
// |                     InputClassHeader                              
// +--------------------------------------------------------------------
        // AbortMultipartUploadInput presents input for AbortMultipartUpload.
class QS_SDK_API AbortMultipartUploadInput:public QsInput
{	
public:
	inline bool CheckIfInputIsVaild()
	{
		return IsPropHasBeenSet("upload_id") && true;
	};
		// Object multipart upload ID
	inline void SetUploadID(std::string  UploadID)
	{
		m_propsHasBeenSet.push_back("upload_id");
		m_UploadID = UploadID;
	};

	inline std::string  GetUploadID(){return m_UploadID;};
	
	private:
		
	// Object multipart upload ID
	std::string  m_UploadID;// Required
	

	};
        // CompleteMultipartUploadInput presents input for CompleteMultipartUpload.
class QS_SDK_API CompleteMultipartUploadInput:public QsInput
{	
public:
	inline bool CheckIfInputIsVaild()
	{
		return IsPropHasBeenSet("upload_id") && true;
	};
		// Object multipart upload ID
	inline void SetUploadID(std::string  UploadID)
	{
		m_propsHasBeenSet.push_back("upload_id");
		m_UploadID = UploadID;
	};

	inline std::string  GetUploadID(){return m_UploadID;};
	
	
		// MD5sum of the object part
	inline void SetETag(std::string  ETag)
	{
		m_propsHasBeenSet.push_back("ETag");
		m_ETag = ETag;
	};

	inline std::string  GetETag(){return m_ETag;};
	// Encryption algorithm of the object
	inline void SetXQSEncryptionCustomerAlgorithm(std::string  XQSEncryptionCustomerAlgorithm)
	{
		m_propsHasBeenSet.push_back("X-QS-Encryption-Customer-Algorithm");
		m_XQSEncryptionCustomerAlgorithm = XQSEncryptionCustomerAlgorithm;
	};

	inline std::string  GetXQSEncryptionCustomerAlgorithm(){return m_XQSEncryptionCustomerAlgorithm;};
	// Encryption key of the object
	inline void SetXQSEncryptionCustomerKey(std::string  XQSEncryptionCustomerKey)
	{
		m_propsHasBeenSet.push_back("X-QS-Encryption-Customer-Key");
		m_XQSEncryptionCustomerKey = XQSEncryptionCustomerKey;
	};

	inline std::string  GetXQSEncryptionCustomerKey(){return m_XQSEncryptionCustomerKey;};
	// MD5sum of encryption key
	inline void SetXQSEncryptionCustomerKeyMD5(std::string  XQSEncryptionCustomerKeyMD5)
	{
		m_propsHasBeenSet.push_back("X-QS-Encryption-Customer-Key-MD5");
		m_XQSEncryptionCustomerKeyMD5 = XQSEncryptionCustomerKeyMD5;
	};

	inline std::string  GetXQSEncryptionCustomerKeyMD5(){return m_XQSEncryptionCustomerKeyMD5;};
	
	
		// Object parts
	inline void SetObjectParts(
	std::vector<ObjectPartType > ObjectParts)
	{
		m_propsHasBeenSet.push_back("object_parts");
		m_ObjectParts = ObjectParts;
	};

	inline 
	std::vector<ObjectPartType > GetObjectParts(){return m_ObjectParts;};
	
	private:
		
	// Object multipart upload ID
	std::string  m_UploadID;// Required
	

	
	// MD5sum of the object part
	std::string  m_ETag;
	
	// Encryption algorithm of the object
	std::string  m_XQSEncryptionCustomerAlgorithm;
	
	// Encryption key of the object
	std::string  m_XQSEncryptionCustomerKey;
	
	// MD5sum of encryption key
	std::string  m_XQSEncryptionCustomerKeyMD5;
	

	
	// Object parts
	
	std::vector<ObjectPartType > m_ObjectParts;
	

	};
        // DeleteObjectInput presents input for DeleteObject.
typedef QsInput DeleteObjectInput;
        // GetObjectInput presents input for GetObject.
class QS_SDK_API GetObjectInput:public QsInput
{	
public:
	inline bool CheckIfInputIsVaild()
	{
		return true;
	};
		// Specified the Cache-Control response header
	inline void SetResponseCacheControl(std::string  ResponseCacheControl)
	{
		m_propsHasBeenSet.push_back("response-cache-control");
		m_ResponseCacheControl = ResponseCacheControl;
	};

	inline std::string  GetResponseCacheControl(){return m_ResponseCacheControl;};
	// Specified the Content-Disposition response header
	inline void SetResponseContentDisposition(std::string  ResponseContentDisposition)
	{
		m_propsHasBeenSet.push_back("response-content-disposition");
		m_ResponseContentDisposition = ResponseContentDisposition;
	};

	inline std::string  GetResponseContentDisposition(){return m_ResponseContentDisposition;};
	// Specified the Content-Encoding response header
	inline void SetResponseContentEncoding(std::string  ResponseContentEncoding)
	{
		m_propsHasBeenSet.push_back("response-content-encoding");
		m_ResponseContentEncoding = ResponseContentEncoding;
	};

	inline std::string  GetResponseContentEncoding(){return m_ResponseContentEncoding;};
	// Specified the Content-Language response header
	inline void SetResponseContentLanguage(std::string  ResponseContentLanguage)
	{
		m_propsHasBeenSet.push_back("response-content-language");
		m_ResponseContentLanguage = ResponseContentLanguage;
	};

	inline std::string  GetResponseContentLanguage(){return m_ResponseContentLanguage;};
	// Specified the Content-Type response header
	inline void SetResponseContentType(std::string  ResponseContentType)
	{
		m_propsHasBeenSet.push_back("response-content-type");
		m_ResponseContentType = ResponseContentType;
	};

	inline std::string  GetResponseContentType(){return m_ResponseContentType;};
	// Specified the Expires response header
	inline void SetResponseExpires(std::string  ResponseExpires)
	{
		m_propsHasBeenSet.push_back("response-expires");
		m_ResponseExpires = ResponseExpires;
	};

	inline std::string  GetResponseExpires(){return m_ResponseExpires;};
	
	
		// Check whether the ETag matches
	inline void SetIfMatch(std::string  IfMatch)
	{
		m_propsHasBeenSet.push_back("If-Match");
		m_IfMatch = IfMatch;
	};

	inline std::string  GetIfMatch(){return m_IfMatch;};
	// Check whether the object has been modified
	inline void SetIfModifiedSince(std::string  IfModifiedSince)
	{
		m_propsHasBeenSet.push_back("If-Modified-Since");
		m_IfModifiedSince = IfModifiedSince;
	};

	inline std::string  GetIfModifiedSince(){return m_IfModifiedSince;};
	// Check whether the ETag does not match
	inline void SetIfNoneMatch(std::string  IfNoneMatch)
	{
		m_propsHasBeenSet.push_back("If-None-Match");
		m_IfNoneMatch = IfNoneMatch;
	};

	inline std::string  GetIfNoneMatch(){return m_IfNoneMatch;};
	// Check whether the object has not been modified
	inline void SetIfUnmodifiedSince(std::string  IfUnmodifiedSince)
	{
		m_propsHasBeenSet.push_back("If-Unmodified-Since");
		m_IfUnmodifiedSince = IfUnmodifiedSince;
	};

	inline std::string  GetIfUnmodifiedSince(){return m_IfUnmodifiedSince;};
	// Specified range of the object
	inline void SetRange(std::string  Range)
	{
		m_propsHasBeenSet.push_back("Range");
		m_Range = Range;
	};

	inline std::string  GetRange(){return m_Range;};
	// Encryption algorithm of the object
	inline void SetXQSEncryptionCustomerAlgorithm(std::string  XQSEncryptionCustomerAlgorithm)
	{
		m_propsHasBeenSet.push_back("X-QS-Encryption-Customer-Algorithm");
		m_XQSEncryptionCustomerAlgorithm = XQSEncryptionCustomerAlgorithm;
	};

	inline std::string  GetXQSEncryptionCustomerAlgorithm(){return m_XQSEncryptionCustomerAlgorithm;};
	// Encryption key of the object
	inline void SetXQSEncryptionCustomerKey(std::string  XQSEncryptionCustomerKey)
	{
		m_propsHasBeenSet.push_back("X-QS-Encryption-Customer-Key");
		m_XQSEncryptionCustomerKey = XQSEncryptionCustomerKey;
	};

	inline std::string  GetXQSEncryptionCustomerKey(){return m_XQSEncryptionCustomerKey;};
	// MD5sum of encryption key
	inline void SetXQSEncryptionCustomerKeyMD5(std::string  XQSEncryptionCustomerKeyMD5)
	{
		m_propsHasBeenSet.push_back("X-QS-Encryption-Customer-Key-MD5");
		m_XQSEncryptionCustomerKeyMD5 = XQSEncryptionCustomerKeyMD5;
	};

	inline std::string  GetXQSEncryptionCustomerKeyMD5(){return m_XQSEncryptionCustomerKeyMD5;};
	
	private:
		
	// Specified the Cache-Control response header
	std::string  m_ResponseCacheControl;
	
	// Specified the Content-Disposition response header
	std::string  m_ResponseContentDisposition;
	
	// Specified the Content-Encoding response header
	std::string  m_ResponseContentEncoding;
	
	// Specified the Content-Language response header
	std::string  m_ResponseContentLanguage;
	
	// Specified the Content-Type response header
	std::string  m_ResponseContentType;
	
	// Specified the Expires response header
	std::string  m_ResponseExpires;
	

	
	// Check whether the ETag matches
	std::string  m_IfMatch;
	
	// Check whether the object has been modified
	std::string  m_IfModifiedSince;
	
	// Check whether the ETag does not match
	std::string  m_IfNoneMatch;
	
	// Check whether the object has not been modified
	std::string  m_IfUnmodifiedSince;
	
	// Specified range of the object
	std::string  m_Range;
	
	// Encryption algorithm of the object
	std::string  m_XQSEncryptionCustomerAlgorithm;
	
	// Encryption key of the object
	std::string  m_XQSEncryptionCustomerKey;
	
	// MD5sum of encryption key
	std::string  m_XQSEncryptionCustomerKeyMD5;
	

	};
        // HeadObjectInput presents input for HeadObject.
class QS_SDK_API HeadObjectInput:public QsInput
{	
public:
	inline bool CheckIfInputIsVaild()
	{
		return true;
	};
		// Check whether the ETag matches
	inline void SetIfMatch(std::string  IfMatch)
	{
		m_propsHasBeenSet.push_back("If-Match");
		m_IfMatch = IfMatch;
	};

	inline std::string  GetIfMatch(){return m_IfMatch;};
	// Check whether the object has been modified
	inline void SetIfModifiedSince(std::string  IfModifiedSince)
	{
		m_propsHasBeenSet.push_back("If-Modified-Since");
		m_IfModifiedSince = IfModifiedSince;
	};

	inline std::string  GetIfModifiedSince(){return m_IfModifiedSince;};
	// Check whether the ETag does not match
	inline void SetIfNoneMatch(std::string  IfNoneMatch)
	{
		m_propsHasBeenSet.push_back("If-None-Match");
		m_IfNoneMatch = IfNoneMatch;
	};

	inline std::string  GetIfNoneMatch(){return m_IfNoneMatch;};
	// Check whether the object has not been modified
	inline void SetIfUnmodifiedSince(std::string  IfUnmodifiedSince)
	{
		m_propsHasBeenSet.push_back("If-Unmodified-Since");
		m_IfUnmodifiedSince = IfUnmodifiedSince;
	};

	inline std::string  GetIfUnmodifiedSince(){return m_IfUnmodifiedSince;};
	// Encryption algorithm of the object
	inline void SetXQSEncryptionCustomerAlgorithm(std::string  XQSEncryptionCustomerAlgorithm)
	{
		m_propsHasBeenSet.push_back("X-QS-Encryption-Customer-Algorithm");
		m_XQSEncryptionCustomerAlgorithm = XQSEncryptionCustomerAlgorithm;
	};

	inline std::string  GetXQSEncryptionCustomerAlgorithm(){return m_XQSEncryptionCustomerAlgorithm;};
	// Encryption key of the object
	inline void SetXQSEncryptionCustomerKey(std::string  XQSEncryptionCustomerKey)
	{
		m_propsHasBeenSet.push_back("X-QS-Encryption-Customer-Key");
		m_XQSEncryptionCustomerKey = XQSEncryptionCustomerKey;
	};

	inline std::string  GetXQSEncryptionCustomerKey(){return m_XQSEncryptionCustomerKey;};
	// MD5sum of encryption key
	inline void SetXQSEncryptionCustomerKeyMD5(std::string  XQSEncryptionCustomerKeyMD5)
	{
		m_propsHasBeenSet.push_back("X-QS-Encryption-Customer-Key-MD5");
		m_XQSEncryptionCustomerKeyMD5 = XQSEncryptionCustomerKeyMD5;
	};

	inline std::string  GetXQSEncryptionCustomerKeyMD5(){return m_XQSEncryptionCustomerKeyMD5;};
	
	private:
	// Check whether the ETag matches
	std::string  m_IfMatch;
	
	// Check whether the object has been modified
	std::string  m_IfModifiedSince;
	
	// Check whether the ETag does not match
	std::string  m_IfNoneMatch;
	
	// Check whether the object has not been modified
	std::string  m_IfUnmodifiedSince;
	
	// Encryption algorithm of the object
	std::string  m_XQSEncryptionCustomerAlgorithm;
	
	// Encryption key of the object
	std::string  m_XQSEncryptionCustomerKey;
	
	// MD5sum of encryption key
	std::string  m_XQSEncryptionCustomerKeyMD5;
	

	};
        // InitiateMultipartUploadInput presents input for InitiateMultipartUpload.
class QS_SDK_API InitiateMultipartUploadInput:public QsInput
{	
public:
	inline bool CheckIfInputIsVaild()
	{
		return true;
	};
		// Object content type
	inline void SetContentType(std::string  ContentType)
	{
		m_propsHasBeenSet.push_back("Content-Type");
		m_ContentType = ContentType;
	};

	inline std::string  GetContentType(){return m_ContentType;};
	// Encryption algorithm of the object
	inline void SetXQSEncryptionCustomerAlgorithm(std::string  XQSEncryptionCustomerAlgorithm)
	{
		m_propsHasBeenSet.push_back("X-QS-Encryption-Customer-Algorithm");
		m_XQSEncryptionCustomerAlgorithm = XQSEncryptionCustomerAlgorithm;
	};

	inline std::string  GetXQSEncryptionCustomerAlgorithm(){return m_XQSEncryptionCustomerAlgorithm;};
	// Encryption key of the object
	inline void SetXQSEncryptionCustomerKey(std::string  XQSEncryptionCustomerKey)
	{
		m_propsHasBeenSet.push_back("X-QS-Encryption-Customer-Key");
		m_XQSEncryptionCustomerKey = XQSEncryptionCustomerKey;
	};

	inline std::string  GetXQSEncryptionCustomerKey(){return m_XQSEncryptionCustomerKey;};
	// MD5sum of encryption key
	inline void SetXQSEncryptionCustomerKeyMD5(std::string  XQSEncryptionCustomerKeyMD5)
	{
		m_propsHasBeenSet.push_back("X-QS-Encryption-Customer-Key-MD5");
		m_XQSEncryptionCustomerKeyMD5 = XQSEncryptionCustomerKeyMD5;
	};

	inline std::string  GetXQSEncryptionCustomerKeyMD5(){return m_XQSEncryptionCustomerKeyMD5;};
	
	private:
	// Object content type
	std::string  m_ContentType;
	
	// Encryption algorithm of the object
	std::string  m_XQSEncryptionCustomerAlgorithm;
	
	// Encryption key of the object
	std::string  m_XQSEncryptionCustomerKey;
	
	// MD5sum of encryption key
	std::string  m_XQSEncryptionCustomerKeyMD5;
	

	};
        // ListMultipartInput presents input for ListMultipart.
class QS_SDK_API ListMultipartInput:public QsInput
{	
public:
	inline bool CheckIfInputIsVaild()
	{
		return IsPropHasBeenSet("upload_id") && true;
	};
		// Limit results count
	inline void SetLimit(int  Limit)
	{
		m_propsHasBeenSet.push_back("limit");
		m_Limit = Limit;
	};

	inline int  GetLimit(){return m_Limit;};
	// Object multipart upload part number
	inline void SetPartNumberMarker(int  PartNumberMarker)
	{
		m_propsHasBeenSet.push_back("part_number_marker");
		m_PartNumberMarker = PartNumberMarker;
	};

	inline int  GetPartNumberMarker(){return m_PartNumberMarker;};
	// Object multipart upload ID
	inline void SetUploadID(std::string  UploadID)
	{
		m_propsHasBeenSet.push_back("upload_id");
		m_UploadID = UploadID;
	};

	inline std::string  GetUploadID(){return m_UploadID;};
	
	private:
		
	// Limit results count
	int  m_Limit;
	
	// Object multipart upload part number
	int  m_PartNumberMarker;
	
	// Object multipart upload ID
	std::string  m_UploadID;// Required
	

	};
        // OptionsObjectInput presents input for OptionsObject.
class QS_SDK_API OptionsObjectInput:public QsInput
{	
public:
	inline bool CheckIfInputIsVaild()
	{
		return IsPropHasBeenSet("Access-Control-Request-Method") && IsPropHasBeenSet("Origin") && true;
	};
		// Request headers
	inline void SetAccessControlRequestHeaders(std::string  AccessControlRequestHeaders)
	{
		m_propsHasBeenSet.push_back("Access-Control-Request-Headers");
		m_AccessControlRequestHeaders = AccessControlRequestHeaders;
	};

	inline std::string  GetAccessControlRequestHeaders(){return m_AccessControlRequestHeaders;};
	// Request method
	inline void SetAccessControlRequestMethod(std::string  AccessControlRequestMethod)
	{
		m_propsHasBeenSet.push_back("Access-Control-Request-Method");
		m_AccessControlRequestMethod = AccessControlRequestMethod;
	};

	inline std::string  GetAccessControlRequestMethod(){return m_AccessControlRequestMethod;};
	// Request origin
	inline void SetOrigin(std::string  Origin)
	{
		m_propsHasBeenSet.push_back("Origin");
		m_Origin = Origin;
	};

	inline std::string  GetOrigin(){return m_Origin;};
	
	private:
	// Request headers
	std::string  m_AccessControlRequestHeaders;
	
	// Request method
	std::string  m_AccessControlRequestMethod;// Required
	
	// Request origin
	std::string  m_Origin;// Required
	

	};
        // PutObjectInput presents input for PutObject.
class QS_SDK_API PutObjectInput:public QsInput
{	
public:
	inline bool CheckIfInputIsVaild()
	{
		return IsPropHasBeenSet("Content-Length") && true;
	};
		// Object content size
	inline void SetContentLength(int64_t  ContentLength)
	{
		m_propsHasBeenSet.push_back("Content-Length");
		m_ContentLength = ContentLength;
	};

	inline int64_t  GetContentLength(){return m_ContentLength;};
	// Object MD5sum
	inline void SetContentMD5(std::string  ContentMD5)
	{
		m_propsHasBeenSet.push_back("Content-MD5");
		m_ContentMD5 = ContentMD5;
	};

	inline std::string  GetContentMD5(){return m_ContentMD5;};
	// Object content type
	inline void SetContentType(std::string  ContentType)
	{
		m_propsHasBeenSet.push_back("Content-Type");
		m_ContentType = ContentType;
	};

	inline std::string  GetContentType(){return m_ContentType;};
	// Used to indicate that particular server behaviors are required by the client
	inline void SetExpect(std::string  Expect)
	{
		m_propsHasBeenSet.push_back("Expect");
		m_Expect = Expect;
	};

	inline std::string  GetExpect(){return m_Expect;};
	// Copy source, format (/<bucket-name>/<object-key>)
	inline void SetXQSCopySource(std::string  XQSCopySource)
	{
		m_propsHasBeenSet.push_back("X-QS-Copy-Source");
		m_XQSCopySource = XQSCopySource;
	};

	inline std::string  GetXQSCopySource(){return m_XQSCopySource;};
	// Encryption algorithm of the object
	inline void SetXQSCopySourceEncryptionCustomerAlgorithm(std::string  XQSCopySourceEncryptionCustomerAlgorithm)
	{
		m_propsHasBeenSet.push_back("X-QS-Copy-Source-Encryption-Customer-Algorithm");
		m_XQSCopySourceEncryptionCustomerAlgorithm = XQSCopySourceEncryptionCustomerAlgorithm;
	};

	inline std::string  GetXQSCopySourceEncryptionCustomerAlgorithm(){return m_XQSCopySourceEncryptionCustomerAlgorithm;};
	// Encryption key of the object
	inline void SetXQSCopySourceEncryptionCustomerKey(std::string  XQSCopySourceEncryptionCustomerKey)
	{
		m_propsHasBeenSet.push_back("X-QS-Copy-Source-Encryption-Customer-Key");
		m_XQSCopySourceEncryptionCustomerKey = XQSCopySourceEncryptionCustomerKey;
	};

	inline std::string  GetXQSCopySourceEncryptionCustomerKey(){return m_XQSCopySourceEncryptionCustomerKey;};
	// MD5sum of encryption key
	inline void SetXQSCopySourceEncryptionCustomerKeyMD5(std::string  XQSCopySourceEncryptionCustomerKeyMD5)
	{
		m_propsHasBeenSet.push_back("X-QS-Copy-Source-Encryption-Customer-Key-MD5");
		m_XQSCopySourceEncryptionCustomerKeyMD5 = XQSCopySourceEncryptionCustomerKeyMD5;
	};

	inline std::string  GetXQSCopySourceEncryptionCustomerKeyMD5(){return m_XQSCopySourceEncryptionCustomerKeyMD5;};
	// Check whether the copy source matches
	inline void SetXQSCopySourceIfMatch(std::string  XQSCopySourceIfMatch)
	{
		m_propsHasBeenSet.push_back("X-QS-Copy-Source-If-Match");
		m_XQSCopySourceIfMatch = XQSCopySourceIfMatch;
	};

	inline std::string  GetXQSCopySourceIfMatch(){return m_XQSCopySourceIfMatch;};
	// Check whether the copy source has been modified
	inline void SetXQSCopySourceIfModifiedSince(std::string  XQSCopySourceIfModifiedSince)
	{
		m_propsHasBeenSet.push_back("X-QS-Copy-Source-If-Modified-Since");
		m_XQSCopySourceIfModifiedSince = XQSCopySourceIfModifiedSince;
	};

	inline std::string  GetXQSCopySourceIfModifiedSince(){return m_XQSCopySourceIfModifiedSince;};
	// Check whether the copy source does not match
	inline void SetXQSCopySourceIfNoneMatch(std::string  XQSCopySourceIfNoneMatch)
	{
		m_propsHasBeenSet.push_back("X-QS-Copy-Source-If-None-Match");
		m_XQSCopySourceIfNoneMatch = XQSCopySourceIfNoneMatch;
	};

	inline std::string  GetXQSCopySourceIfNoneMatch(){return m_XQSCopySourceIfNoneMatch;};
	// Check whether the copy source has not been modified
	inline void SetXQSCopySourceIfUnmodifiedSince(std::string  XQSCopySourceIfUnmodifiedSince)
	{
		m_propsHasBeenSet.push_back("X-QS-Copy-Source-If-Unmodified-Since");
		m_XQSCopySourceIfUnmodifiedSince = XQSCopySourceIfUnmodifiedSince;
	};

	inline std::string  GetXQSCopySourceIfUnmodifiedSince(){return m_XQSCopySourceIfUnmodifiedSince;};
	// Encryption algorithm of the object
	inline void SetXQSEncryptionCustomerAlgorithm(std::string  XQSEncryptionCustomerAlgorithm)
	{
		m_propsHasBeenSet.push_back("X-QS-Encryption-Customer-Algorithm");
		m_XQSEncryptionCustomerAlgorithm = XQSEncryptionCustomerAlgorithm;
	};

	inline std::string  GetXQSEncryptionCustomerAlgorithm(){return m_XQSEncryptionCustomerAlgorithm;};
	// Encryption key of the object
	inline void SetXQSEncryptionCustomerKey(std::string  XQSEncryptionCustomerKey)
	{
		m_propsHasBeenSet.push_back("X-QS-Encryption-Customer-Key");
		m_XQSEncryptionCustomerKey = XQSEncryptionCustomerKey;
	};

	inline std::string  GetXQSEncryptionCustomerKey(){return m_XQSEncryptionCustomerKey;};
	// MD5sum of encryption key
	inline void SetXQSEncryptionCustomerKeyMD5(std::string  XQSEncryptionCustomerKeyMD5)
	{
		m_propsHasBeenSet.push_back("X-QS-Encryption-Customer-Key-MD5");
		m_XQSEncryptionCustomerKeyMD5 = XQSEncryptionCustomerKeyMD5;
	};

	inline std::string  GetXQSEncryptionCustomerKeyMD5(){return m_XQSEncryptionCustomerKeyMD5;};
	// Check whether fetch target object has not been modified
	inline void SetXQSFetchIfUnmodifiedSince(std::string  XQSFetchIfUnmodifiedSince)
	{
		m_propsHasBeenSet.push_back("X-QS-Fetch-If-Unmodified-Since");
		m_XQSFetchIfUnmodifiedSince = XQSFetchIfUnmodifiedSince;
	};

	inline std::string  GetXQSFetchIfUnmodifiedSince(){return m_XQSFetchIfUnmodifiedSince;};
	// Fetch source, should be a valid url
	inline void SetXQSFetchSource(std::string  XQSFetchSource)
	{
		m_propsHasBeenSet.push_back("X-QS-Fetch-Source");
		m_XQSFetchSource = XQSFetchSource;
	};

	inline std::string  GetXQSFetchSource(){return m_XQSFetchSource;};
	// Move source, format (/<bucket-name>/<object-key>)
	inline void SetXQSMoveSource(std::string  XQSMoveSource)
	{
		m_propsHasBeenSet.push_back("X-QS-Move-Source");
		m_XQSMoveSource = XQSMoveSource;
	};

	inline std::string  GetXQSMoveSource(){return m_XQSMoveSource;};
	
	
		std::shared_ptr<std::iostream> GetBody(){ return m_streambody;};
		void SetBody(std::shared_ptr<std::iostream> streambody){ m_streambody = streambody;};
	private:
	// Object content size
	int64_t  m_ContentLength;// Required
	
	// Object MD5sum
	std::string  m_ContentMD5;
	
	// Object content type
	std::string  m_ContentType;
	
	// Used to indicate that particular server behaviors are required by the client
	std::string  m_Expect;
	
	// Copy source, format (/<bucket-name>/<object-key>)
	std::string  m_XQSCopySource;
	
	// Encryption algorithm of the object
	std::string  m_XQSCopySourceEncryptionCustomerAlgorithm;
	
	// Encryption key of the object
	std::string  m_XQSCopySourceEncryptionCustomerKey;
	
	// MD5sum of encryption key
	std::string  m_XQSCopySourceEncryptionCustomerKeyMD5;
	
	// Check whether the copy source matches
	std::string  m_XQSCopySourceIfMatch;
	
	// Check whether the copy source has been modified
	std::string  m_XQSCopySourceIfModifiedSince;
	
	// Check whether the copy source does not match
	std::string  m_XQSCopySourceIfNoneMatch;
	
	// Check whether the copy source has not been modified
	std::string  m_XQSCopySourceIfUnmodifiedSince;
	
	// Encryption algorithm of the object
	std::string  m_XQSEncryptionCustomerAlgorithm;
	
	// Encryption key of the object
	std::string  m_XQSEncryptionCustomerKey;
	
	// MD5sum of encryption key
	std::string  m_XQSEncryptionCustomerKeyMD5;
	
	// Check whether fetch target object has not been modified
	std::string  m_XQSFetchIfUnmodifiedSince;
	
	// Fetch source, should be a valid url
	std::string  m_XQSFetchSource;
	
	// Move source, format (/<bucket-name>/<object-key>)
	std::string  m_XQSMoveSource;
	

	
		std::shared_ptr<std::iostream> m_streambody;
	};
        // UploadMultipartInput presents input for UploadMultipart.
class QS_SDK_API UploadMultipartInput:public QsInput
{	
public:
	inline bool CheckIfInputIsVaild()
	{
		return IsPropHasBeenSet("part_number") && IsPropHasBeenSet("upload_id") && true;
	};
		// Object multipart upload part number
	inline void SetPartNumber(int  PartNumber)
	{
		m_propsHasBeenSet.push_back("part_number");
		m_PartNumber = PartNumber;
	};

	inline int  GetPartNumber(){return m_PartNumber;};
	// Object multipart upload ID
	inline void SetUploadID(std::string  UploadID)
	{
		m_propsHasBeenSet.push_back("upload_id");
		m_UploadID = UploadID;
	};

	inline std::string  GetUploadID(){return m_UploadID;};
	
	
		// Object multipart content length
	inline void SetContentLength(int64_t  ContentLength)
	{
		m_propsHasBeenSet.push_back("Content-Length");
		m_ContentLength = ContentLength;
	};

	inline int64_t  GetContentLength(){return m_ContentLength;};
	// Object multipart content MD5sum
	inline void SetContentMD5(std::string  ContentMD5)
	{
		m_propsHasBeenSet.push_back("Content-MD5");
		m_ContentMD5 = ContentMD5;
	};

	inline std::string  GetContentMD5(){return m_ContentMD5;};
	// Specify range of the source object
	inline void SetXQSCopyRange(std::string  XQSCopyRange)
	{
		m_propsHasBeenSet.push_back("X-QS-Copy-Range");
		m_XQSCopyRange = XQSCopyRange;
	};

	inline std::string  GetXQSCopyRange(){return m_XQSCopyRange;};
	// Copy source, format (/<bucket-name>/<object-key>)
	inline void SetXQSCopySource(std::string  XQSCopySource)
	{
		m_propsHasBeenSet.push_back("X-QS-Copy-Source");
		m_XQSCopySource = XQSCopySource;
	};

	inline std::string  GetXQSCopySource(){return m_XQSCopySource;};
	// Encryption algorithm of the object
	inline void SetXQSCopySourceEncryptionCustomerAlgorithm(std::string  XQSCopySourceEncryptionCustomerAlgorithm)
	{
		m_propsHasBeenSet.push_back("X-QS-Copy-Source-Encryption-Customer-Algorithm");
		m_XQSCopySourceEncryptionCustomerAlgorithm = XQSCopySourceEncryptionCustomerAlgorithm;
	};

	inline std::string  GetXQSCopySourceEncryptionCustomerAlgorithm(){return m_XQSCopySourceEncryptionCustomerAlgorithm;};
	// Encryption key of the object
	inline void SetXQSCopySourceEncryptionCustomerKey(std::string  XQSCopySourceEncryptionCustomerKey)
	{
		m_propsHasBeenSet.push_back("X-QS-Copy-Source-Encryption-Customer-Key");
		m_XQSCopySourceEncryptionCustomerKey = XQSCopySourceEncryptionCustomerKey;
	};

	inline std::string  GetXQSCopySourceEncryptionCustomerKey(){return m_XQSCopySourceEncryptionCustomerKey;};
	// MD5sum of encryption key
	inline void SetXQSCopySourceEncryptionCustomerKeyMD5(std::string  XQSCopySourceEncryptionCustomerKeyMD5)
	{
		m_propsHasBeenSet.push_back("X-QS-Copy-Source-Encryption-Customer-Key-MD5");
		m_XQSCopySourceEncryptionCustomerKeyMD5 = XQSCopySourceEncryptionCustomerKeyMD5;
	};

	inline std::string  GetXQSCopySourceEncryptionCustomerKeyMD5(){return m_XQSCopySourceEncryptionCustomerKeyMD5;};
	// Check whether the Etag of copy source matches the specified value
	inline void SetXQSCopySourceIfMatch(std::string  XQSCopySourceIfMatch)
	{
		m_propsHasBeenSet.push_back("X-QS-Copy-Source-If-Match");
		m_XQSCopySourceIfMatch = XQSCopySourceIfMatch;
	};

	inline std::string  GetXQSCopySourceIfMatch(){return m_XQSCopySourceIfMatch;};
	// Check whether the copy source has been modified since the specified date
	inline void SetXQSCopySourceIfModifiedSince(std::string  XQSCopySourceIfModifiedSince)
	{
		m_propsHasBeenSet.push_back("X-QS-Copy-Source-If-Modified-Since");
		m_XQSCopySourceIfModifiedSince = XQSCopySourceIfModifiedSince;
	};

	inline std::string  GetXQSCopySourceIfModifiedSince(){return m_XQSCopySourceIfModifiedSince;};
	// Check whether the Etag of copy source does not matches the specified value
	inline void SetXQSCopySourceIfNoneMatch(std::string  XQSCopySourceIfNoneMatch)
	{
		m_propsHasBeenSet.push_back("X-QS-Copy-Source-If-None-Match");
		m_XQSCopySourceIfNoneMatch = XQSCopySourceIfNoneMatch;
	};

	inline std::string  GetXQSCopySourceIfNoneMatch(){return m_XQSCopySourceIfNoneMatch;};
	// Check whether the copy source has not been unmodified since the specified date
	inline void SetXQSCopySourceIfUnmodifiedSince(std::string  XQSCopySourceIfUnmodifiedSince)
	{
		m_propsHasBeenSet.push_back("X-QS-Copy-Source-If-Unmodified-Since");
		m_XQSCopySourceIfUnmodifiedSince = XQSCopySourceIfUnmodifiedSince;
	};

	inline std::string  GetXQSCopySourceIfUnmodifiedSince(){return m_XQSCopySourceIfUnmodifiedSince;};
	// Encryption algorithm of the object
	inline void SetXQSEncryptionCustomerAlgorithm(std::string  XQSEncryptionCustomerAlgorithm)
	{
		m_propsHasBeenSet.push_back("X-QS-Encryption-Customer-Algorithm");
		m_XQSEncryptionCustomerAlgorithm = XQSEncryptionCustomerAlgorithm;
	};

	inline std::string  GetXQSEncryptionCustomerAlgorithm(){return m_XQSEncryptionCustomerAlgorithm;};
	// Encryption key of the object
	inline void SetXQSEncryptionCustomerKey(std::string  XQSEncryptionCustomerKey)
	{
		m_propsHasBeenSet.push_back("X-QS-Encryption-Customer-Key");
		m_XQSEncryptionCustomerKey = XQSEncryptionCustomerKey;
	};

	inline std::string  GetXQSEncryptionCustomerKey(){return m_XQSEncryptionCustomerKey;};
	// MD5sum of encryption key
	inline void SetXQSEncryptionCustomerKeyMD5(std::string  XQSEncryptionCustomerKeyMD5)
	{
		m_propsHasBeenSet.push_back("X-QS-Encryption-Customer-Key-MD5");
		m_XQSEncryptionCustomerKeyMD5 = XQSEncryptionCustomerKeyMD5;
	};

	inline std::string  GetXQSEncryptionCustomerKeyMD5(){return m_XQSEncryptionCustomerKeyMD5;};
	
	
		std::shared_ptr<std::iostream> GetBody(){ return m_streambody;};
		void SetBody(std::shared_ptr<std::iostream> streambody){ m_streambody = streambody;};
	private:
		
	// Object multipart upload part number
	int  m_PartNumber;// Required
	
	// Object multipart upload ID
	std::string  m_UploadID;// Required
	

	
	// Object multipart content length
	int64_t  m_ContentLength;
	
	// Object multipart content MD5sum
	std::string  m_ContentMD5;
	
	// Specify range of the source object
	std::string  m_XQSCopyRange;
	
	// Copy source, format (/<bucket-name>/<object-key>)
	std::string  m_XQSCopySource;
	
	// Encryption algorithm of the object
	std::string  m_XQSCopySourceEncryptionCustomerAlgorithm;
	
	// Encryption key of the object
	std::string  m_XQSCopySourceEncryptionCustomerKey;
	
	// MD5sum of encryption key
	std::string  m_XQSCopySourceEncryptionCustomerKeyMD5;
	
	// Check whether the Etag of copy source matches the specified value
	std::string  m_XQSCopySourceIfMatch;
	
	// Check whether the copy source has been modified since the specified date
	std::string  m_XQSCopySourceIfModifiedSince;
	
	// Check whether the Etag of copy source does not matches the specified value
	std::string  m_XQSCopySourceIfNoneMatch;
	
	// Check whether the copy source has not been unmodified since the specified date
	std::string  m_XQSCopySourceIfUnmodifiedSince;
	
	// Encryption algorithm of the object
	std::string  m_XQSEncryptionCustomerAlgorithm;
	
	// Encryption key of the object
	std::string  m_XQSEncryptionCustomerKey;
	
	// MD5sum of encryption key
	std::string  m_XQSEncryptionCustomerKeyMD5;
	

	
		std::shared_ptr<std::iostream> m_streambody;
	};
// +--------------------------------------------------------------------
// |                     InputClassHeader                              
// +--------------------------------------------------------------------
        
typedef QsOutput DeleteBucketOutput;
        
typedef QsOutput DeleteBucketCORSOutput;
        
typedef QsOutput DeleteBucketExternalMirrorOutput;
        
typedef QsOutput DeleteBucketPolicyOutput;
        
// DeleteMultipleObjectsOutput presents input for DeleteMultipleObjects.
class QS_SDK_API DeleteMultipleObjectsOutput: public QsOutput
{	
public:
	DeleteMultipleObjectsOutput(QsError err, Http::HttpResponseCode responseCode):QsOutput(err, responseCode){};
	DeleteMultipleObjectsOutput(){};
		
	// List of deleted objects
	inline void SetDeleted(
	std::vector<KeyType > Deleted)
	{

		m_Deleted = Deleted;
	};

	inline 
	std::vector<KeyType > GetDeleted(){return m_Deleted;};
	
	// Error messages
	inline void SetErrors(
	std::vector<KeyDeleteErrorType > Errors)
	{

		m_Errors = Errors;
	};

	inline 
	std::vector<KeyDeleteErrorType > GetErrors(){return m_Errors;};
	
	private:
	// List of deleted objects
	
	std::vector<KeyType > m_Deleted;
	
	// Error messages
	
	std::vector<KeyDeleteErrorType > m_Errors;
	

	
};
        
// GetBucketACLOutput presents input for GetBucketACL.
class QS_SDK_API GetBucketACLOutput: public QsOutput
{	
public:
	GetBucketACLOutput(QsError err, Http::HttpResponseCode responseCode):QsOutput(err, responseCode){};
	GetBucketACLOutput(){};
		
	// Bucket ACL rules
	inline void SetACL(
	std::vector<ACLType > ACL)
	{

		m_ACL = ACL;
	};

	inline 
	std::vector<ACLType > GetACL(){return m_ACL;};
	
	// Bucket owner
	inline void SetOwner(
	OwnerType Owner)
	{

		m_Owner = Owner;
	};

	inline 
	OwnerType GetOwner(){return m_Owner;};
	
	private:
	// Bucket ACL rules
	
	std::vector<ACLType > m_ACL;
	
	// Bucket owner
	
	OwnerType m_Owner;
	

	
};
        
// GetBucketCORSOutput presents input for GetBucketCORS.
class QS_SDK_API GetBucketCORSOutput: public QsOutput
{	
public:
	GetBucketCORSOutput(QsError err, Http::HttpResponseCode responseCode):QsOutput(err, responseCode){};
	GetBucketCORSOutput(){};
		
	// Bucket CORS rules
	inline void SetCORSRules(
	std::vector<CORSRuleType > CORSRules)
	{

		m_CORSRules = CORSRules;
	};

	inline 
	std::vector<CORSRuleType > GetCORSRules(){return m_CORSRules;};
	
	private:
	// Bucket CORS rules
	
	std::vector<CORSRuleType > m_CORSRules;
	

	
};
        
// GetBucketExternalMirrorOutput presents input for GetBucketExternalMirror.
class QS_SDK_API GetBucketExternalMirrorOutput: public QsOutput
{	
public:
	GetBucketExternalMirrorOutput(QsError err, Http::HttpResponseCode responseCode):QsOutput(err, responseCode){};
	GetBucketExternalMirrorOutput(){};
		
	// Source site url
	inline void SetSourceSite(std::string  SourceSite)
	{

		m_SourceSite = SourceSite;
	};

	inline std::string  GetSourceSite(){return m_SourceSite;};
	
	private:
	// Source site url
	std::string  m_SourceSite;
	

	
};
        
// GetBucketPolicyOutput presents input for GetBucketPolicy.
class QS_SDK_API GetBucketPolicyOutput: public QsOutput
{	
public:
	GetBucketPolicyOutput(QsError err, Http::HttpResponseCode responseCode):QsOutput(err, responseCode){};
	GetBucketPolicyOutput(){};
		
	// Bucket policy statement
	inline void SetStatement(
	std::vector<StatementType > Statement)
	{

		m_Statement = Statement;
	};

	inline 
	std::vector<StatementType > GetStatement(){return m_Statement;};
	
	private:
	// Bucket policy statement
	
	std::vector<StatementType > m_Statement;
	

	
};
        
// GetBucketStatisticsOutput presents input for GetBucketStatistics.
class QS_SDK_API GetBucketStatisticsOutput: public QsOutput
{	
public:
	GetBucketStatisticsOutput(QsError err, Http::HttpResponseCode responseCode):QsOutput(err, responseCode){};
	GetBucketStatisticsOutput(){};
		
	// Objects count in the bucket
	inline void SetCount(int64_t  Count)
	{

		m_Count = Count;
	};

	inline int64_t  GetCount(){return m_Count;};
	
	// Bucket created time
	inline void SetCreated(std::string  Created)
	{

		m_Created = Created;
	};

	inline std::string  GetCreated(){return m_Created;};
	
	// QingCloud Zone ID
	inline void SetLocation(std::string  Location)
	{

		m_Location = Location;
	};

	inline std::string  GetLocation(){return m_Location;};
	
	// Bucket name
	inline void SetName(std::string  Name)
	{

		m_Name = Name;
	};

	inline std::string  GetName(){return m_Name;};
	
	// Bucket storage size
	inline void SetSize(int64_t  Size)
	{

		m_Size = Size;
	};

	inline int64_t  GetSize(){return m_Size;};
	
	// Bucket status// Status's available values: active, suspended
	inline void SetStatus(std::string  Status)
	{

		m_Status = Status;
	};

	inline std::string  GetStatus(){return m_Status;};
	
	// URL to access the bucket
	inline void SetURL(std::string  URL)
	{

		m_URL = URL;
	};

	inline std::string  GetURL(){return m_URL;};
	
	private:
	// Objects count in the bucket
	int64_t  m_Count;
	
	// Bucket created time
	std::string  m_Created;
	
	// QingCloud Zone ID
	std::string  m_Location;
	
	// Bucket name
	std::string  m_Name;
	
	// Bucket storage size
	int64_t  m_Size;
	
	// Bucket status
	// Status's available values: active, suspended
	std::string  m_Status;
	
	// URL to access the bucket
	std::string  m_URL;
	

	
};
        
typedef QsOutput HeadBucketOutput;
        
// ListMultipartUploadsOutput presents input for ListMultipartUploads.
class QS_SDK_API ListMultipartUploadsOutput: public QsOutput
{	
public:
	ListMultipartUploadsOutput(QsError err, Http::HttpResponseCode responseCode):QsOutput(err, responseCode){};
	ListMultipartUploadsOutput(){};
		
	// Other object keys that share common prefixes
	inline void SetCommonPrefixes(
	std::vector<std::string > CommonPrefixes)
	{

		m_CommonPrefixes = CommonPrefixes;
	};

	inline 
	std::vector<std::string > GetCommonPrefixes(){return m_CommonPrefixes;};
	
	// Delimiter that specified in request parameters
	inline void SetDelimiter(std::string  Delimiter)
	{

		m_Delimiter = Delimiter;
	};

	inline std::string  GetDelimiter(){return m_Delimiter;};
	
	// Limit that specified in request parameters
	inline void SetLimit(int  Limit)
	{

		m_Limit = Limit;
	};

	inline int  GetLimit(){return m_Limit;};
	
	// Marker that specified in request parameters
	inline void SetMarker(std::string  Marker)
	{

		m_Marker = Marker;
	};

	inline std::string  GetMarker(){return m_Marker;};
	
	// Bucket name
	inline void SetName(std::string  Name)
	{

		m_Name = Name;
	};

	inline std::string  GetName(){return m_Name;};
	
	// The last key in keys list
	inline void SetNextMarker(std::string  NextMarker)
	{

		m_NextMarker = NextMarker;
	};

	inline std::string  GetNextMarker(){return m_NextMarker;};
	
	// Prefix that specified in request parameters
	inline void SetPrefix(std::string  Prefix)
	{

		m_Prefix = Prefix;
	};

	inline std::string  GetPrefix(){return m_Prefix;};
	
	// Multipart uploads
	inline void SetUploads(
	std::vector<UploadsType > Uploads)
	{

		m_Uploads = Uploads;
	};

	inline 
	std::vector<UploadsType > GetUploads(){return m_Uploads;};
	
	private:
	// Other object keys that share common prefixes
	
	std::vector<std::string > m_CommonPrefixes;
	
	// Delimiter that specified in request parameters
	std::string  m_Delimiter;
	
	// Limit that specified in request parameters
	int  m_Limit;
	
	// Marker that specified in request parameters
	std::string  m_Marker;
	
	// Bucket name
	std::string  m_Name;
	
	// The last key in keys list
	std::string  m_NextMarker;
	
	// Prefix that specified in request parameters
	std::string  m_Prefix;
	
	// Multipart uploads
	
	std::vector<UploadsType > m_Uploads;
	

	
};
        
// ListObjectsOutput presents input for ListObjects.
class QS_SDK_API ListObjectsOutput: public QsOutput
{	
public:
	ListObjectsOutput(QsError err, Http::HttpResponseCode responseCode):QsOutput(err, responseCode){};
	ListObjectsOutput(){};
		
	// Other object keys that share common prefixes
	inline void SetCommonPrefixes(
	std::vector<std::string > CommonPrefixes)
	{

		m_CommonPrefixes = CommonPrefixes;
	};

	inline 
	std::vector<std::string > GetCommonPrefixes(){return m_CommonPrefixes;};
	
	// Delimiter that specified in request parameters
	inline void SetDelimiter(std::string  Delimiter)
	{

		m_Delimiter = Delimiter;
	};

	inline std::string  GetDelimiter(){return m_Delimiter;};
	
	// Object keys
	inline void SetKeys(
	std::vector<KeyType > Keys)
	{

		m_Keys = Keys;
	};

	inline 
	std::vector<KeyType > GetKeys(){return m_Keys;};
	
	// Limit that specified in request parameters
	inline void SetLimit(int  Limit)
	{

		m_Limit = Limit;
	};

	inline int  GetLimit(){return m_Limit;};
	
	// Marker that specified in request parameters
	inline void SetMarker(std::string  Marker)
	{

		m_Marker = Marker;
	};

	inline std::string  GetMarker(){return m_Marker;};
	
	// Bucket name
	inline void SetName(std::string  Name)
	{

		m_Name = Name;
	};

	inline std::string  GetName(){return m_Name;};
	
	// The last key in keys list
	inline void SetNextMarker(std::string  NextMarker)
	{

		m_NextMarker = NextMarker;
	};

	inline std::string  GetNextMarker(){return m_NextMarker;};
	
	// Bucket owner
	inline void SetOwner(
	OwnerType Owner)
	{

		m_Owner = Owner;
	};

	inline 
	OwnerType GetOwner(){return m_Owner;};
	
	// Prefix that specified in request parameters
	inline void SetPrefix(std::string  Prefix)
	{

		m_Prefix = Prefix;
	};

	inline std::string  GetPrefix(){return m_Prefix;};
	
	private:
	// Other object keys that share common prefixes
	
	std::vector<std::string > m_CommonPrefixes;
	
	// Delimiter that specified in request parameters
	std::string  m_Delimiter;
	
	// Object keys
	
	std::vector<KeyType > m_Keys;
	
	// Limit that specified in request parameters
	int  m_Limit;
	
	// Marker that specified in request parameters
	std::string  m_Marker;
	
	// Bucket name
	std::string  m_Name;
	
	// The last key in keys list
	std::string  m_NextMarker;
	
	// Bucket owner
	
	OwnerType m_Owner;
	
	// Prefix that specified in request parameters
	std::string  m_Prefix;
	

	
};
        
typedef QsOutput PutBucketOutput;
        
typedef QsOutput PutBucketACLOutput;
        
typedef QsOutput PutBucketCORSOutput;
        
typedef QsOutput PutBucketExternalMirrorOutput;
        
typedef QsOutput PutBucketPolicyOutput;
// +--------------------------------------------------------------------
// |                     InputClassHeader                              
// +--------------------------------------------------------------------
        
typedef QsOutput AbortMultipartUploadOutput;
        
typedef QsOutput CompleteMultipartUploadOutput;
        
typedef QsOutput DeleteObjectOutput;
        
// GetObjectOutput presents input for GetObject.
class QS_SDK_API GetObjectOutput: public QsOutput
{	
public:
	GetObjectOutput(QsError err, Http::HttpResponseCode responseCode):QsOutput(err, responseCode){};
	GetObjectOutput(){};
		
	// Object content length
	inline void SetContentLength(int64_t  ContentLength)
	{

		m_ContentLength = ContentLength;
	};

	inline int64_t  GetContentLength(){return m_ContentLength;};
	
	// Range of response data content
	inline void SetContentRange(std::string  ContentRange)
	{

		m_ContentRange = ContentRange;
	};

	inline std::string  GetContentRange(){return m_ContentRange;};
	
	// MD5sum of the object
	inline void SetETag(std::string  ETag)
	{

		m_ETag = ETag;
	};

	inline std::string  GetETag(){return m_ETag;};
	
	// Encryption algorithm of the object
	inline void SetXQSEncryptionCustomerAlgorithm(std::string  XQSEncryptionCustomerAlgorithm)
	{

		m_XQSEncryptionCustomerAlgorithm = XQSEncryptionCustomerAlgorithm;
	};

	inline std::string  GetXQSEncryptionCustomerAlgorithm(){return m_XQSEncryptionCustomerAlgorithm;};
	
	
		std::shared_ptr<std::iostream> GetBody(){ return m_streambody;};
		void SetBody(std::shared_ptr<std::iostream> streambody){ m_streambody = streambody;};
	private:
	// Object content length
	int64_t  m_ContentLength;
	
	// Range of response data content
	std::string  m_ContentRange;
	
	// MD5sum of the object
	std::string  m_ETag;
	
	// Encryption algorithm of the object
	std::string  m_XQSEncryptionCustomerAlgorithm;
	

	
		std::shared_ptr<std::iostream> m_streambody;
	
};
        
// HeadObjectOutput presents input for HeadObject.
class QS_SDK_API HeadObjectOutput: public QsOutput
{	
public:
	HeadObjectOutput(QsError err, Http::HttpResponseCode responseCode):QsOutput(err, responseCode){};
	HeadObjectOutput(){};
		
	// Object content length
	inline void SetContentLength(int64_t  ContentLength)
	{

		m_ContentLength = ContentLength;
	};

	inline int64_t  GetContentLength(){return m_ContentLength;};
	
	// Object content type
	inline void SetContentType(std::string  ContentType)
	{

		m_ContentType = ContentType;
	};

	inline std::string  GetContentType(){return m_ContentType;};
	
	// MD5sum of the object
	inline void SetETag(std::string  ETag)
	{

		m_ETag = ETag;
	};

	inline std::string  GetETag(){return m_ETag;};
	
	inline void SetLastModified(std::string  LastModified)
	{

		m_LastModified = LastModified;
	};

	inline std::string  GetLastModified(){return m_LastModified;};
	
	// Encryption algorithm of the object
	inline void SetXQSEncryptionCustomerAlgorithm(std::string  XQSEncryptionCustomerAlgorithm)
	{

		m_XQSEncryptionCustomerAlgorithm = XQSEncryptionCustomerAlgorithm;
	};

	inline std::string  GetXQSEncryptionCustomerAlgorithm(){return m_XQSEncryptionCustomerAlgorithm;};
	
	private:
	// Object content length
	int64_t  m_ContentLength;
	
	// Object content type
	std::string  m_ContentType;
	
	// MD5sum of the object
	std::string  m_ETag;
	
	std::string  m_LastModified;
	
	// Encryption algorithm of the object
	std::string  m_XQSEncryptionCustomerAlgorithm;
	

	
};
        
// InitiateMultipartUploadOutput presents input for InitiateMultipartUpload.
class QS_SDK_API InitiateMultipartUploadOutput: public QsOutput
{	
public:
	InitiateMultipartUploadOutput(QsError err, Http::HttpResponseCode responseCode):QsOutput(err, responseCode){};
	InitiateMultipartUploadOutput(){};
		
	// Encryption algorithm of the object
	inline void SetXQSEncryptionCustomerAlgorithm(std::string  XQSEncryptionCustomerAlgorithm)
	{

		m_XQSEncryptionCustomerAlgorithm = XQSEncryptionCustomerAlgorithm;
	};

	inline std::string  GetXQSEncryptionCustomerAlgorithm(){return m_XQSEncryptionCustomerAlgorithm;};
	
	
		
	// Bucket name
	inline void SetBucket(std::string  Bucket)
	{

		m_Bucket = Bucket;
	};

	inline std::string  GetBucket(){return m_Bucket;};
	
	// Object key
	inline void SetKey(std::string  Key)
	{

		m_Key = Key;
	};

	inline std::string  GetKey(){return m_Key;};
	
	// Object multipart upload ID
	inline void SetUploadID(std::string  UploadID)
	{

		m_UploadID = UploadID;
	};

	inline std::string  GetUploadID(){return m_UploadID;};
	
	private:
	// Encryption algorithm of the object
	std::string  m_XQSEncryptionCustomerAlgorithm;
	

	
	// Bucket name
	std::string  m_Bucket;
	
	// Object key
	std::string  m_Key;
	
	// Object multipart upload ID
	std::string  m_UploadID;
	

	
};
        
// ListMultipartOutput presents input for ListMultipart.
class QS_SDK_API ListMultipartOutput: public QsOutput
{	
public:
	ListMultipartOutput(QsError err, Http::HttpResponseCode responseCode):QsOutput(err, responseCode){};
	ListMultipartOutput(){};
		
	// Object multipart count
	inline void SetCount(int  Count)
	{

		m_Count = Count;
	};

	inline int  GetCount(){return m_Count;};
	
	// Object parts
	inline void SetObjectParts(
	std::vector<ObjectPartType > ObjectParts)
	{

		m_ObjectParts = ObjectParts;
	};

	inline 
	std::vector<ObjectPartType > GetObjectParts(){return m_ObjectParts;};
	
	private:
	// Object multipart count
	int  m_Count;
	
	// Object parts
	
	std::vector<ObjectPartType > m_ObjectParts;
	

	
};
        
// OptionsObjectOutput presents input for OptionsObject.
class QS_SDK_API OptionsObjectOutput: public QsOutput
{	
public:
	OptionsObjectOutput(QsError err, Http::HttpResponseCode responseCode):QsOutput(err, responseCode){};
	OptionsObjectOutput(){};
		
	// Allowed headers
	inline void SetAccessControlAllowHeaders(std::string  AccessControlAllowHeaders)
	{

		m_AccessControlAllowHeaders = AccessControlAllowHeaders;
	};

	inline std::string  GetAccessControlAllowHeaders(){return m_AccessControlAllowHeaders;};
	
	// Allowed methods
	inline void SetAccessControlAllowMethods(std::string  AccessControlAllowMethods)
	{

		m_AccessControlAllowMethods = AccessControlAllowMethods;
	};

	inline std::string  GetAccessControlAllowMethods(){return m_AccessControlAllowMethods;};
	
	// Allowed origin
	inline void SetAccessControlAllowOrigin(std::string  AccessControlAllowOrigin)
	{

		m_AccessControlAllowOrigin = AccessControlAllowOrigin;
	};

	inline std::string  GetAccessControlAllowOrigin(){return m_AccessControlAllowOrigin;};
	
	// Expose headers
	inline void SetAccessControlExposeHeaders(std::string  AccessControlExposeHeaders)
	{

		m_AccessControlExposeHeaders = AccessControlExposeHeaders;
	};

	inline std::string  GetAccessControlExposeHeaders(){return m_AccessControlExposeHeaders;};
	
	// Max age
	inline void SetAccessControlMaxAge(std::string  AccessControlMaxAge)
	{

		m_AccessControlMaxAge = AccessControlMaxAge;
	};

	inline std::string  GetAccessControlMaxAge(){return m_AccessControlMaxAge;};
	
	private:
	// Allowed headers
	std::string  m_AccessControlAllowHeaders;
	
	// Allowed methods
	std::string  m_AccessControlAllowMethods;
	
	// Allowed origin
	std::string  m_AccessControlAllowOrigin;
	
	// Expose headers
	std::string  m_AccessControlExposeHeaders;
	
	// Max age
	std::string  m_AccessControlMaxAge;
	

	
};
        
typedef QsOutput PutObjectOutput;
        
typedef QsOutput UploadMultipartOutput; 

// +--------------------------------------------------------------------
// |                           Bucket                              
// +-------------------------------------------------------------------- 
class QS_SDK_API Bucket
{
    public:
        Bucket(const QsConfig qsConfig, const std::string strBucketName, const std::string strZone);

        virtual ~Bucket(){};

	// Delete does Delete a bucket.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/bucket/delete.html
	QsError deleteBucket(DeleteBucketInput& input, DeleteBucketOutput& output);


	// DeleteCORS does Delete CORS information of the bucket.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/bucket/cors/delete_cors.html
	QsError deleteBucketCORS(DeleteBucketCORSInput& input, DeleteBucketCORSOutput& output);


	// DeleteExternalMirror does Delete external mirror of the bucket.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/bucket/external_mirror/delete_external_mirror.html
	QsError deleteBucketExternalMirror(DeleteBucketExternalMirrorInput& input, DeleteBucketExternalMirrorOutput& output);


	// DeletePolicy does Delete policy information of the bucket.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/bucket/policy/delete_policy.html
	QsError deleteBucketPolicy(DeleteBucketPolicyInput& input, DeleteBucketPolicyOutput& output);


	// DeleteMultipleObjects does Delete multiple objects from the bucket.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/bucket/delete_multiple.html
	QsError deleteMultipleObjects(DeleteMultipleObjectsInput& input, DeleteMultipleObjectsOutput& output);


	// GetACL does Get ACL information of the bucket.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/bucket/get_acl.html
	QsError getBucketACL(GetBucketACLInput& input, GetBucketACLOutput& output);


	// GetCORS does Get CORS information of the bucket.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/bucket/cors/get_cors.html
	QsError getBucketCORS(GetBucketCORSInput& input, GetBucketCORSOutput& output);


	// GetExternalMirror does Get external mirror of the bucket.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/bucket/external_mirror/get_external_mirror.html
	QsError getBucketExternalMirror(GetBucketExternalMirrorInput& input, GetBucketExternalMirrorOutput& output);


	// GetPolicy does Get policy information of the bucket.
	// Documentation URL: https://https://docs.qingcloud.com/qingstor/api/bucket/policy/get_policy.html
	QsError getBucketPolicy(GetBucketPolicyInput& input, GetBucketPolicyOutput& output);


	// GetStatistics does Get statistics information of the bucket.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/bucket/get_stats.html
	QsError getBucketStatistics(GetBucketStatisticsInput& input, GetBucketStatisticsOutput& output);


	// Head does Check whether the bucket exists and available.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/bucket/head.html
	QsError headBucket(HeadBucketInput& input, HeadBucketOutput& output);


	// ListMultipartUploads does List multipart uploads in the bucket.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/bucket/list_multipart_uploads.html
	QsError listMultipartUploads(ListMultipartUploadsInput& input, ListMultipartUploadsOutput& output);


	// ListObjects does Retrieve the object list in a bucket.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/bucket/get.html
	QsError listObjects(ListObjectsInput& input, ListObjectsOutput& output);


	// Put does Create a new bucket.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/bucket/put.html
	QsError putBucket(PutBucketInput& input, PutBucketOutput& output);


	// PutACL does Set ACL information of the bucket.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/bucket/put_acl.html
	QsError putBucketACL(PutBucketACLInput& input, PutBucketACLOutput& output);


	// PutCORS does Set CORS information of the bucket.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/bucket/cors/put_cors.html
	QsError putBucketCORS(PutBucketCORSInput& input, PutBucketCORSOutput& output);


	// PutExternalMirror does Set external mirror of the bucket.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/bucket/external_mirror/put_external_mirror.html
	QsError putBucketExternalMirror(PutBucketExternalMirrorInput& input, PutBucketExternalMirrorOutput& output);


	// PutPolicy does Set policy information of the bucket.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/bucket/policy/put_policy.html
	QsError putBucketPolicy(PutBucketPolicyInput& input, PutBucketPolicyOutput& output);


	// AbortMultipartUpload does Abort multipart upload.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/object/abort_multipart_upload.html
	QsError abortMultipartUpload(std::string objectKey, AbortMultipartUploadInput& input, AbortMultipartUploadOutput& output);


	// CompleteMultipartUpload does Complete multipart upload.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/object/complete_multipart_upload.html
	QsError completeMultipartUpload(std::string objectKey, CompleteMultipartUploadInput& input, CompleteMultipartUploadOutput& output);


	// DeleteObject does Delete the object.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/object/delete.html
	QsError deleteObject(std::string objectKey, DeleteObjectInput& input, DeleteObjectOutput& output);


	// GetObject does Retrieve the object.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/object/get.html
	QsError getObject(std::string objectKey, GetObjectInput& input, GetObjectOutput& output);


	// HeadObject does Check whether the object exists and available.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/object/head.html
	QsError headObject(std::string objectKey, HeadObjectInput& input, HeadObjectOutput& output);


	// InitiateMultipartUpload does Initial multipart upload on the object.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/object/initiate_multipart_upload.html
	QsError initiateMultipartUpload(std::string objectKey, InitiateMultipartUploadInput& input, InitiateMultipartUploadOutput& output);


	// ListMultipart does List object parts.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/object/list_multipart.html
	QsError listMultipart(std::string objectKey, ListMultipartInput& input, ListMultipartOutput& output);


	// OptionsObject does Check whether the object accepts a origin with method and header.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/object/options.html
	QsError optionsObject(std::string objectKey, OptionsObjectInput& input, OptionsObjectOutput& output);


	// PutObject does Upload the object.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/object/put.html
	QsError putObject(std::string objectKey, PutObjectInput& input, PutObjectOutput& output);


	// UploadMultipart does Upload object multipart.
	// Documentation URL: https://docs.qingcloud.com/qingstor/api/object/multipart/upload_multipart.html
	QsError uploadMultipart(std::string objectKey, UploadMultipartInput& input, UploadMultipartOutput& output);

    private:
        QsConfig m_qsConfig;
        Properties m_properties;

};


}// namespace QingStor












































