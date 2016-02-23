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
 * JSONDataPort.h
 *
 *  Created on: 22/07/2014
 *      Author: Neil Stephens <dearknarl@gmail.com>
 */

#ifndef JSONDATAPORT_H_
#define JSONDATAPORT_H_

#include <unordered_map>
#include <opendatacon/DataPort.h>
#include "JSONPortConf.h"

using namespace ODC;

class JSONPort: public DataPort
{
public:
	JSONPort(const std::string& aName, ODC::Context& aParent, const std::string& aConfFilename, const Json::Value& aConfOverrides):
		DataPort(aName, aParent, aConfFilename, aConfOverrides)
	{
		//the creation of a new PortConf will get the point details
		pConf.reset(new JSONPortConf(aConfFilename, *this));

		//We still may need to process the file (or overrides) to get Addr details:
		ProcessFile();
	}
	using DataPort::IOHandler::Event;

	void ProcessElements(const Json::Value& JSONRoot)
	{
		if(!JSONRoot["IP"].isNull())
			static_cast<JSONPortConf*>(pConf.get())->mAddrConf.IP = JSONRoot["IP"].asString();

		if(!JSONRoot["Port"].isNull())
			static_cast<JSONPortConf*>(pConf.get())->mAddrConf.Port = JSONRoot["Port"].asUInt();

		//TODO: document this
		if(!JSONRoot["RetryTimems"].isNull())
			static_cast<JSONPortConf*>(pConf.get())->retry_time_ms = JSONRoot["RetryTimems"].asUInt();
	}

	virtual void Enable()=0;
	virtual void Disable()=0;
	virtual void BuildOrRebuild()=0;

	virtual std::future<CommandStatus> Event(const Binary& meas, uint16_t index, const std::string& SenderName) { return IOHandler::CommandFutureNotSupported(); }
	virtual std::future<CommandStatus> Event(const DoubleBitBinary& meas, uint16_t index, const std::string& SenderName) { return IOHandler::CommandFutureNotSupported(); }
	virtual std::future<CommandStatus> Event(const Analog& meas, uint16_t index, const std::string& SenderName) { return IOHandler::CommandFutureNotSupported(); }
	virtual std::future<CommandStatus> Event(const Counter& meas, uint16_t index, const std::string& SenderName) { return IOHandler::CommandFutureNotSupported(); }
	virtual std::future<CommandStatus> Event(const FrozenCounter& meas, uint16_t index, const std::string& SenderName) { return IOHandler::CommandFutureNotSupported(); }
	virtual std::future<CommandStatus> Event(const BinaryOutputStatus& meas, uint16_t index, const std::string& SenderName) { return IOHandler::CommandFutureNotSupported(); }
	virtual std::future<CommandStatus> Event(const AnalogOutputStatus& meas, uint16_t index, const std::string& SenderName) { return IOHandler::CommandFutureNotSupported(); }

	virtual std::future<CommandStatus> Event(const ControlRelayOutputBlock& arCommand, uint16_t index, const std::string& SenderName) { return IOHandler::CommandFutureNotSupported(); }
	virtual std::future<CommandStatus> Event(const AnalogOutputInt16& arCommand, uint16_t index, const std::string& SenderName) { return IOHandler::CommandFutureNotSupported(); }
	virtual std::future<CommandStatus> Event(const AnalogOutputInt32& arCommand, uint16_t index, const std::string& SenderName) { return IOHandler::CommandFutureNotSupported(); }
	virtual std::future<CommandStatus> Event(const AnalogOutputFloat32& arCommand, uint16_t index, const std::string& SenderName) { return IOHandler::CommandFutureNotSupported(); }
	virtual std::future<CommandStatus> Event(const AnalogOutputDouble64& arCommand, uint16_t index, const std::string& SenderName) { return IOHandler::CommandFutureNotSupported(); }
	virtual std::future<CommandStatus> ConnectionEvent(ConnectState state, const std::string& SenderName) { return IOHandler::CommandFutureNotSupported(); }

	virtual std::future<CommandStatus> Event(const BinaryQuality qual, uint16_t index, const std::string& SenderName) { return IOHandler::CommandFutureNotSupported(); }
	virtual std::future<CommandStatus> Event(const DoubleBitBinaryQuality qual, uint16_t index, const std::string& SenderName) { return IOHandler::CommandFutureNotSupported(); }
	virtual std::future<CommandStatus> Event(const AnalogQuality qual, uint16_t index, const std::string& SenderName) { return IOHandler::CommandFutureNotSupported(); }
	virtual std::future<CommandStatus> Event(const CounterQuality qual, uint16_t index, const std::string& SenderName) { return IOHandler::CommandFutureNotSupported(); }
	virtual std::future<CommandStatus> Event(const FrozenCounterQuality qual, uint16_t index, const std::string& SenderName) { return IOHandler::CommandFutureNotSupported(); }
	virtual std::future<CommandStatus> Event(const BinaryOutputStatusQuality qual, uint16_t index, const std::string& SenderName) { return IOHandler::CommandFutureNotSupported(); }
	virtual std::future<CommandStatus> Event(const AnalogOutputStatusQuality qual, uint16_t index, const std::string& SenderName) { return IOHandler::CommandFutureNotSupported(); }

	std::unique_ptr<asio::basic_stream_socket<asio::ip::tcp> > pSock;

};

#endif /* JSONDATAPORT_H_ */
