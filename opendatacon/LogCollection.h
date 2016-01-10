/*	opendatacon
 *
 *	Copyright (c) 2014:
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
 */
//
//  ResponderMap.h
//  opendatacon
//
//  Created by Alan Murray on 14/09/2014.
//
//

#ifndef opendatacon_LogCollection_h
#define opendatacon_LogCollection_h

#include "logging_cmds.h"
#include <opendatacon/IUIResponder.h>

namespace ODC
{/*
    class LogCollection: public ResponderMap<AdvancedLogger>
    {
    public:
        LogCollection()
        {
            this->AddCommand("ignore", [this](const ParamCollection &params) {
                if(auto target = GetTarget(params))
                {
                    std::stringstream filter;
                    filter << params.at("filter");
                    
                    std::string mregex;
                    if(!extract_delimited_string(filter,mregex))
                    {
                        return IUIResponder::GenerateResult("Syntax error: Delimited regex expected, found \"..." + mregex + "\"");
                    }
                    if(mregex == "")
                    {
                        return IUIResponder::GenerateResult("Please supply a regex filter");
                    }
                    target->AddIngoreAlways(mregex);
                    return IUIResponder::GenerateResult("Success");
                }
                return IUIResponder::GenerateResult("Bad parameter");
            }, "Enter regex to silence matching messages from the logger.");
            this->AddCommand("unignore", [this](const ParamCollection &params) {
                if(auto target = GetTarget(params))
                {
                    std::stringstream filter;
                    filter << params.at("filter");
                    cmd_unignore_message(filter,*target);
                    return IUIResponder::GenerateResult("Success");
                }
                return IUIResponder::GenerateResult("Bad parameter");
            }, "Enter regex to remove from the ignore list.");
            this->AddCommand("ShowFilters", [this](const ParamCollection &params) {
                if(auto target = GetTarget(params))
                {
                    return target->ShowIgnored();
                }
                return IUIResponder::GenerateResult("Bad parameter");
            }, "Shows all message ignore regexes and how many messages they've matched.");
        }
        virtual ~LogCollection(){};
    };*/
}

#endif
