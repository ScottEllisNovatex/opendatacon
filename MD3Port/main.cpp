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
 * main.cpp
 *
 *  Created on: 16/10/2014
 *      Author: Alan Murray
 */

#include "MD3OutstationPort.h"
#include "MD3MasterPort.h"

extern "C" MD3MasterPort* new_MD3MasterPort(std::string Name, std::string File, const Json::Value Overrides)
{
	return nullptr;
//	return new MD3MasterPort(Name,File,Overrides);
}

extern "C" MD3OutstationPort* new_MD3OutstationPort(std::string Name, std::string File, const Json::Value Overrides)
{
	return new MD3OutstationPort(Name,File,Overrides);
}

extern "C" void delete_MD3MasterPort(MD3MasterPort* aMD3MasterPort_ptr)
{
	// delete aMD3MasterPort_ptr;
	return;
}

extern "C" void delete_MD3OutstationPort(MD3OutstationPort* aMD3OutstationPort_ptr)
{
	delete aMD3OutstationPort_ptr;
	return;
}