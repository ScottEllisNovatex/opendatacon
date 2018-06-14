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
/**
 */
#include <catch.hpp>

#include "DNP3MasterPort.h"
#include "PortLoader.h"

#define SUITE(name) "DNP3MasterPortTestSuite - " name

TEST_CASE(SUITE("ConstructEnableDisableDestroy"))
{
	{
		fptr newMaster = GetPortCreator("DNP3Port", "DNP3Master");
		REQUIRE(newMaster);
		DataPort* MPUT = newMaster("MasterUnderTest", "", "");

		asio::io_service ios;
		MPUT->SetIOS(&ios);
		MPUT->Enable();
		MPUT->Disable();

		delete MPUT;
	}
	/// Test the destruction of an enabled port
	{
		fptr newMaster = GetPortCreator("DNP3Port", "DNP3Master");
		REQUIRE(newMaster);
		DataPort* MPUT = newMaster("MasterUnderTest", "", "");

		asio::io_service ios;
		MPUT->SetIOS(&ios);
		MPUT->Enable();

		delete MPUT;
	}
}
