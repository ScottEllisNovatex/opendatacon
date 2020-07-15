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
 * SimPort.cpp
 *
 *  Created on: 29/07/2015
 *      Author: Neil Stephens <dearknarl@gmail.com>
 */

#include "SimPort.h"
#include "SimPortCollection.h"
#include "SimPortConf.h"
#include "sqlite3/sqlite3.h"
#include <opendatacon/IOTypes.h>
#include <opendatacon/util.h>
#include <opendatacon/Version.h>
#include <chrono>
#include <limits>
#include <memory>
#include <random>

thread_local std::mt19937 SimPort::RandNumGenerator = std::mt19937(std::random_device()());

std::string StringToLower(std::string strToConvert)
{
	std::transform(strToConvert.begin(), strToConvert.end(), strToConvert.begin(), (int (*)(int)) std::tolower); // Specific overload requested..google it

	return strToConvert;
}
std::vector<std::string> split(const std::string& s, char delimiter)
{
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(s);
	while (std::getline(tokenStream, token, delimiter))
	{
		tokens.push_back(token);
	}
	return tokens;
}

//Implement DataPort interface
SimPort::SimPort(const std::string& Name, const std::string& File, const Json::Value& Overrides):
	DataPort(Name, File, Overrides),
	SimCollection(nullptr)
{
	static std::atomic_flag init_flag = ATOMIC_FLAG_INIT;
	static std::weak_ptr<SimPortCollection> weak_collection;

	//if we're the first/only one on the scene,
	// init the SimPortCollection
	if(!init_flag.test_and_set(std::memory_order_acquire))
	{
		//make a custom deleter for the DNP3Manager that will also clear the init flag
		auto deinit_del = [](SimPortCollection* collection_ptr)
					{init_flag.clear(); delete collection_ptr;};
		this->SimCollection = std::shared_ptr<SimPortCollection>(new SimPortCollection(), deinit_del);
		weak_collection = this->SimCollection;
	}
	//otherwise just make sure it's finished initialising and take a shared_ptr
	else
	{
		while (!(this->SimCollection = weak_collection.lock()))
		{} //init happens very seldom, so spin lock is good
	}

	pConf = std::make_unique<SimPortConf>();
	pSimConf = static_cast<SimPortConf*>(this->pConf.get());
	pSimConf->SetName(Name);
	ProcessFile();
}

void SimPort::Enable()
{
	pEnableDisableSync->post([&]()
		{
			if(!enabled)
			{
			      enabled = true;
			      HttpServerManager::StartConnection(pServer);
			      PortUp();
			}
		});
}

void SimPort::Disable()
{
	pEnableDisableSync->post([&]()
		{
			if(enabled)
			{
			      enabled = false;
			      HttpServerManager::StopConnection(pServer);
			      PortDown();
			}
		});
}

// Just a way to keep the BinaryVals and AnalogVals up to date...
void SimPort::PostPublishEvent(std::shared_ptr<EventInfo> event, SharedStatusCallback_t pStatusCallback = std::make_shared<std::function<void(CommandStatus status)>>([](CommandStatus status) {}))
{
	PublishEvent(event, pStatusCallback);

	// If we publish it, the value has changed so update current values.
	size_t index = event->GetIndex();
	if (event->GetEventType() == EventType::Analog)
	{
		double val = event->GetPayload<EventType::Analog>();
		pIOS->post([&, index, val]()
			{
				std::unique_lock<std::shared_timed_mutex> lck(ConfMutex);
				pSimConf->AnalogVals[index] = val;
			});
	}
	if (event->GetEventType() == EventType::Binary)
	{
		bool val = event->GetPayload<EventType::Binary>();
		pIOS->post([&, index, val]()
			{
				std::unique_lock<std::shared_timed_mutex> lck(ConfMutex);
				pSimConf->BinaryVals[index] = val;
			});
	}
}

std::pair<std::string, std::shared_ptr<IUIResponder> > SimPort::GetUIResponder()
{
	return std::pair<std::string,std::shared_ptr<SimPortCollection>>("SimControl",this->SimCollection);
}

const Json::Value SimPort::GetCurrentState() const
{
	Json::Value current_state;
	{ //lock scope
		std::shared_lock<std::shared_timed_mutex> lck(ConfMutex);
		for(const auto& ind_val_pair : pSimConf->BinaryVals)
			current_state["BinaryCurrent"][ind_val_pair.first] = ind_val_pair.second;
		for(const auto& ind_val_pair : pSimConf->AnalogVals)
			current_state["AnalogCurrent"][ind_val_pair.first] = ind_val_pair.second;
	}
	return current_state;
}
const Json::Value SimPort::GetStatistics() const
{
	Json::Value stats;
	return stats;
}
const Json::Value SimPort::GetStatus() const
{
	Json::Value status;
	return status;
}

std::vector<uint32_t> SimPort::GetAllowedIndexes(std::string type)
{
	std::vector<uint32_t> allowed_indexes;
	// Take a copy of the shared allowed_indexs while mutex protected.
	if (StringToLower(type) == "analog")
	{
		std::shared_lock<std::shared_timed_mutex> lck(ConfMutex);
		allowed_indexes = pSimConf->AnalogIndicies;
	}
	else if (StringToLower(type) == "binary")
	{
		std::shared_lock<std::shared_timed_mutex> lck(ConfMutex);
		allowed_indexes = pSimConf->BinaryIndicies;
	}
	return allowed_indexes;
}


std::vector<uint32_t> SimPort::IndexesFromString(const std::string& index_str, const std::string& type)
{
	std::vector<uint32_t> indexes;
	// Take a copy of the shared allowed_indexs while mutex protected.
	std::vector<uint32_t> allowed_indexes = GetAllowedIndexes(type);

	if (allowed_indexes.size() == 0)
		return indexes;

	//Check for comma separated list,no white space
	std::regex comma_regx("^[0-9]+(?:,[0-9]+)*$");

	if(std::regex_match(index_str, comma_regx))
	{
		// We have matched the regex. Now split.
		auto idxstrings = split(index_str, ',');
		for(const auto& idxs : idxstrings)
		{
			size_t idx;
			try
			{
				idx = std::stoi(idxs);
			}
			catch(std::exception& e)
			{
				continue;
			}
			for(auto allowed : allowed_indexes)
			{
				if(idx == allowed)
				{
					indexes.push_back(idx);
					break;
				}
			}
		}
	}
	else //Not comma seaparated list
	{//use it as a regex
		try
		{
			std::regex ind_regex(index_str,std::regex::extended);
			for(auto allowed : allowed_indexes)
			{
				if(std::regex_match(std::to_string(allowed),ind_regex))
					indexes.push_back(allowed);
			}
		}
		catch(std::exception& e)
		{}
	}
	return indexes;
}

bool SimPort::UILoad(const std::string& type, const std::string& index, const std::string& value, const std::string& quality, const std::string& timestamp, const bool force)
{
	if (auto log = odc::spdlog_get("SimPort"))
	{
		log->debug("{} : UILoad : {}, {}, {}, {}, {}, {}", Name, type, index, value, quality, timestamp, force);
	}

	double val;
	try
	{
		val = std::stod(value);
	}
	catch(std::exception& e)
	{
		return false;
	}

	auto indexes = IndexesFromString(index,type);
	if(!indexes.size())
		return false;

	QualityFlags Q;
	if(quality == "")
		Q = QualityFlags::ONLINE;
	else if(!GetQualityFlagsFromStringName(quality, Q))
		return false;

	msSinceEpoch_t ts;
	if(timestamp == "")
		ts = msSinceEpoch();
	else
	{
		try
		{
			ts = std::stoull(timestamp);
		}
		catch(std::exception& e)
		{
			return false;
		}
	}

	if(StringToLower(type) == "binary")
	{
		for(auto idx : indexes)
		{
			if (force)
			{ //lock scope
				std::unique_lock<std::shared_timed_mutex> lck(ConfMutex);
				pSimConf->BinaryForcedStates[idx] = true;
			}
			auto event = std::make_shared<EventInfo>(EventType::Binary,idx,Name,Q,ts);
			bool valb = (val >= 1);
			event->SetPayload<EventType::Binary>(std::move(valb));
			PostPublishEvent(event);
		}
	}
	else if(StringToLower(type) == "analog")
	{
		for(auto idx : indexes)
		{
			if (force)
			{ //lock scope
				std::unique_lock<std::shared_timed_mutex> lck(ConfMutex);
				pSimConf->AnalogForcedStates[idx] = true;
			}
			auto event = std::make_shared<EventInfo>(EventType::Analog,idx,Name,Q,ts);
			event->SetPayload<EventType::Analog>(std::move(val));
			PostPublishEvent(event);
		}
	}
	else
		return false;

	return true;
}

bool SimPort::UISetUpdateInterval(const std::string& type, const std::string& index, const std::string& period)
{
	unsigned int delta;
	try
	{
		delta = std::stoi(period);
	}
	catch(std::exception& e)
	{
		return false;
	}

	auto indexes = IndexesFromString(index,type);
	if(!indexes.size())
		return false;

	if(StringToLower(type) == "binary")
	{
		for(auto idx : indexes)
		{
			{ //lock scope
				std::unique_lock<std::shared_timed_mutex> lck(ConfMutex);
				if(!pSimConf->BinaryStartVals.count(idx))
					return false;
				pSimConf->BinaryUpdateIntervalms[idx] = delta;
			}
			auto pTimer = Timers.at("Binary"+std::to_string(idx));
			if(!delta) //zero means no updates
			{
				pTimer->cancel();
			}
			else
			{
				auto random_interval = std::uniform_int_distribution<unsigned int>(0, 2*delta)(RandNumGenerator);
				pTimer->expires_from_now(std::chrono::milliseconds(random_interval));
				pTimer->async_wait([=](asio::error_code err_code)
					{
						if(enabled && !err_code)
							StartBinaryEvents(idx);
					});
			}
		}
	}
	else if(StringToLower(type) == "analog")
	{
		for(auto idx : indexes)
		{
			{ //lock scope
				std::unique_lock<std::shared_timed_mutex> lck(ConfMutex);
				if(!pSimConf->AnalogStartVals.count(idx))
					return false;
				pSimConf->AnalogUpdateIntervalms[idx] = delta;
			}
			auto pTimer = Timers.at("Analog"+std::to_string(idx));
			if(!delta) //zero means no updates
			{
				pTimer->cancel();
			}
			else
			{
				auto random_interval = std::uniform_int_distribution<unsigned int>(0, 2*delta)(RandNumGenerator);
				pTimer->expires_from_now(std::chrono::milliseconds(random_interval));
				pTimer->async_wait([=](asio::error_code err_code)
					{
						if(enabled && !err_code)
							StartAnalogEvents(idx);
					});
			}
		}
	}
	else
		return false;

	return true;
}

bool SimPort::UIRelease(const std::string& type, const std::string& index)
{
	return SetForcedState(index, type, false);
}

bool SimPort::SetForcedState(const std::string& index, const std::string& type, bool forced)
{
	auto indexes = IndexesFromString(index, type);
	if (!indexes.size())
		return false;

	if (StringToLower(type) == "binary")
	{
		std::unique_lock<std::shared_timed_mutex> lck(ConfMutex);
		for (auto idx : indexes)
		{
			pSimConf->BinaryForcedStates[idx] = forced;
		}
	}
	else if (StringToLower(type) == "analog")
	{
		std::unique_lock<std::shared_timed_mutex> lck(ConfMutex);
		for (auto idx : indexes)
		{
			pSimConf->AnalogForcedStates[idx] = forced;
		}
	}
	else
		return false;

	return true;
}
std::string SimPort::GetCurrentBinaryValsAsJSONString(const std::string& index)
{
	auto indexes = IndexesFromString(index, "binary");
	if (!indexes.size())
		return "";
	Json::Value root;

	{
		std::unique_lock<std::shared_timed_mutex> lck(ConfMutex);
		for (auto idx : indexes)
		{
			root[std::to_string(idx)] = pSimConf->BinaryVals[idx] ? "1" : "0";
		}
	}
	Json::StreamWriterBuilder wbuilder;
	wbuilder["indentation"] = "";
	const std::string result = Json::writeString(wbuilder, root);
	return result;
}

std::string SimPort::GetCurrentAnalogValsAsJSONString(const std::string& index)
{
	auto indexes = IndexesFromString(index, "analog");
	if (!indexes.size())
		return "";
	Json::Value root;

	{
		std::unique_lock<std::shared_timed_mutex> lck(ConfMutex);
		for (auto idx : indexes)
		{
			root[std::to_string(idx)] = std::to_string(pSimConf->AnalogVals[idx]);
		}
	}
	Json::StreamWriterBuilder wbuilder;
	wbuilder["indentation"] = "";
	const std::string result = Json::writeString(wbuilder, root);
	return result;
}

void SimPort::PortUp()
{
	auto now = msSinceEpoch();
	std::vector<uint32_t> indexes = GetAllowedIndexes("analog"); // Mutex protected copy of indexes.

	for(auto index : indexes)
	{
		pTimer_t pTimer = pIOS->make_steady_timer();
		Timers["Analog"+std::to_string(index)] = pTimer;

		//Check if we're configured to load this point from DB
		const auto db_stats = pSimConf->GetDBStats();
		if(db_stats.find("Analog"+std::to_string(index)) != db_stats.end())
		{
			auto event = std::make_shared<EventInfo>(EventType::Analog,index,Name);
			NextEventFromDB(event);
			int64_t time_offset = 0;
			const auto timestamp_handling = pSimConf->GetTimestampHandling();
			if(!(timestamp_handling & TimestampMode::ABSOLUTE_T))
			{
				if(!!(timestamp_handling & TimestampMode::FIRST))
				{
					time_offset = now - event->GetTimestamp();
				}
				else if(!!(timestamp_handling & TimestampMode::TOD))
				{
					auto whole_days_ts = std::chrono::duration_cast<days>(std::chrono::milliseconds(event->GetTimestamp()));
					auto whole_days_now = std::chrono::duration_cast<days>(std::chrono::milliseconds(now));
					time_offset = std::chrono::duration_cast<std::chrono::milliseconds>(whole_days_now).count()
					              - std::chrono::duration_cast<std::chrono::milliseconds>(whole_days_ts).count();
				}
				else
				{
					throw std::runtime_error("Invalid timestamp mode: Not absolute, but not relative either");
				}
			}
			if(!(timestamp_handling & TimestampMode::FASTFORWARD))
			{
				//Find the first event that's not in the past
				while(now > (event->GetTimestamp()+time_offset))
					NextEventFromDB(event);
			}

			//TODO: remeber the last event in the past to send an initial event
			//posting this is essential - otherwise SpawnEvent will deadlock the mutex we have locked
//			pIOS->post([this,event,time_offset]()
//				{
//					SpawnEvent(last_event, time_offset);
//				});

			msSinceEpoch_t delta;
			if(now > event->GetTimestamp())
				delta = 0;
			else
				delta = event->GetTimestamp() - now;
			pTimer->expires_from_now(std::chrono::milliseconds(delta));
			pTimer->async_wait([=](asio::error_code err_code)
				{
					if(enabled && !err_code)
						SpawnEvent(event, time_offset);
					//else - break timer cycle
				});

			continue;
		}

		double mean;
		{ //lock scope
			std::unique_lock<std::shared_timed_mutex> lck(ConfMutex);
			//send initial event
			mean = pSimConf->AnalogStartVals.count(index) ? pSimConf->AnalogStartVals.at(index) : 0;
			pSimConf->AnalogStartVals[index] = mean;
		}
		auto event = std::make_shared<EventInfo>(EventType::Analog,index,Name,QualityFlags::ONLINE);
		event->SetPayload<EventType::Analog>(std::move(mean));
		PostPublishEvent(event);

		//queue up a timer if it has an update interval
		{ //lock scope
			std::unique_lock<std::shared_timed_mutex> lck(ConfMutex);
			if (pSimConf->AnalogUpdateIntervalms.count(index))
			{
				auto interval = pSimConf->AnalogUpdateIntervalms[index];
				auto std_dev = pSimConf->AnalogStdDevs.count(index) ? pSimConf->AnalogStdDevs.at(index) : (mean ? (pSimConf->default_std_dev_factor * mean) : 20);
				pSimConf->AnalogStdDevs[index] = std_dev;

				auto random_interval = std::uniform_int_distribution<unsigned int>(0, 2 * interval)(RandNumGenerator);
				pTimer->expires_from_now(std::chrono::milliseconds(random_interval));
				pTimer->async_wait([=](asio::error_code err_code)
					{
						if (enabled && !err_code)
							StartAnalogEvents(index);
					});
			}
		}
	}

	indexes = GetAllowedIndexes("binary"); // Mutex protected copy of indexes.

	for(auto index : indexes)
	{
		//send initial event
		bool val;
		{ //lock scope
			std::unique_lock<std::shared_timed_mutex> lck(ConfMutex);
			val = pSimConf->BinaryStartVals.count(index) ? pSimConf->BinaryStartVals.at(index) : false;
			pSimConf->BinaryStartVals[index] = val;
		}
		auto event = std::make_shared<EventInfo>(EventType::Binary,index,Name,QualityFlags::ONLINE);
		event->SetPayload<EventType::Binary>(std::move(val));
		PostPublishEvent(event);

		pTimer_t pTimer = pIOS->make_steady_timer();
		Timers["Binary"+std::to_string(index)] = pTimer;

		//queue up a timer if it has an update interval
		{ //lock scope
			std::unique_lock<std::shared_timed_mutex> lck(ConfMutex);
			if (pSimConf->BinaryUpdateIntervalms.count(index))
			{
				auto interval = pSimConf->BinaryUpdateIntervalms[index];

				auto random_interval = std::uniform_int_distribution<unsigned int>(0, 2 * interval)(RandNumGenerator);
				pTimer->expires_from_now(std::chrono::milliseconds(random_interval));
				pTimer->async_wait([=](asio::error_code err_code)
					{
						if (enabled && !err_code)
							StartBinaryEvents(index, !val);
					});
			}
		}
	}
}

void SimPort::PortDown()
{
	for(const auto& pTimer : Timers)
		pTimer.second->cancel();
	Timers.clear();
}

void SimPort::NextEventFromDB(const std::shared_ptr<EventInfo>& event)
{
	if(event->GetEventType() == EventType::Analog)
	{
		auto db_stats = pSimConf->GetDBStats();
		auto rv = sqlite3_step(db_stats["Analog"+std::to_string(event->GetIndex())].get());
		if(rv == SQLITE_ROW)
		{
			auto t = static_cast<msSinceEpoch_t>(sqlite3_column_int64(db_stats["Analog"+std::to_string(event->GetIndex())].get(),0));
			//TODO: apply some relative offset to time
			event->SetTimestamp(t);
			event->SetPayload<EventType::Analog>(sqlite3_column_double(db_stats["Analog"+std::to_string(event->GetIndex())].get(),1));
		}
		else
		{
			//wait forever
			auto forever = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::duration::max()).count();
			event->SetTimestamp(forever);
		}
	}
	else if(auto log = odc::spdlog_get("SimPort"))
	{
		log->error("{} : Unsupported EventType : '{}'", ToString(event->GetEventType()));
		return;
	}
}

void SimPort::PopulateNextEvent(const std::shared_ptr<EventInfo>& event, int64_t time_offset)
{
	//Check if we're configured to load this point from DB
	const auto db_stats = pSimConf->GetDBStats();
	if(db_stats.find("Analog"+std::to_string(event->GetIndex())) !=
	   db_stats.end())
	{
		NextEventFromDB(event);
		event->SetTimestamp(event->GetTimestamp()+time_offset);
		return;
	}

	//Otherwise do random
	unsigned int interval;
	if(event->GetEventType() == EventType::Analog)
	{
		RandomiseAnalog(event);
		{ //lock scope
			std::shared_lock<std::shared_timed_mutex> lck(ConfMutex);
			interval = pSimConf->AnalogUpdateIntervalms.at(event->GetIndex());
		}
	}
	else if(event->GetEventType() == EventType::Binary)
	{
		bool val = !event->GetPayload<EventType::Binary>();
		event->SetPayload<EventType::Binary>(std::move(val));
		{ //lock scope
			std::shared_lock<std::shared_timed_mutex> lck(ConfMutex);
			interval = pSimConf->BinaryUpdateIntervalms.at(event->GetIndex());
		}
	}
	else if(auto log = odc::spdlog_get("SimPort"))
	{
		log->error("{} : Unsupported EventType : '{}'", ToString(event->GetEventType()));
		return;
	}
	else
		return;

	auto random_interval = std::uniform_int_distribution<unsigned int>(0, 2*interval)(RandNumGenerator);
	event->SetTimestamp(msSinceEpoch()+random_interval);
}

void SimPort::SpawnEvent(const std::shared_ptr<EventInfo>& event, int64_t time_offset)
{
	//deep copy event to modify as next event
	auto next_event = std::make_shared<EventInfo>(*event);

	bool shouldPub = true;
	if (event->GetEventType() == EventType::Analog)
	{
		std::shared_lock<std::shared_timed_mutex> lck(ConfMutex);
		if (pSimConf->AnalogForcedStates.count(event->GetIndex()))
			shouldPub = !pSimConf->AnalogForcedStates.at(event->GetIndex());
	}
	if (event->GetEventType() == EventType::Binary)
	{
		std::shared_lock<std::shared_timed_mutex> lck(ConfMutex);
		if (pSimConf->BinaryForcedStates.count(event->GetIndex()))
			shouldPub = !pSimConf->BinaryForcedStates.at(event->GetIndex());
	}

	if (shouldPub)
		PostPublishEvent(event);

	auto pTimer = Timers.at(ToString(event->GetEventType()) +std::to_string(event->GetIndex()));
	PopulateNextEvent(next_event, time_offset);
	auto now = msSinceEpoch();
	msSinceEpoch_t delta;
	if(now > next_event->GetTimestamp())
		delta = 0;
	else
		delta = next_event->GetTimestamp() - now;
	pTimer->expires_from_now(std::chrono::milliseconds(delta));
	//wait til next time
	pTimer->async_wait([=](asio::error_code err_code)
		{
			if(enabled && !err_code)
				SpawnEvent(next_event, time_offset);
			//else - break timer cycle
		});
}

void SimPort::Build()
{
	pEnableDisableSync = pIOS->make_strand();
	auto shared_this = std::static_pointer_cast<SimPort>(shared_from_this());
	this->SimCollection->Add(shared_this,this->Name);

	if ((pSimConf->HttpAddr.length() != 0) && (pSimConf->HttpPort.length() != 0))
	{
		pServer = HttpServerManager::AddConnection(pIOS, pSimConf->HttpAddr, pSimConf->HttpPort); //Static method - creates a new HttpServerManager if required

		// Now add all the callbacks that we need - the root handler might be a duplicate, in which case it will be ignored!

		auto roothandler = std::make_shared<http::HandlerCallbackType>([](const std::string& absoluteuri, const http::ParameterMapType& parameters, const std::string& content, http::reply& rep)
			{
				rep.status = http::reply::ok;
				rep.content.append("You have reached the SimPort http interface.<br>To talk to a port the url must contain the SimPort name, which is case senstive.<br>The rest is formatted as if it were a console UI command.");
				rep.headers.resize(2);
				rep.headers[0].name = "Content-Length";
				rep.headers[0].value = std::to_string(rep.content.size());
				rep.headers[1].name = "Content-Type";
				rep.headers[1].value = "text/html"; // http::server::mime_types::extension_to_type(extension);
			});

		HttpServerManager::AddHandler(pServer, "GET /", roothandler);

		std::string VersionResp = fmt::format("{{\"ODCVersion\":\"{}\",\"ConfigFileVersion\":\"{}\"}}", ODC_VERSION_STRING, pSimConf->Version);
		auto versionhandler = std::make_shared<http::HandlerCallbackType>([=](const std::string& absoluteuri, const http::ParameterMapType& parameters, const std::string& content, http::reply& rep)
			{
				rep.status = http::reply::ok;

				rep.content.append(VersionResp);
				rep.headers.resize(2);
				rep.headers[0].name = "Content-Length";
				rep.headers[0].value = std::to_string(rep.content.size());
				rep.headers[1].name = "Content-Type";
				rep.headers[1].value = "application/json"; // http::server::mime_types::extension_to_type(extension);
			});

		HttpServerManager::AddHandler(pServer, "GET /Version", versionhandler);

		auto gethandler = std::make_shared<http::HandlerCallbackType>([&](const std::string& absoluteuri, const http::ParameterMapType& parameters, const std::string& content, http::reply& rep)
			{
				// So when we hit here, someone has made a Get request of our Named Port.
				// The parameters are type and index, we return value, quality and timestamp
				// Split the absolute uri into parameters.

				std::string type = "";
				std::string index = "";
				std::string error = "";

				if (parameters.count("type") != 0)
					type = parameters.at("type");
				else
					error = "No 'type' parameter found";

				if (parameters.count("index") != 0)
					index = parameters.at("index");
				else
					error += " No 'index' parameter found";

				std::string result = "";
				std::string contenttype = "application/json";

				if (error.length() == 0)
				{
				      if (StringToLower(type) == "binary")
				      {
				            result = GetCurrentBinaryValsAsJSONString(index);
					}
				      if (StringToLower(type) == "analog")
				      {
				            result = GetCurrentAnalogValsAsJSONString(index);
					}
				}
				if (result.length() != 0)
				{
				      rep.status = http::reply::ok;
				      rep.content.append(result);
				}
				else
				{
				      rep.status = http::reply::not_found;
				      contenttype = "text/html";
				      rep.content.append("You have reached the SimPort Instance with GET on " + Name + " Invalid Request " + type + ", " + index + " - " + error);
				}
				rep.headers.resize(2);
				rep.headers[0].name = "Content-Length";
				rep.headers[0].value = std::to_string(rep.content.size());
				rep.headers[1].name = "Content-Type";
				rep.headers[1].value = contenttype;
			});
		HttpServerManager::AddHandler(pServer, "GET /" + Name, gethandler);

		auto posthandler = std::make_shared<http::HandlerCallbackType>([&](const std::string& absoluteuri, const http::ParameterMapType& parameters, const std::string& content, http::reply& rep)
			{
				// So when we hit here, someone has made a POST request of our Port.
				// The UILoad checks the values past to it and sets sensible defaults if they are missing.

				std::string type = "";
				std::string index = "";
				std::string value = "";
				std::string quality = "";
				std::string timestamp = "";
				std::string period = "";
				std::string error = "";

				if (parameters.count("type") != 0)
					type = parameters.at("type");
				else
					error = "No 'type' parameter found";

				if (parameters.count("index") != 0)
					index = parameters.at("index");
				else
					error += " No 'index' parameter found";

				if (parameters.count("quality") != 0)
					quality = parameters.at("quality");
				if (parameters.count("timestamp") != 0)
					timestamp = parameters.at("timestamp");

				if (parameters.count("force") != 0)
				{
				      if (((StringToLower(parameters.at("force")) == "true") || (parameters.at("force") == "1")))
				      {
				            SetForcedState(index, type, true);
				            rep.content.append("Set Period Command Accepted\n");
					}
				      if (((StringToLower(parameters.at("force")) == "false") || (parameters.at("force") == "0")))
				      {
				            SetForcedState(index, type, false);
				            rep.content.append("Set Period Command Accepted\n");
					}
				}

				if (parameters.count("value") != 0)
					value = parameters.at("value");
				if (parameters.count("period") != 0)
					period = parameters.at("period");

				if ((error.length() == 0) && (value.length() != 0) && UILoad(type, index, value, quality, timestamp, false)) // Forced set above
				{
				      rep.status = http::reply::ok;
				      rep.content.append("Set Value Command Accepted\n");
				}

				if ((error.length() == 0) && (period.length() != 0) && UISetUpdateInterval(type, index, period))
				{
				      rep.status = http::reply::ok;
				      rep.content.append("Set Period Command Accepted\n");
				}

				if ((value.length() == 0) && (period.length() != 0))
				{
				      error += " Missing a value or period parameter. Must have at least one.";
				}

				if (error.length() != 0)
				{
				      rep.status = http::reply::not_found;
				      rep.content.append("You have reached the SimPort Instance with POST on " + Name + " POST Command Failed - " + error);
				}
				rep.headers.resize(2);
				rep.headers[0].name = "Content-Length";
				rep.headers[0].value = std::to_string(rep.content.size());
				rep.headers[1].name = "Content-Type";
				rep.headers[1].value = "text/html";
			});
		HttpServerManager::AddHandler(pServer, "POST /" + Name, posthandler);
	}
}

void SimPort::ProcessElements(const Json::Value& json_root)
{
	pSimConf->ProcessElements(json_root);
}

void SimPort::Event(std::shared_ptr<const EventInfo> event, const std::string& SenderName, SharedStatusCallback_t pStatusCallback)
{
	if(auto log = odc::spdlog_get("SimPort"))
		log->trace("{}: Recieved control.", Name);
	if(event->GetEventType() != EventType::ControlRelayOutputBlock)
	{
		if(auto log = odc::spdlog_get("SimPort"))
			log->trace("{}: Control code not supported.", Name);
		(*pStatusCallback)(CommandStatus::NOT_SUPPORTED);
		return;
	}
	auto index = event->GetIndex();
	auto& command = event->GetPayload<EventType::ControlRelayOutputBlock>();
	std::vector<unsigned int> indexes;
	{
		std::shared_lock<std::shared_timed_mutex> lck(ConfMutex);
		indexes = pSimConf->ControlIndicies;
	}
	for(auto i : indexes)
	{
		if(i == index)
		{
			if(auto log = odc::spdlog_get("SimPort"))
				log->trace("{}: Control {}: Matched configured index.", Name, index);

			std::shared_lock<std::shared_timed_mutex> lck(ConfMutex);

			if(pSimConf->ControlFeedback.count(i))
			{
				if(auto log = odc::spdlog_get("SimPort"))
					log->trace("{}: Control {}: Setting ({}) control feedback point(s)...", Name, index, pSimConf->ControlFeedback[i].size());
				for(auto& fb : pSimConf->ControlFeedback[i])
				{
					if(fb.mode == FeedbackMode::PULSE)
					{
						if(auto log = odc::spdlog_get("SimPort"))
							log->trace("{}: Control {}: Pulse feedback to Binary {}.", Name, index,fb.on_value->GetIndex());
						switch(command.functionCode)
						{
							case ControlCode::PULSE_ON:
							case ControlCode::LATCH_ON:
							case ControlCode::LATCH_OFF:
							case ControlCode::CLOSE_PULSE_ON:
							case ControlCode::TRIP_PULSE_ON:
							{
								if(!pSimConf->BinaryForcedStates[fb.on_value->GetIndex()])
									PostPublishEvent(fb.on_value);
								pTimer_t pTimer = pIOS->make_steady_timer();
								pTimer->expires_from_now(std::chrono::milliseconds(command.onTimeMS));
								pTimer->async_wait([pTimer,fb,this](asio::error_code err_code)
									{
										//FIXME: check err_code?
										if(!pSimConf->BinaryForcedStates[fb.off_value->GetIndex()])
											PostPublishEvent(fb.off_value);
									});
								//TODO: (maybe) implement multiple pulses - command has count and offTimeMS
								break;
							}
							default:
								(*pStatusCallback)(CommandStatus::NOT_SUPPORTED);
								return;
						}
					}
					else //LATCH
					{
						switch(command.functionCode)
						{
							case ControlCode::LATCH_ON:
							case ControlCode::CLOSE_PULSE_ON:
							case ControlCode::PULSE_ON:
								if(auto log = odc::spdlog_get("SimPort"))
									log->trace("{}: Control {}: Latch on feedback to Binary {}.",
										Name, index,fb.on_value->GetIndex());
								fb.on_value->SetTimestamp();
								if(!pSimConf->BinaryForcedStates[fb.on_value->GetIndex()])
									PostPublishEvent(fb.on_value);
								break;
							case ControlCode::LATCH_OFF:
							case ControlCode::TRIP_PULSE_ON:
							case ControlCode::PULSE_OFF:
								if(auto log = odc::spdlog_get("SimPort"))
									log->trace("{}: Control {}: Latch off feedback to Binary {}.",
										Name, index,fb.off_value->GetIndex());
								fb.off_value->SetTimestamp();
								if(!pSimConf->BinaryForcedStates[fb.off_value->GetIndex()])
									PostPublishEvent(fb.off_value);
								break;
							default:
								(*pStatusCallback)(CommandStatus::NOT_SUPPORTED);
								return;
						}
					}
				}
				(*pStatusCallback)(CommandStatus::SUCCESS);
				return;
			}
			else
			{
				if(auto log = odc::spdlog_get("SimPort"))
					log->trace("{}: Control {}: No feeback points configured.", Name, index);
			}
			(*pStatusCallback)(CommandStatus::UNDEFINED);
			return;
		}
	}
	(*pStatusCallback)(CommandStatus::NOT_SUPPORTED);
}

