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
 * MD3PointConf.h
 *
 *  Created on: 10/03/2018
 *      Author: Scott Ellis <scott.ellis@novatex.com.au> - Derived from  DNP3PointConf.h
 */

#ifndef MD3POINTCONF_H_
#define MD3POINTCONF_H_

#include <vector>
#include <map>
#include <unordered_map>
#include <bitset>
#include <chrono>
#include <opendatacon/IOTypes.h>
#include <opendatacon/DataPointConf.h>
#include <opendatacon/ConfigParser.h>

// This one has been given wraps: http://moodycamel.com/blog/2014/a-fast-general-purpose-lock-free-queue-for-c++
// https://github.com/cameron314/concurrentqueue
// And is faster than the lockfree queue from boost.
// Good article http://www.codersblock.org/blog/2016/6/02/ditching-the-mutex

#include "MD3.h"
#include "MD3Utility.h"
#include "MD3PointTableAccess.h"

// Also I have concerns about blocking checks on futures, which would block one of the asio threads - we may only have 4!
// This guys has some answers:
// http://code-slim-jim.blogspot.com/2014/07/async-usage-of-futures-and-promises-in.html
// So here is how you use std::promise and std::future in lambdas, binds, asio and anything else u cant move into.
// I also tossed in an example of how to use non-blocking checks on the futures which is another key tool for using futures in asio code.
// Apparently you can just call IOsvc.run() to yield the thread. It will come back when it has nothing to do.

using namespace odc;

// So the MD3 Addressing structure requires us to support a Module addressing scheme 1 to 256 for digital IO with 16 bits at each location
// The Analogs are effectively the same, with 16 channels at each Module address.

// OpenDataCon uses an Index to refer to Analogs and Binaries. MD3 Uses a Module/Channel addressing mode. So our config file needs to contain information about how
// we map the Index exposed to OpenDataCon to the Module/Channel required by MD3. The Channel we assume is zero based. (ie 0 to 15 valid values)
// So for a given Index (or Range) we define a Module and an Offset.
// For example a module with address 20, Channel 5 would look like "Binaries" : [{"Index": 0, "Module" : 20, "Offset" : 5}]
//
// Or for a range which we require to be contiguous. If more than 16 channels are in a range, then the Module Address will be incremented for the next 16 and so on.
//
// "Analogs" : [{"Range" : {"Start" : 0, "Stop" : 15}, "Module" : 32, "Offset" : 0}],
//


class MD3PointConf: public ConfigParser
{
public:
	MD3PointConf(const std::string& FileName, const Json::Value& ConfOverrides);

	// JSON File section processing commands
	void ProcessElements(const Json::Value& JSONRoot) override;
	void ProcessPollGroups(const Json::Value & JSONRoot);
	void ProcessBinaryPoints(PointType ptype, const Json::Value & JSONNode);
	void ProcessAnalogCounterPoints(PointType ptype, const Json::Value & JSONNode);

	MD3PointTableAccess PointTable; // All access to point table through this.

	std::map<uint32_t, MD3PollGroup> PollGroups;

	// Use the OLD Digital Commands 7/8 or the NEW ones 9/10/11/12
	bool NewDigitalCommands = true;

	// Checks if the point time is within 30 minutes of current time from a Binary event. If it is outside this window, sets the event time to the current time.
	bool OverrideOldTimeStamps = false;
	bool UpdateAnalogCounterTimeStamps = false; // Every time we get a no change message back, update the current timestamp on the point table - so the timestamp becomes last valid time.

	// If true, the outstation will send responses on the TCP connection without waiting for ODC responses.
	bool StandAloneOutstation = false;

	// Time to wait for ODC command to return a result before returning with an error of TIMEOUT. Remember there can be multiple responders!
	uint32_t MD3CommandTimeoutmsec = 5000;
	// How many times do we retry a command, before we give up and move onto the next one?
	uint32_t MD3CommandRetries = 3;
	std::string FileName;
};
#endif
