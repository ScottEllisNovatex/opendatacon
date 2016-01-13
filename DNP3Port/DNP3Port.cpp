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
 * DNP3Port.cpp
 *
 *  Created on: 18/07/2014
 *      Author: Neil Stephens <dearknarl@gmail.com>
 */
#include "DNP3Port.h"
#include <iostream>
#include "DNP3PortConf.h"
#include <asiopal/PhysicalLayerTCPServer.h>
#include <asiopal/PhysicalLayerTCPClient.h>

std::unordered_map<std::string, asiodnp3::IChannel*> DNP3Port::TCPServers;
std::unordered_map<std::string, asiodnp3::IChannel*> DNP3Port::TCPClients;
asiodnp3::ChannelSet* DNP3Port::channels(new asiodnp3::ChannelSet());

DNP3Port::DNP3Port(std::string& aName, Context& aParent, std::string& aConfFilename, const Json::Value& aConfOverrides):
	DataPort(aName, aParent, aConfFilename, aConfOverrides),
	LogWrapper(aName, *this),
	pChannel(nullptr),
	status(opendnp3::LinkStatus::UNRESET),
	link_dead(true)
{
	//the creation of a new DNP3PortConf will get the point details
	pConf.reset(new DNP3PortConf(ConfFilename));

	//We still may need to process the file (or overrides) to get Addr details:
	ProcessFile();
}

//DataPort function for UI
const Json::Value DNP3Port::GetStatus() const
{
	auto ret_val = Json::Value();

	if(!enabled)
		ret_val["Result"] = "Port disabled";
	else if(link_dead)
		ret_val["Result"] = "Port enabled - link down";
	else if(status == opendnp3::LinkStatus::RESET)
		ret_val["Result"] = "Port enabled - link up (reset)";
	else if(status == opendnp3::LinkStatus::UNRESET)
		ret_val["Result"] = "Port enabled - link up (unreset)";
	else
		ret_val["Result"] = "Port enabled - link status unknown";

	return ret_val;
}

void DNP3Port::ProcessElements(const Json::Value& JSONRoot)
{
	if(!JSONRoot.isObject()) return;
	if(!JSONRoot["IP"].isNull())
		static_cast<DNP3PortConf*>(pConf.get())->mAddrConf.IP = JSONRoot["IP"].asString();

	if(!JSONRoot["Port"].isNull())
		static_cast<DNP3PortConf*>(pConf.get())->mAddrConf.Port = JSONRoot["Port"].asUInt();

	if(!JSONRoot["OutstationAddr"].isNull())
		static_cast<DNP3PortConf*>(pConf.get())->mAddrConf.OutstationAddr = JSONRoot["OutstationAddr"].asUInt();

	if(!JSONRoot["MasterAddr"].isNull())
		static_cast<DNP3PortConf*>(pConf.get())->mAddrConf.MasterAddr = JSONRoot["MasterAddr"].asUInt();

	if(!JSONRoot["ServerType"].isNull())
	{
		if(JSONRoot["ServerType"].asString() == "ONDEMAND")
			static_cast<DNP3PortConf*>(pConf.get())->mAddrConf.ServerType = server_type_t::ONDEMAND;
		else if(JSONRoot["ServerType"].asString() == "PERSISTENT")
			static_cast<DNP3PortConf*>(pConf.get())->mAddrConf.ServerType = server_type_t::PERSISTENT;
		else if(JSONRoot["ServerType"].asString() == "MANUAL")
			static_cast<DNP3PortConf*>(pConf.get())->mAddrConf.ServerType = server_type_t::MANUAL;
		else
			std::cout<<"Invalid DNP3 Port server type: '"<<JSONRoot["ServerType"].asString()<<"'."<<std::endl;
	}

}

asiodnp3::IChannel* DNP3Port::getTCPServer(const DNP3AddrConf& mAddrConf)
{
	auto IPPort = "TCPS_" + mAddrConf.IP +":"+ std::to_string(mAddrConf.Port);

	//create a new channel if one isn't already up
	if(!TCPServers.count(IPPort))
	{
		auto pRoot = new openpal::LogRoot(&this->LogWrapper, IPPort.c_str(), opendnp3::levels::ALL);
		auto pPhys = new asiopal::PhysicalLayerTCPServer(*pRoot, *this->GetIOService(), mAddrConf.IP, mAddrConf.Port);
		TCPServers[IPPort] = channels->CreateChannel(pRoot, pPhys->executor, opendnp3::ChannelRetry::Default(), pPhys, nullptr);
	}
	return TCPServers[IPPort];
}

asiodnp3::IChannel* DNP3Port::getTCPClient(const DNP3AddrConf& mAddrConf)
{
	auto IPPort = "TCPC_" + mAddrConf.IP +":"+ std::to_string(mAddrConf.Port);

	//create a new channel if one isn't already up
	if(!TCPClients.count(IPPort))
	{
		auto pRoot = new openpal::LogRoot(&this->LogWrapper, IPPort.c_str(), opendnp3::levels::ALL);
		auto pPhys = new asiopal::PhysicalLayerTCPClient(*pRoot, *this->GetIOService(), mAddrConf.IP, "0.0.0.0", mAddrConf.Port);
		TCPClients[IPPort] = channels->CreateChannel(pRoot, pPhys->executor, opendnp3::ChannelRetry::Default(), pPhys, nullptr);
	}
	return TCPClients[IPPort];
}
