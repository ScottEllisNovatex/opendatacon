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
* CBOutStationPort.h
*
*  Created on: 12/09/2018
*      Author: Scott Ellis <scott.ellis@novatex.com.au>
*/

#ifndef CBOUTSTATIONPORT_H_
#define CBOUTSTATIONPORT_H_

#include <unordered_map>
#include <vector>
#include <functional>

#include "CB.h"
#include "CBPort.h"
#include "CBUtility.h"
#include "CBConnection.h"
#include "CBPointTableAccess.h"


class OutstationSystemFlags
{
	// CB can support 16 bits of status flags, which are reported by Fn52, system flag scan. The first 8 are reserved for MegaData use, only 3 are documented.
	// The last 8 are contract dependent, we don't know if any are used.
	// A change in any will set the RSF bit in ANY scan/control replies. So we maintain a separate RSF bit in the structure, which will be reset on a flag scan.

public:
	bool GetRemoteStatusChangeFlag() { return RSF; }

	// This is calculated by checking the digital bit changed flag, using a method registered with us
	bool GetDigitalChangedFlag()
	{
		if (DCPCalc != nullptr)
			return DCPCalc();

		LOGERROR("GetDigitalChangedFlag called without a handler being registered");
		return false;
	}

	// This is calculated by checking the timetagged data queues, using a method registered with us
	bool GetTimeTaggedDataAvailableFlag()
	{
		if (HRPCalc != nullptr)
			return HRPCalc();

		LOGERROR("GetTimeTaggedDataAvailableFlag called without a handler being registered");
		return false;
	}

	bool GetSystemPoweredUpFlag() { return SPU; }
	bool GetSystemTimeIncorrectFlag() { return STI; }

	void FlagScanPacketSent() { SPU = false; RSF = false; }
	void TimePacketReceived() { STI = false; }

	void SetDigitalChangedFlagCalculationMethod(std::function<bool(void)> Calc) { DCPCalc = Calc; }
	void SetTimeTaggedDataAvailableFlagCalculationMethod(std::function<bool(void)> Calc) { HRPCalc = Calc; }

private:
	bool RSF = true;                             // All true on start up...
	std::function<bool(void)> HRPCalc = nullptr; // HRER/TimeTagged Events Pending
	std::function<bool(void)> DCPCalc = nullptr; // Digital bit has changed and is waiting to be sent

	bool SPU = true; // Bit 16 of Block data, bit 0 of 16 bit flag data
	bool STI = true; // Bit 17 of Block data, bit 1 of 16 bit flag data
};



class PendingCommandType
{
public:
	enum CommandType { None, Trip, Close, SetA, SetB };

	const CBTime CommandValidTimemsec = 10000; // PendingCommand valid for 10 seconds..
	CommandType Command = None;
	uint16_t Data = 0;
	CBTime ExpiryTime = 0; // If we dont receive the execute before this time, it will not be executed
};

class CBOutstationPort: public CBPort
{
	enum AnalogChangeType { NoChange, DeltaChange, AllChange };
	enum AnalogCounterModuleType { CounterModule, AnalogModule };

public:
	CBOutstationPort(const std::string & aName, const std::string & aConfFilename, const Json::Value & aConfOverrides);
	~CBOutstationPort() override;

	void Enable() override;
	void Disable() override;
	void Build() override;

	void Event(std::shared_ptr<const EventInfo> event, const std::string& SenderName, SharedStatusCallback_t pStatusCallback) override;
	CommandStatus Perform(std::shared_ptr<EventInfo> event, bool waitforresult);

	void SendCBMessage(const CBMessage_t & CompleteCBMessage) override;
	void ProcessCBMessage(CBMessage_t &CompleteCBMessage);

	// Response to PendingCommand Methods
	void ScanRequest(CBBlockData &Header);
	void FuncTripClose(CBBlockData & Header, PendingCommandType::CommandType pCommand);
	void FuncSetAB(CBBlockData & Header, PendingCommandType::CommandType pCommand);
	void ExecuteCommand(CBBlockData & Header);
	void ExecuteBinaryControl(uint8_t group, int Channel, bool point_on);
	void ExecuteAnalogControl(uint8_t group, int Channel, uint16_t data);
	void FuncMasterStationRequest(CBBlockData &Header, CBMessage_t &CompleteCBMessage);

	void ProcessUpdateTimeRequest(CBMessage_t & CompleteCBMessage);
	void EchoReceivedHeaderToMaster(CBBlockData & Header);

	void BuildScanRequestResponseData(uint8_t Group, std::vector<uint16_t>& BlockValues);
	uint16_t GetPayload(uint8_t &Group, PayloadLocationType &payloadlocation);

	void MarkAllBinaryPointsAsChanged();
	uint8_t CountBinaryBlocksWithChanges();

	// Testing use only
	CBPointTableAccess *GetPointTable() { return &(MyPointConf->PointTable); }
	PendingCommandType GetPendingCommand(uint8_t group) { return PendingCommands[group]; } // Return a copy, cannot be changed
private:

	bool DigitalChangedFlagCalculationMethod(void);
	bool TimeTaggedDataAvailableFlagCalculationMethod(void);

	OutstationSystemFlags SystemFlags;

	void SocketStateHandler(bool state);

	uint8_t LastHRERSequenceNumber = 100;      // Used to remember the last HRER scan we sent, starts with an invalid value
	uint8_t LastDigitalScanSequenceNumber = 0; // Used to remember the last digital scan we had
	CBMessage_t LastDigitialScanResponseCBMessage;
	CBMessage_t LastDigitialHRERResponseCBMessage;

	PendingCommandType PendingCommands[15]; // Store a potential pending command for each group.
};

#endif
