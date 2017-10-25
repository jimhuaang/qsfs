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

#include  "QsSdkConfig.h"
#include  <string>
#include  <vector>
#include  <algorithm>

class QS_SDK_API QsBaseType{
public:
	QsBaseType(){};

	virtual ~QsBaseType() {};

    inline  bool IsPropHasBeenSet(std::string strPropName)
    {
        auto it = std::find( m_propsHasBeenSet.begin(), m_propsHasBeenSet.end(), strPropName );
        if( it != m_propsHasBeenSet.end() ) // finded
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    virtual bool CheckIfInputIsVaild(){ return true;};
protected:
    std::vector<std::string> m_propsHasBeenSet;
};
