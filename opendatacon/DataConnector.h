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
 * DataConnector.h
 *
 *  Created on: 15/07/2014
 *      Author: Neil Stephens <dearknarl@gmail.com>
 */

#ifndef DATACONNECTOR_H_
#define DATACONNECTOR_H_

#include <opendatacon/IOHandler.h>
#include <opendatacon/ConfigParser.h>
#include <opendatacon/Transform.h>
#include <opendatacon/Logger.h>

namespace ODC
{
	class DataConnector : public Logger, public IOHandler, public ConfigParser
	{
	public:
		DataConnector(std::string aName, std::string aConfFilename, const Json::Value aConfOverrides);
		~DataConnector(){};
        
		std::future<CommandStatus> Event(const Binary& meas, uint16_t index, const std::string& SenderName);
		std::future<CommandStatus> Event(const DoubleBitBinary& meas, uint16_t index, const std::string& SenderName);
		std::future<CommandStatus> Event(const Analog& meas, uint16_t index, const std::string& SenderName);
		std::future<CommandStatus> Event(const Counter& meas, uint16_t index, const std::string& SenderName);
		std::future<CommandStatus> Event(const FrozenCounter& meas, uint16_t index, const std::string& SenderName);
		std::future<CommandStatus> Event(const BinaryOutputStatus& meas, uint16_t index, const std::string& SenderName);
		std::future<CommandStatus> Event(const AnalogOutputStatus& meas, uint16_t index, const std::string& SenderName);

		std::future<CommandStatus> Event(const BinaryQuality qual, uint16_t index, const std::string& SenderName);
		std::future<CommandStatus> Event(const DoubleBitBinaryQuality qual, uint16_t index, const std::string& SenderName);
		std::future<CommandStatus> Event(const AnalogQuality qual, uint16_t index, const std::string& SenderName);
		std::future<CommandStatus> Event(const CounterQuality qual, uint16_t index, const std::string& SenderName);
		std::future<CommandStatus> Event(const FrozenCounterQuality qual, uint16_t index, const std::string& SenderName);
		std::future<CommandStatus> Event(const BinaryOutputStatusQuality qual, uint16_t index, const std::string& SenderName);
		std::future<CommandStatus> Event(const AnalogOutputStatusQuality qual, uint16_t index, const std::string& SenderName);

		std::future<CommandStatus> Event(const ControlRelayOutputBlock& arCommand, uint16_t index, const std::string& SenderName);
		std::future<CommandStatus> Event(const AnalogOutputInt16& arCommand, uint16_t index, const std::string& SenderName);
		std::future<CommandStatus> Event(const AnalogOutputInt32& arCommand, uint16_t index, const std::string& SenderName);
		std::future<CommandStatus> Event(const AnalogOutputFloat32& arCommand, uint16_t index, const std::string& SenderName);
		std::future<CommandStatus> Event(const AnalogOutputDouble64& arCommand, uint16_t index, const std::string& SenderName);

		std::future<CommandStatus> Event(ConnectState state, uint16_t index, const std::string& SenderName);

		virtual const Json::Value GetStatistics() const
		{
			return Json::Value();
		};

		virtual const Json::Value GetCurrentState() const
		{
			return Json::Value();
		};

		void Enable();
		void Disable();
		void BuildOrRebuild();

	protected:
		void ProcessElements(const Json::Value& JSONRoot);
		template<typename T> std::future<CommandStatus> EventT(const T& meas, uint16_t index, const std::string& SenderName);

		std::unordered_map<std::string, std::pair<IOHandler*, IOHandler*> > Connections;
		std::multimap<std::string, std::string> SenderConnectionsLookup;
		std::unordered_map<std::string, std::vector<std::unique_ptr<Transform> > > ConnectionTransforms;
	};

}

#endif /* DATACONNECTOR_H_ */
