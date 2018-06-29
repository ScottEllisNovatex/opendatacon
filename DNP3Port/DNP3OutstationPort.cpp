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
 * DNP3ServerPort.cpp
 *
 *  Created on: 15/07/2014
 *      Author: Neil Stephens <dearknarl@gmail.com>
 */

#include <iostream>
#include <regex>
#include <chrono>
#include <asiopal/UTCTimeSource.h>
#include <asiodnp3/MeasUpdate.h>
#include <opendnp3/outstation/IOutstationApplication.h>
#include <openpal/logging/LogLevels.h>
#include "DNP3OutstationPort.h"
#include "DNP3PortConf.h"
#include "ChannelStateSubscriber.h"

#include "OpenDNP3Helpers.h"
#include "TypeConversion.h"

DNP3OutstationPort::DNP3OutstationPort(const std::string& aName, const std::string& aConfFilename, const Json::Value& aConfOverrides):
	DNP3Port(aName, aConfFilename, aConfOverrides),
	pOutstation(nullptr)
{}

DNP3OutstationPort::~DNP3OutstationPort()
{
	//pOutstation->Shutdown();
	ChannelStateSubscriber::Unsubscribe(this);
}

void DNP3OutstationPort::Enable()
{
	if(enabled)
		return;
	if(nullptr == pOutstation)
	{
		if(auto log = spdlog::get("DNP3Port"))
			log->error("{}: DNP3 stack not configured.", Name);

		return;
	}
	pOutstation->Enable();
	enabled = true;

	PublishEvent(ConnectState::PORT_UP);
}
void DNP3OutstationPort::Disable()
{
	if(!enabled)
		return;
	enabled = false;

	pOutstation->Disable();
}

// Called by OpenDNP3 Thread Pool
// Called when a the reset/unreset status of the link layer changes (and on link up / channel down)
void DNP3OutstationPort::OnStateChange(opendnp3::LinkStatus status)
{
	this->status = status;
	if(link_dead && !channel_dead) //must be on link up
	{
		link_dead = false;
		PublishEvent(ConnectState::CONNECTED);
	}
	//TODO: track a new statistic - reset count
}
// Called by OpenDNP3 Thread Pool
// Called when a keep alive message (request link status) receives no response
void DNP3OutstationPort::OnKeepAliveFailure()
{
	this->OnLinkDown();
}
void DNP3OutstationPort::OnLinkDown()
{
	if(!link_dead)
	{
		link_dead = true;
		PublishEvent(ConnectState::DISCONNECTED);
	}
}
// Called by OpenDNP3 Thread Pool
// Called when a keep alive message receives a valid response
void DNP3OutstationPort::OnKeepAliveSuccess()
{
	if(link_dead)
	{
		link_dead = false;
		PublishEvent(ConnectState::CONNECTED);
	}
}

TCPClientServer DNP3OutstationPort::ClientOrServer()
{
	DNP3PortConf* pConf = static_cast<DNP3PortConf*>(this->pConf.get());
	if(pConf->mAddrConf.ClientServer == TCPClientServer::DEFAULT)
		return TCPClientServer::SERVER;
	return pConf->mAddrConf.ClientServer;
}

void DNP3OutstationPort::BuildOrRebuild()
{
	DNP3PortConf* pConf = static_cast<DNP3PortConf*>(this->pConf.get());

	pChannel = GetChannel();

	if (pChannel == nullptr)
	{
		if(auto log = spdlog::get("DNP3Port"))
			log->error("{}: Channel not found for outstation.", Name);
		return;
	}

	opendnp3::OutstationStackConfig StackConfig;

	// Link layer configuration
	StackConfig.link.LocalAddr = pConf->mAddrConf.OutstationAddr;
	StackConfig.link.RemoteAddr = pConf->mAddrConf.MasterAddr;
	StackConfig.link.NumRetry = pConf->pPointConf->LinkNumRetry;
	StackConfig.link.Timeout = openpal::TimeDuration::Milliseconds(pConf->pPointConf->LinkTimeoutms);
	if(pConf->pPointConf->LinkKeepAlivems == 0)
		StackConfig.link.KeepAliveTimeout = openpal::TimeDuration::Max();
	else
		StackConfig.link.KeepAliveTimeout = openpal::TimeDuration::Milliseconds(pConf->pPointConf->LinkKeepAlivems);
	StackConfig.link.UseConfirms = pConf->pPointConf->LinkUseConfirms;

	// Outstation parameters
	StackConfig.outstation.params.indexMode = opendnp3::IndexMode::Discontiguous;
	StackConfig.outstation.params.allowUnsolicited = pConf->pPointConf->EnableUnsol;
	StackConfig.outstation.params.unsolClassMask = pConf->pPointConf->GetUnsolClassMask();
	StackConfig.outstation.params.typesAllowedInClass0 = opendnp3::StaticTypeBitField::AllTypes();                                     /// TODO: Create parameter
	StackConfig.outstation.params.maxControlsPerRequest = pConf->pPointConf->MaxControlsPerRequest;                                    /// The maximum number of controls the outstation will attempt to process from a single APDU
	StackConfig.outstation.params.maxTxFragSize = pConf->pPointConf->MaxTxFragSize;                                                    /// The maximum fragment size the outstation will use for fragments it sends
	StackConfig.outstation.params.maxRxFragSize = pConf->pPointConf->MaxTxFragSize;                                                    /// The maximum fragment size the outstation will use for fragments it sends
	StackConfig.outstation.params.selectTimeout = openpal::TimeDuration::Milliseconds(pConf->pPointConf->SelectTimeoutms);             /// How long the outstation will allow an operate to proceed after a prior select
	StackConfig.outstation.params.solConfirmTimeout = openpal::TimeDuration::Milliseconds(pConf->pPointConf->SolConfirmTimeoutms);     /// Timeout for solicited confirms
	StackConfig.outstation.params.unsolConfirmTimeout = openpal::TimeDuration::Milliseconds(pConf->pPointConf->UnsolConfirmTimeoutms); /// Timeout for unsolicited confirms
	StackConfig.outstation.params.unsolRetryTimeout = openpal::TimeDuration::Milliseconds(pConf->pPointConf->UnsolConfirmTimeoutms);   /// Timeout for unsolicited retried

	// TODO: Expose event limits for any new event types to be supported by opendatacon
	StackConfig.outstation.eventBufferConfig.maxBinaryEvents = pConf->pPointConf->MaxBinaryEvents;   /// The number of binary events the outstation will buffer before overflowing
	StackConfig.outstation.eventBufferConfig.maxAnalogEvents = pConf->pPointConf->MaxAnalogEvents;   /// The number of analog events the outstation will buffer before overflowing
	StackConfig.outstation.eventBufferConfig.maxCounterEvents = pConf->pPointConf->MaxCounterEvents; /// The number of counter events the outstation will buffer before overflowing

	StackConfig.dbTemplate = opendnp3::DatabaseTemplate(pConf->pPointConf->BinaryIndicies.size(), 0, pConf->pPointConf->AnalogIndicies.size());

	pOutstation = pChannel->AddOutstation(Name.c_str(), *this, *this, StackConfig);
	ChannelStateSubscriber::Subscribe(this,pChannel);

	if (pOutstation == nullptr)
	{
		if(auto log = spdlog::get("DNP3Port"))
			log->error("{}: Error creating outstation.", Name);
		return;
	}

	auto configView = pOutstation->GetConfigView();

	{
		uint16_t rawIndex = 0;
		for (auto index : pConf->pPointConf->AnalogIndicies)
		{
			configView.analogs[rawIndex].vIndex = index;
			configView.analogs[rawIndex].variation = pConf->pPointConf->StaticAnalogResponses[index];
			configView.analogs[rawIndex].metadata.clazz = pConf->pPointConf->AnalogClasses[index];
			configView.analogs[rawIndex].metadata.variation = pConf->pPointConf->EventAnalogResponses[index];
			configView.analogs[rawIndex].metadata.deadband = pConf->pPointConf->AnalogDeadbands[index];
			++rawIndex;
		}
	}
	{
		uint16_t rawIndex = 0;
		for (auto index : pConf->pPointConf->BinaryIndicies)
		{
			configView.binaries[rawIndex].vIndex = index;
			configView.binaries[rawIndex].variation = pConf->pPointConf->StaticBinaryResponses[index];
			configView.binaries[rawIndex].metadata.clazz = pConf->pPointConf->BinaryClasses[index];
			configView.binaries[rawIndex].metadata.variation = pConf->pPointConf->EventBinaryResponses[index];
			++rawIndex;
		}
	}
}

//DataPort function for UI
const Json::Value DNP3OutstationPort::GetCurrentState() const
{
	Json::Value event;
	Json::Value analogValues;
	Json::Value binaryValues;
	if (pOutstation == nullptr)
		return IUIResponder::GenerateResult("Bad port");

	auto configView = pOutstation->GetConfigView();

	for (auto point : configView.analogs)
	{
		analogValues[std::to_string(point.vIndex)] = point.value.value;
	}
	for (auto point : configView.binaries)
	{
		binaryValues[std::to_string(point.vIndex)] = point.value.value;
	}

	event["AnalogCurrent"] = analogValues;
	event["BinaryCurrent"] = binaryValues;

	return event;
}

//DataPort function for UI
const Json::Value DNP3OutstationPort::GetStatistics() const
{
	Json::Value event;
	if (pChannel != nullptr)
	{
		auto ChanStats = this->pChannel->GetChannelStatistics();
		event["numCrcError"] = ChanStats.numCrcError;             /// Number of frames discared due to CRC errors
		event["numLinkFrameTx"] = ChanStats.numLinkFrameTx;       /// Number of frames transmitted
		event["numLinkFrameRx"] = ChanStats.numLinkFrameRx;       /// Number of frames received
		event["numBadLinkFrameRx"] = ChanStats.numBadLinkFrameRx; /// Number of frames detected with bad / malformed contents
		event["numBytesRx"] = ChanStats.numBytesRx;
		event["numBytesTx"] = ChanStats.numBytesTx;
		event["numClose"] = ChanStats.numClose;
		event["numOpen"] = ChanStats.numOpen;
		event["numOpenFail"] = ChanStats.numOpenFail;
	}
	if (pOutstation != nullptr)
	{
		auto StackStats = this->pOutstation->GetStackStatistics();
		event["numTransportErrorRx"] = StackStats.numTransportErrorRx;
		event["numTransportRx"] = StackStats.numTransportRx;
		event["numTransportTx"] = StackStats.numTransportTx;
	}

	return event;
}

template<typename T>
inline opendnp3::CommandStatus DNP3OutstationPort::SupportsT(T& arCommand, uint16_t aIndex)
{
	if(!enabled)
		return opendnp3::CommandStatus::UNDEFINED;

	auto pConf = static_cast<DNP3PortConf*>(this->pConf.get());
	if(std::is_same<T,opendnp3::ControlRelayOutputBlock>::value) //TODO: add support for other types of controls (probably un-templatise when we support more)
	{
		for(auto index : pConf->pPointConf->ControlIndicies)
			if(index == aIndex)
				return opendnp3::CommandStatus::SUCCESS;
	}
	return opendnp3::CommandStatus::NOT_SUPPORTED;
}

// Called by OpenDNP3 Thread Pool
template<typename T>
inline opendnp3::CommandStatus DNP3OutstationPort::PerformT(T& arCommand, uint16_t aIndex)
{
	if(!enabled)
		return opendnp3::CommandStatus::UNDEFINED;

	auto event = ToODC(arCommand, aIndex, Name);

	auto pConf = static_cast<DNP3PortConf*>(this->pConf.get());
	if (!pConf->pPointConf->WaitForCommandResponses)
	{
		PublishEvent(event);
		return opendnp3::CommandStatus::SUCCESS;
	}

	//TODO: enquire about the possibility of the opendnp3 API having a callback for the result
	// Or if the outstation supported Group13Var1/2 (BinaryCommandEvent), we could use that (maybe the latest opendnp3 already does...?)
	// either one would avoid the below polling loop
	std::atomic_bool cb_executed;
	CommandStatus cb_status;
	auto StatusCallback = std::make_shared<std::function<void (CommandStatus status)>>([&](CommandStatus status)
		{
			cb_status = status;
			cb_executed = true;
		});
	PublishEvent(event, StatusCallback);
	while(!cb_executed)
	{
		//This loop pegs a core and blocks the outstation strand,
		//	but there's no other way to wait for the result.
		//	We can maybe do some work while we wait.
		pIOS->poll_one();
	}
	return FromODC(cb_status);
}

void DNP3OutstationPort::Event(std::shared_ptr<const EventInfo> event, const std::string& SenderName, SharedStatusCallback_t pStatusCallback)
{
	if (!enabled)
	{
		(*pStatusCallback)(CommandStatus::UNDEFINED);
		return;
	}

	switch(event->GetEventType())
	{
		case EventType::Binary:
			EventT(FromODC<opendnp3::Binary>(event), event->GetIndex());
			break;
		case EventType::Analog:
			EventT(FromODC<opendnp3::Analog>(event), event->GetIndex());
			break;
		case EventType::BinaryQuality:
			EventQ<opendnp3::Binary>(FromODC<opendnp3::BinaryQuality>(event), event->GetIndex());
			break;
		case EventType::AnalogQuality:
			EventQ<opendnp3::Analog>(FromODC<opendnp3::AnalogQuality>(event), event->GetIndex());
			break;
		case EventType::ConnectState:
			break;
		default:
			(*pStatusCallback)(CommandStatus::NOT_SUPPORTED);
			return;
	}
	(*pStatusCallback)(CommandStatus::SUCCESS);
}

template<typename T, typename Q>
inline void DNP3OutstationPort::EventQ(Q qual, uint16_t index)
{
	auto lambda = [=](const T &existing)
			  {
				  uint8_t state = 0;

				  if(std::is_same<Q,opendnp3::BinaryQuality>::value)
					  state = existing.quality & static_cast<uint8_t>(opendnp3::BinaryQuality::STATE);

				  T updated = existing;
				  updated.quality = static_cast<uint8_t>(qual) | state;
				  updated.time = opendnp3::DNPTime(msSinceEpoch());
				  return updated;
			  };
	const auto modify = openpal::Function1<const T&, T>::Bind(lambda);
	{ //transaction scope
		asiodnp3::MeasUpdate tx(pOutstation);
		// TODO: confirm the timestamp used for the modify
		tx.Modify(modify, index, opendnp3::EventMode::Force);
	}
}

template<typename T>
inline void DNP3OutstationPort::EventT(T meas, uint16_t index)
{
	auto pConf = static_cast<DNP3PortConf*>(this->pConf.get());

	{ //transaction scope
		asiodnp3::MeasUpdate tx(pOutstation);
		if (
			(pConf->pPointConf->TimestampOverride == DNP3PointConf::TimestampOverride_t::ALWAYS) ||
			((pConf->pPointConf->TimestampOverride == DNP3PointConf::TimestampOverride_t::ZERO) && (meas.time == 0))
			)
		{
			meas.time = opendnp3::DNPTime(msSinceEpoch());
		}
		tx.Update(meas, index);
	}
}


