/*
 * Licensed to Green Energy Corp (www.greenenergycorp.com) under one or
 * more contributor license agreements. See the NOTICE file distributed
 * with this work for additional information regarding copyright ownership.
 * Green Energy Corp licenses this file to you under the Apache License,
 * Version 2.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This project was forked on 01/01/2013 by Automatak, LLC and modifications
 * may have been made to this file. Automatak, LLC licenses these modifications
 * to you under the terms of the License.
 *
 * This file was forked on 24/12/2015 and modifications may have been made to
 * this file. These modifications are licensed to you under the terms of the
 * License.
 */
#include "ConsoleLogger.h"

#include <chrono>
#include <sstream>
#include <iostream>
#include <assert.h>

#include <opendnp3/LogLevels.h>

using namespace std;
using namespace opendnp3;
using namespace std::chrono;

ConsoleLogger ConsoleLogger::instance;

ConsoleLogger::ConsoleLogger() : printLocation(false)
{

}

void ConsoleLogger::SetPrintLocation(bool printLocation_)
{
	printLocation = printLocation_;
}

void ConsoleLogger::Log(const openpal::LogEntry& entry)
{
	auto time = std::chrono::high_resolution_clock::now();
	auto num = duration_cast<milliseconds>(time.time_since_epoch()).count();

	ostringstream oss;

	oss << "ms(" << num << ") " << LogFlagToString(entry.GetFilters().GetBitfield());
	oss << " " << entry.GetAlias();
	if (printLocation)
	{
		oss << " - " << entry.GetLocation();
	}
	oss << " - " << entry.GetMessage();

	if (entry.GetErrorCode() != -1)
	{
		oss << " - " << entry.GetErrorCode();
	}

	std::unique_lock<std::mutex> lock(mutex);
	std::cout << oss.str() << std::endl;
}

