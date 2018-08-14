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
#include <opendnp3/app/MeasurementTypes.h>
#include <opendnp3/gen/ControlCode.h>
#include <opendatacon/DataPointConf.h>
#include <opendatacon/ConfigParser.h>

// This one has been given wraps: http://moodycamel.com/blog/2014/a-fast-general-purpose-lock-free-queue-for-c++
// https://github.com/cameron314/concurrentqueue
// And is faster than the lockfree queue from boost.
// Good article http://www.codersblock.org/blog/2016/6/02/ditching-the-mutex

#include "MD3Utility.h"

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

class MD3Point // Abstract Class
{
	//TODO: Can we convert protection to strands??
	//TODO: Integrate ODC quality instead of HasBeenSet flag and Module failed flag.
	/*	NONE              = 0,
	ONLINE            = 1<<0,
	RESTART           = 1<<1,
	COMM_LOST         = 1<<2,
	REMOTE_FORCED     = 1<<3,
	LOCAL_FORCED      = 1<<4,
	OVERRANGE         = 1<<5,
	REFERENCE_ERR     = 1<<6,
	ROLLOVER          = 1<<7,
	DISCONTINUITY     = 1<<8,
	CHATTER_FILTER    = 1<<9*/

public:
	MD3Point() {};

	// We need these as we DO NOT want to copy the mutex!
	MD3Point(const MD3Point &src):
		Index(src.Index),
		ModuleAddress(src.ModuleAddress),
		Channel(src.Channel),
		PollGroup(src.PollGroup),
		ChangedTime(src.ChangedTime),
		ModuleFailed(src.ModuleFailed),
		HasBeenSet(src.HasBeenSet)
	{};
	MD3Point& MD3Point::operator=(const MD3Point& src)
	{
		Index = src.Index;
		ModuleAddress = src.ModuleAddress;
		Channel = src.Channel;
		PollGroup = src.PollGroup;
		ChangedTime = src.ChangedTime;
		ModuleFailed = src.ModuleFailed;
		HasBeenSet = src.HasBeenSet;
		return *this;
	}

	MD3Point(uint32_t index, uint8_t moduleaddress, uint8_t channel, MD3Time changedtime, uint8_t pollgroup ):
		Index(index),
		ModuleAddress(moduleaddress),
		Channel(channel),
		PollGroup(pollgroup),
		ChangedTime(changedtime)
	{};

	// These first 4 never change, so no protection
	virtual uint32_t GetIndex() = 0; // Make the base class pure virtual

	uint8_t GetModuleAddress() { return ModuleAddress; };
	uint8_t GetChannel() { return Channel; };
	uint8_t GetPollGroup() { return PollGroup; };

	//TODO: Do we need to get them all at the same time - so one does not get changed in the middle??
	bool GetModuleFailed() { std::unique_lock<std::mutex> lck(PointMutex); return ModuleFailed; };
	MD3Time GetChangedTime() { std::unique_lock<std::mutex> lck(PointMutex); return ChangedTime;    };
	bool GetHasBeenSet() { std::unique_lock<std::mutex> lck(PointMutex); return HasBeenSet; };
//	void SetChangedTime(const MD3Time &ct) { std::unique_lock<std::mutex> lck(PointMutex); ChangedTime = ct; }; // Not needed...

protected:
	std::mutex PointMutex;
	uint32_t Index = 0;
	uint8_t ModuleAddress = 0;
	bool ModuleFailed = false; // Will be set to true if the connection to a master through ODC signals the master is not talking to its slave. For digitals we send a different response
	uint8_t Channel = 0;
	uint8_t PollGroup = 0;
	MD3Time ChangedTime = (MD3Time)0; // msec since epoch. 1970,1,1 Only used for Fn9 and 11 queued data. TimeStamp is Uint48_t, MD3 is uint64_t but does not overflow.
	bool HasBeenSet = false;          // To determine if we have been set since startup
};

enum BinaryPointType {BASICINPUT, TIMETAGGEDINPUT, DOMOUTPUT, POMOUTPUT};

class MD3BinaryPoint: public MD3Point
{
public:
	MD3BinaryPoint() {};
	MD3BinaryPoint(const MD3BinaryPoint &src): MD3Point(src),
		Binary(src.Binary),
		ModuleBinarySnapShot(src.ModuleBinarySnapShot),
		Changed(src.Changed),
		PointType(src.PointType)
	{};
	// Needed for the templated Queue
	MD3BinaryPoint& MD3BinaryPoint::operator=(const MD3BinaryPoint& src)
	{
		Index = src.Index;
		ModuleAddress = src.ModuleAddress;
		Channel = src.Channel;
		PollGroup = src.PollGroup;
		ChangedTime = src.ChangedTime;
		ModuleFailed = src.ModuleFailed;
		HasBeenSet = src.HasBeenSet;
		Binary = src.Binary;
		ModuleBinarySnapShot = src.ModuleBinarySnapShot;
		Changed = src.Changed;
		PointType = src.PointType;
		return *this;
	}
	MD3BinaryPoint(uint32_t index, uint8_t moduleaddress, uint8_t channel, uint8_t pollgroup, BinaryPointType pointtype): MD3Point(index, moduleaddress, channel, (MD3Time)0, pollgroup),
		PointType(pointtype)
	{};

	MD3BinaryPoint(uint32_t index, uint8_t moduleaddress, uint8_t channel, uint8_t pollgroup, BinaryPointType pointtype, uint8_t binval, bool changed, MD3Time changedtime):
		MD3Point(index, moduleaddress,  channel, changedtime, pollgroup),
		PointType(pointtype),
		Binary(binval),
		Changed(changed)
	{};

	const BinaryPointType GetPointType() { return PointType; };
	uint32_t GetIndex() { return Index; };

	uint8_t GetBinary() { std::unique_lock<std::mutex> lck(PointMutex); return Binary; };
	uint16_t GetModuleBinarySnapShot() { std::unique_lock<std::mutex> lck(PointMutex); return ModuleBinarySnapShot; };

	bool GetChangedFlag() { std::unique_lock<std::mutex> lck(PointMutex); return Changed; };
	void SetChangedFlag() { std::unique_lock<std::mutex> lck(PointMutex); Changed = true; };
	bool GetAndResetChangedFlag() { std::unique_lock<std::mutex> lck(PointMutex); bool res = Changed; Changed = false; return res; };

	void SetBinary(const uint8_t &b, const MD3Time &ctime) { std::unique_lock<std::mutex> lck(PointMutex); HasBeenSet = true; ChangedTime = ctime; Changed = (Binary != b); Binary = b; };
	void SetChanged(const bool &c) { std::unique_lock<std::mutex> lck(PointMutex); Changed = c; };
	void SetModuleBinarySnapShot(const uint16_t &bm) { std::unique_lock<std::mutex> lck(PointMutex); ModuleBinarySnapShot = bm; };

protected:
	// Only the values below will be changed in two places
	uint8_t Binary = 0x01;
	uint16_t ModuleBinarySnapShot = 0; // Used for the queue necessary to handle Fn11 time tagged events. Have to remember all 16 bits when the event happened
	bool Changed = true;
	BinaryPointType PointType = BASICINPUT;
};

class MD3AnalogCounterPoint: public MD3Point
{

public:
	MD3AnalogCounterPoint() {};

	MD3AnalogCounterPoint(uint32_t index, uint8_t moduleaddress, uint8_t channel, uint8_t pollgroup): MD3Point(index, moduleaddress, channel, (MD3Time)0, pollgroup)
	{};

	uint32_t GetIndex() { return Index; };
	uint16_t GetAnalog() { std::unique_lock<std::mutex> lck(PointMutex); return Analog; };
	uint16_t GetAnalogAndDelta(int &delta) { std::unique_lock<std::mutex> lck(PointMutex); delta = (int)Analog - (int)LastReadAnalog; LastReadAnalog = Analog; return Analog; };

	void SetAnalog(const uint16_t & a, const MD3Time &ctime) { std::unique_lock<std::mutex> lck(PointMutex); HasBeenSet = true; Analog = a; ChangedTime = ctime; };
	void SetLastReadAnalog(const uint16_t & a) { std::unique_lock<std::mutex> lck(PointMutex); LastReadAnalog = a; };

protected:
	uint16_t Analog = 0x8000;
	uint16_t LastReadAnalog = 0x8000;
};

typedef std::map<uint32_t, std::shared_ptr<MD3BinaryPoint>>::iterator ODCBinaryPointMapIterType;
typedef std::map<uint16_t, std::shared_ptr<MD3BinaryPoint>>::iterator MD3BinaryPointMapIterType;
typedef std::map<uint32_t, std::shared_ptr<MD3AnalogCounterPoint>>::iterator ODCAnalogCounterPointMapIterType;
typedef std::map<uint16_t, std::shared_ptr<MD3AnalogCounterPoint>>::iterator MD3AnalogCounterPointMapIterType;
typedef std::map<uint8_t, uint16_t> ModuleMapType;

enum PollGroupType { BinaryPoints, AnalogPoints, TimeSetCommand, NewTimeSetCommand, SystemFlagScan };

class MD3PollGroup
{
public:
	MD3PollGroup():
		ID(0),
		pollrate(0),
		polltype(BinaryPoints),
		ForceUnconditional(false)
	{ }

	MD3PollGroup(uint32_t ID_, uint32_t pollrate_, PollGroupType polltype_,bool forceunconditional, bool timetaggeddigital):
		ID(ID_),
		pollrate(pollrate_),
		polltype(polltype_),
		ForceUnconditional(forceunconditional),
		TimeTaggedDigital(timetaggeddigital)
	{ }

	uint32_t ID;
	uint32_t pollrate;
	PollGroupType polltype;
	bool ForceUnconditional;       // Used to prevent use of delta or COS commands.
	bool TimeTaggedDigital;        // Set to true if this poll group contains time tagged digital points.
	ModuleMapType ModuleAddresses; // The second value is for channel count
	// As we load points we will build this list
};

class MD3PointConf: public ConfigParser
{
private:


public:
	MD3PointConf(const std::string& FileName, const Json::Value& ConfOverrides);

	// JSON File section processing commands
	void ProcessElements(const Json::Value& JSONRoot) override;
	void ProcessPollGroups(const Json::Value & JSONRoot);
	void ProcessBinaryPoints(const std::string & BinaryName, const Json::Value & JSONNode, std::map<uint16_t, std::shared_ptr<MD3BinaryPoint>>& MD3PointMap, std::map<uint32_t, std::shared_ptr<MD3BinaryPoint>>& ODCPointMap);
	void ProcessAnalogCounterPoints(const Json::Value & JSONNode, std::map<uint16_t, std::shared_ptr<MD3AnalogCounterPoint>>& MD3PointMap, std::map<uint32_t, std::shared_ptr<MD3AnalogCounterPoint>>& ODCPointMap);


	// We access the map using a Module:Channel combination, so that they will always be in order. Makes searching the next item easier.
	std::map<uint16_t , std::shared_ptr<MD3BinaryPoint>> BinaryMD3PointMap; // ModuleAndChannel, MD3Point
	std::map<uint32_t , std::shared_ptr<MD3BinaryPoint>> BinaryODCPointMap; // Index OpenDataCon, MD3Point

	std::map<uint16_t, std::shared_ptr<MD3AnalogCounterPoint>> AnalogMD3PointMap; // ModuleAndChannel, MD3Point
	std::map<uint32_t, std::shared_ptr<MD3AnalogCounterPoint>> AnalogODCPointMap; // Index OpenDataCon, MD3Point

	std::map<uint16_t, std::shared_ptr<MD3AnalogCounterPoint>> CounterMD3PointMap; // ModuleAndChannel, MD3Point
	std::map<uint32_t, std::shared_ptr<MD3AnalogCounterPoint>> CounterODCPointMap; // Index OpenDataCon, MD3Point

	// Binary Control Points are not readable
	std::map<uint16_t, std::shared_ptr<MD3BinaryPoint>> BinaryControlMD3PointMap; // ModuleAndChannel, MD3Point
	std::map<uint32_t, std::shared_ptr<MD3BinaryPoint>> BinaryControlODCPointMap; // Index OpenDataCon, MD3Point

	// Analog Control Points are not readable
	std::map<uint16_t, std::shared_ptr<MD3AnalogCounterPoint>> AnalogControlMD3PointMap; // ModuleAndChannel, MD3Point
	std::map<uint32_t, std::shared_ptr<MD3AnalogCounterPoint>> AnalogControlODCPointMap; // Index OpenDataCon, MD3Point


	std::map<uint32_t, MD3PollGroup> PollGroups;

	// Time Set Point Configuration - this is a "special" point that is used to pass the time set command through ODC.
	//TODO: double does not pass uint64_t through for time pass through commands - find another way...
	std::pair<double, uint32_t> TimeSetPoint = std::make_pair(double(0), (uint32_t)0);
	// Same as above but for Fn44 - don't pass UTC offset through. Get that from the machine running ODC
	std::pair<double, uint32_t> TimeSetPointNew = std::make_pair(double(0), (uint32_t)0);

	// System Sign On Configuration - this is a "special" point that is used to pass the systemsignon command through ODC.
	std::pair<int32_t, uint32_t> SystemSignOnPoint = std::make_pair(int32_t(0),(uint32_t)0);
	std::pair<int32_t, uint32_t> FreezeResetCountersPoint = std::make_pair(int32_t(0), (uint32_t)0);
	std::pair<int32_t, uint32_t> POMControlPoint = std::make_pair(int32_t(0), (uint32_t)0);
	std::pair<int32_t, uint32_t> DOMControlPoint = std::make_pair(int32_t(0), (uint32_t)0);

	// Use the OLD Digital Commands 7/8 or the NEW ones 9/10/11/12
	bool NewDigitalCommands = true;

	// Checks if the point time is within 30 minutes of current time from a Binary event. If it is outside this window, sets the event time to the current time.
	bool OverrideOldTimeStamps = false;

	// If true, the outstation will send responses on the TCP connection without waiting for ODC responses.
	bool StandAloneOutstation = false;

	// Time to wait for ODC command to return a result before returning with an error of TIMEOUT. Remember there can be multiple responders!
	uint32_t MD3CommandTimeoutmsec = 5000;
	// How many times do we retry a command, before we give up and move onto the next one?
	uint32_t MD3CommandRetries = 3;
};
#endif /* MD3POINTCONF_H_ */