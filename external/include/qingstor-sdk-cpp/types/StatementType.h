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
#include "ConditionType.h"

// StatementType presents costom type：Statement.
class QS_SDK_API StatementType: QsBaseType
{
public:
	StatementType(){};
	StatementType(std::string serializedStr);// QingStor API methods
	inline void SetAction(
	std::vector<std::string > Action)
	{
		m_propsHasBeenSet.push_back("action");
		m_Action = Action;
	};

	inline 
	std::vector<std::string > GetAction(){return m_Action;};

	
	inline void SetCondition(
	ConditionType Condition)
	{
		m_propsHasBeenSet.push_back("condition");
		m_Condition = Condition;
	};

	inline 
	ConditionType GetCondition(){return m_Condition;};

	// Statement effect
	// Effect's available values: allow, deny
	inline void SetEffect(std::string  Effect)
	{
		m_propsHasBeenSet.push_back("effect");
		m_Effect = Effect;
	};

	inline std::string  GetEffect(){return m_Effect;};

	// Bucket policy id, must be unique
	inline void SetID(std::string  ID)
	{
		m_propsHasBeenSet.push_back("id");
		m_ID = ID;
	};

	inline std::string  GetID(){return m_ID;};

	// The resources to apply bucket policy
	inline void SetResource(
	std::vector<std::string > Resource)
	{
		m_propsHasBeenSet.push_back("resource");
		m_Resource = Resource;
	};

	inline 
	std::vector<std::string > GetResource(){return m_Resource;};

	// The user to apply bucket policy
	inline void SetUser(
	std::vector<std::string > User)
	{
		m_propsHasBeenSet.push_back("user");
		m_User = User;
	};

	inline 
	std::vector<std::string > GetUser(){return m_User;};

	std::string Serialize();

private:		
	
	// QingStor API methods
	
	std::vector<std::string > m_Action;// Required
	
	
	ConditionType m_Condition;
	
	// Statement effect
	// Effect's available values: allow, deny
	std::string  m_Effect;// Required
	
	// Bucket policy id, must be unique
	std::string  m_ID;// Required
	
	// The resources to apply bucket policy
	
	std::vector<std::string > m_Resource;
	
	// The user to apply bucket policy
	
	std::vector<std::string > m_User;// Required
	

};












































