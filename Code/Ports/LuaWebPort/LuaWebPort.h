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
 * LuaWebPort.h
 *
 *  Created on: 13/08/2024
 *      Author: Scott Ellis
 */

#ifndef LuaWebPort_H_
#define LuaWebPort_H_

#include <Lua/DynamicSymbols.h>
#include <Lua/CLua.h>
#include <opendatacon/DataPort.h>
#include <WebHelpers.h>
#include <opendatacon/TCPSocketManager.h>
#include <regex>
#include <shared_mutex>
#include <queue>
#include <string_view>

using namespace odc;

class LuaWebPort: public DataPort
{
public:
	LuaWebPort(const std::string& aName, const std::string& aConfFilename, const Json::Value& aConfOverrides);
	~LuaWebPort();

	void Enable() override
	{
		pLuaSyncStrand->post([this,h{handler_tracker}](){Enable_();});
	}

	void Disable() override
	{
		pLuaSyncStrand->post([this,h{handler_tracker}](){Disable_();});
	}

	void Build() override;

	void Event(std::shared_ptr<const EventInfo> event, const std::string& SenderName, SharedStatusCallback_t pStatusCallback) override
	{
		pLuaSyncStrand->post([=,h{handler_tracker}](){Event_(event,SenderName,pStatusCallback);});
	}

	void ProcessElements(const Json::Value& JSONRoot) override;
	void RegisterRequestHandler(const std::string& urlpattern, const std::string& method, const std::string& LuaHandlerName);

private:
	void DefaultRequestHandler(std::shared_ptr<WebServer::Response> response, std::shared_ptr<WebServer::Request> request);

	std::shared_ptr <WebServer> WebSrv;
	Lua::DynamicSymbols lua_syms; //in case lua modules need to resolve symbols

	//copy this to posted handlers so we can manage lifetime
	std::shared_ptr<void> handler_tracker = std::make_shared<char>();
	std::shared_ptr<asio::io_service::strand> pLuaSyncStrand = pIOS->make_strand();

	//synchronised versions of public counterparts above
	void Enable_();
	void Disable_();
	void Event_(std::shared_ptr<const EventInfo> event, const std::string& SenderName, SharedStatusCallback_t pStatusCallback);

	void ExportLuaPublishEvent();
	void ExportLuaRegisterRequestHandler();
	void ExportLuaInDemand();
	void CallLuaGlobalVoidVoidFunc(const std::string& FnName);

	lua_State* LuaState = luaL_newstate();
	Json::Value JSONConf;
};

#endif /* LuaWebPort_H_ */
