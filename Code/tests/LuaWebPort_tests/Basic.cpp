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
 *  Created on: 17/06/2023
 *      Author: Neil Stephens <dearknarl@gmail.com>
 */

#include <client_https.hpp>
#include "Helpers.h"
#include "../PortLoader.h"
#include "../../../opendatacon/NullPort.h"
#include <catch.hpp>
#include <opendatacon/asio.h>
#include <thread>
#include <string>

#define SUITE(name) "LuaWebPortBasicsTestSuite - " name

using HttpsClient = SimpleWeb::Client<SimpleWeb::HTTPS>;

//TEST_CASE(SUITE("ConstructBuildEnableDisableDestroy"))
//{
//	TestSetup();
//	auto portlib = LoadModule(GetLibFileName("LuaWebPort"));
//	REQUIRE(portlib);
//
//	//Go through the typical life cycle of a port, albeit short
//	{
//		newptr newPort = GetPortCreator(portlib, "LuaWeb");
//		REQUIRE(newPort);
//		delptr deletePort = GetPortDestroyer(portlib, "LuaWeb");
//		REQUIRE(deletePort);
//
//		std::shared_ptr<DataPort> PUT(newPort("PortUnderTest", "", GetConfigJSON()), deletePort);
//
//		NullPort Null("Null","","");
//		PUT->Subscribe(&Null,"Null");
//		Null.Subscribe(PUT.get(),"PortUnderTest");
//
//		PUT->Build();
//		Null.Build();
//
//		auto ios = odc::asio_service::Get();
//		auto work = ios->make_work();
//		std::thread t([ios](){ios->run();});
//
//		PUT->Enable();
//		Null.Enable();
//		auto event = std::make_shared<EventInfo>(EventType::OctetString,123);
//		event->SetPayload<EventType::OctetString>(OctetStringBuffer(std::string("test event")));
//		PUT->Event(event,"test_harness",std::make_shared<std::function<void (CommandStatus)>>([] (CommandStatus){}));
//		auto event2 = std::make_shared<EventInfo>(EventType::ControlRelayOutputBlock,666);
//		event2->SetPayload<EventType::ControlRelayOutputBlock>(ControlRelayOutputBlock());
//		PUT->Event(event2,"test_harness",std::make_shared<std::function<void (CommandStatus)>>([] (CommandStatus){}));
//		PUT->Disable();
//
//		work.reset();
//		ios->run();
//		t.join();
//		ios.reset();
//	}
//
//	TestTearDown();
//	UnLoadModule(portlib);
//}

std::string rbuftostr(std::streambuf *rdbuf)
{
	std::ostringstream ss;
	ss << rdbuf;
	return ss.str();
}

TEST_CASE(SUITE("WebRequest"))
{
	TestSetup();
	auto portlib = LoadModule(GetLibFileName("LuaWebPort"));
	REQUIRE(portlib);

	//Go through the typical life cycle of a port, albeit short
	{
		newptr newPort = GetPortCreator(portlib, "LuaWeb");
		REQUIRE(newPort);
		delptr deletePort = GetPortDestroyer(portlib, "LuaWeb");
		REQUIRE(deletePort);

		std::shared_ptr<DataPort> PUT(newPort("PortUnderTest", "", GetConfigJSON()), deletePort);

		NullPort Null("Null", "", "");
		PUT->Subscribe(&Null, "Null");
		Null.Subscribe(PUT.get(), "PortUnderTest");

		PUT->Build();
		Null.Build();

		auto ios = odc::asio_service::Get();
		auto work = ios->make_work();
		std::thread t([ios]() {ios->run(); });

		PUT->Enable();
		Null.Enable();
		//auto event = std::make_shared<EventInfo>(EventType::OctetString, 123);
		//event->SetPayload<EventType::OctetString>(OctetStringBuffer(std::string("test event")));
		//PUT->Event(event, "test_harness", std::make_shared<std::function<void(CommandStatus)>>([](CommandStatus) {}));
		//auto event2 = std::make_shared<EventInfo>(EventType::ControlRelayOutputBlock, 666);
		//event2->SetPayload<EventType::ControlRelayOutputBlock>(ControlRelayOutputBlock());
		//PUT->Event(event2, "test_harness", std::make_shared<std::function<void(CommandStatus)>>([](CommandStatus) {}));

		// Synchronous client request examples
		const std::string json_string = "{\"firstName\": \"John\",\"lastName\": \"Smith\",\"age\": 25}";

		HttpsClient client("localhost:443", false);
		try
		{
			if (auto log = odc::spdlog_get("LuaWebPort"))
				log->info("Example GET request to https ://localhost:8080/match/123");
			auto r1 = client.request("GET", "/match/123");
			if (auto log = odc::spdlog_get("LuaWebPort"))
				log->info("Response code {} header {} content: {}", r1->status_code, r1->header.size(), r1->content.string()); // Alternatively, use the convenience function r1->content.string()

			if (auto log = odc::spdlog_get("LuaWebPort"))
				log->info("Example POST request to https://localhost:8080/string");
			auto r2 = client.request("POST", "/string?arg=1&another=2", json_string);
			if (auto log = odc::spdlog_get("LuaWebPort"))
				log->info("Response code {} header {} content: {}", r2->status_code, r2->header.size(), rbuftostr(r2->content.rdbuf()));

			if (auto log = odc::spdlog_get("LuaWebPort"))
				log->info("Example GET request to https ://localhost:8080/fail");
			auto r3 = client.request("GET", "/fail");
			if (auto log = odc::spdlog_get("LuaWebPort"))
				log->info("Response code {} header {} content: {}", r3->status_code, r3->header.size(), r3->content.string()); // Alternatively, use the convenience function r1->content.string()
		}
		catch (const SimpleWeb::system_error& e)
		{
			if (auto log = odc::spdlog_get("LuaWebPort"))
				log->error("Client request error: {}", e.what());
		}


		PUT->Disable();

		work.reset();
		ios->run();
		t.join();
		ios.reset();
	}

	TestTearDown();
	UnLoadModule(portlib);
}



//TEST_CASE(SUITE("ConstructBuildEnableDestroy"))
//{
//	TestSetup();
//	auto portlib = LoadModule(GetLibFileName("LuaWebPort"));
//	REQUIRE(portlib);
//
//	//Test the destruction of an enabled port (happens in case of exception)
//	{
//		newptr newPort = GetPortCreator(portlib, "LuaWeb");
//		REQUIRE(newPort);
//		delptr deletePort = GetPortDestroyer(portlib, "LuaWeb");
//		REQUIRE(deletePort);
//
//		std::shared_ptr<DataPort> PUT(newPort("PortUnderTest", "", GetConfigJSON()), deletePort);
//
//		PUT->Build();
//
//		auto ios = odc::asio_service::Get();
//		auto work = ios->make_work();
//		std::thread t([ios](){ios->run();});
//
//		PUT->Enable();
//		PUT.reset();
//
//		work.reset();
//		ios->run();
//		t.join();
//		ios.reset();
//	}
//
//	TestTearDown();
//	UnLoadModule(portlib);
//}
