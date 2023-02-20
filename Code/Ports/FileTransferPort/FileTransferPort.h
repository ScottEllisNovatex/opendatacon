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
 * FileTransferPort.h
 *
 *  Created on: 19/01/2023
 *      Author: Neil Stephens
 */

#ifndef FileTransferPort_H_
#define FileTransferPort_H_
#include "FileTransferPortConf.h"
#include <opendatacon/DataPort.h>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <fstream>
#include <deque>
#include <atomic>

using namespace odc;

class FileTransferPort: public DataPort
{
private:
	//copy this to posted handlers so we can manage lifetime
	//std::shared_ptr<void> handler_tracker;

public:
	FileTransferPort(const std::string& aName, const std::string& aConfFilename, const Json::Value& aConfOverrides);
	~FileTransferPort();

	void Enable() override
	{
		pSyncStrand->post([this,h{handler_tracker}](){Enable_();});
	}

	void Disable() override
	{
		pSyncStrand->post([this,h{handler_tracker}](){Disable_();});
	}

	void Build() override;

	void Event(std::shared_ptr<const EventInfo> event, const std::string& SenderName, SharedStatusCallback_t pStatusCallback) override
	{
		pSyncStrand->post([=,h{handler_tracker}](){Event_(event,SenderName,pStatusCallback);});
	}

	void ProcessElements(const Json::Value& JSONRoot) override;
	const Json::Value GetStatistics() const override;

private:
	//these run only on the synchronising strand
	void Periodic(asio::error_code err, std::shared_ptr<asio::steady_timer> pTimer, size_t periodms, bool only_modified);
	void Enable_();
	void Disable_();
	void Event_(std::shared_ptr<const EventInfo> event, const std::string& SenderName, SharedStatusCallback_t pStatusCallback);
	void TxEvent(std::shared_ptr<const EventInfo> event, const std::string& SenderName, SharedStatusCallback_t pStatusCallback);
	void RxEvent(std::shared_ptr<const EventInfo> event, const std::string& SenderName, SharedStatusCallback_t pStatusCallback);
	void TrySend(const std::string& path, std::string tx_name);
	void TxPath(std::string path, const std::string& tx_name, bool only_modified);
	void Tx(bool only_modified);
	void SaveModTimes();
	void LoadModTimes();
	std::pair<bool,std::string> FileNameTransmissionMatch(const std::string& filename);

	//copy this to posted handlers so we can manage lifetime
	std::shared_ptr<void> handler_tracker = std::make_shared<char>();
	std::unique_ptr<asio::io_service::strand> pSyncStrand = pIOS->make_strand();

	bool enabled = false;
	size_t seq = 0;
	std::string Filename = "";
	std::vector<std::shared_ptr<asio::steady_timer>> Timers;
	std::unordered_map<std::string,std::filesystem::file_time_type> FileModTimes;
	bool rx_in_progress = false;
	bool tx_in_progress = false;
	std::ofstream fout;
	std::unordered_map<size_t,std::deque<std::shared_ptr<const EventInfo>>> event_buffer;
	std::unordered_map<std::string,std::string> tx_filename_q;

	//statistics
	std::atomic<size_t> FilesTransferred = 0;
	std::atomic<size_t> FileBytesTransferred = 0;
	std::atomic<size_t> TxFileDirCount = 0;
	std::atomic<size_t> TxFileMatchCount = 0;
};

#endif /* FileTransferPort_H_ */
