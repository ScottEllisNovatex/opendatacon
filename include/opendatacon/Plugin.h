/*	opendatacon
 *
 *	Copyright (c) 2016:
 *
 *		DCrip3fJguWgVCLrZFfA7sIGgvx1Ou3fHfCxnrz4svAi
 *		yxeOtDhDCXf1Z4ApgXvX5ahqQmzRfJ2DoX8S05SqHA==
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 *  Created by Alan Murray on 09/01/2016.
 */

#ifndef opendatacon_suite_Plugin_h
#define opendatacon_suite_Plugin_h

#include "ConfigParser.h"

namespace ODC
{
class Plugin: public ConfigParser
{
public:
	Plugin(const std::string& aName, const std::string& aConfFilename, const Json::Value aConfOverrides):
		ConfigParser(aConfFilename, aConfOverrides)
	{};
	virtual void BuildOrRebuild() = 0;
	virtual void Enable() = 0;
	virtual void Disable() = 0;
private:
};
}

#endif
