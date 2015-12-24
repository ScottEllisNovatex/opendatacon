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
#ifndef CONSOLELOGGER_H
#define CONSOLELOGGER_H

#include <mutex>
#include <openpal/logging/ILogHandler.h>

std::ostringstream& operator<<(std::ostringstream& ss, const openpal::LogFilters& filters);

/**
* Singleton class that prints all log messages to the console
*
*
*/
class ConsoleLogger : public openpal::ILogHandler
{

public:

	virtual void Log(const openpal::LogEntry& entry) override final;

	void SetPrintLocation(bool printLocation);

	static openpal::ILogHandler& Instance()
	{
		return instance;
	};

private:

	ConsoleLogger();

	static ConsoleLogger instance;

	bool printLocation;

	std::mutex mutex;
};

#endif
