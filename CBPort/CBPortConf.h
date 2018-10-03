/*	opendatacon
 *
 *	Copyright (c) 2018:
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
 * CBOutstationPortConf.h
 *
 *  Created on: 13/09/2018
 *      Author: Scott Ellis <scott.ellis@novatex.com.au> - derived from DNP3 version
 */

#ifndef CBOUTSTATIONPORTCONF_H_
#define CBOUTSTATIONPORTCONF_H_

#include <opendatacon/DataPort.h>
#include <opendatacon/TCPSocketManager.h>
#include "CBPointConf.h"
#include "CBPointTableAccess.h"
#include "CBConnection.h"

// Megadata System Flag register definition bits
#define SYSTEMPOWERUPFLAGBIT 15
#define SYSTEMTIMEINCORRECTFLAGBIT 14
#define FILEUPLOADPENDINGFLAGBIT 13

enum TCPClientServer { CLIENT, SERVER };
enum server_type_t { ONDEMAND, PERSISTENT, MANUAL };

enum class SerialParity: char
{
	NONE='N',EVEN='E',ODD='O'
};

struct CBAddrConf
{
	//IP
	std::string IP;
	std::string Port;
	TCPClientServer ClientServer;

	//Common
	uint8_t OutstationAddr;
	uint16_t TCPConnectRetryPeriodms;

	// Default address values can minimally set IP.
	CBAddrConf():
		IP("127.0.0.1"),
		Port("20000"),
		ClientServer(TCPClientServer::SERVER),
		OutstationAddr(1),
		TCPConnectRetryPeriodms(500)
	{}
	uint64_t ChannelID()
	{
		if (channelid == 0)
			channelid = CBConnection::MakeChannelID(IP, Port, (ClientServer == SERVER));
		return channelid;
	}

private:
	uint64_t channelid = 0;
};

class CBPortConf: public DataPortConf
{
public:
	CBPortConf(std::string FileName, const Json::Value& ConfOverrides)
	{
		pPointConf.reset(new CBPointConf(FileName, ConfOverrides));
	}

	std::shared_ptr<CBPointConf> pPointConf;
	CBAddrConf mAddrConf;
};

#endif
