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
/*
 * LuaWebPortConf.h
 *
 *  Created on: 13/08/2024
 *      Author: Scott Ellis
 */

#ifndef LuaWebPortConf_H_
#define LuaWebPortConf_H_
#include <opendatacon/DataPort.h>

class LuaWebPortConf: public DataPortConf
{
public:
	std::string LuaFile;
	std::string ip = "0.0.0.0";
	uint16_t port = 443;
	std::string web_crt = "server.crt";
	std::string web_key = "server.key";
};

#endif /* LuaWebPortConf_H_ */
