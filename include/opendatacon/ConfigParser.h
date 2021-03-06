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
 * ConfigParser.h
 *
 *  Created on: 16/07/2014
 *      Author: Neil Stephens <dearknarl@gmail.com>
 */

#ifndef CONFIGPARSER_H_
#define CONFIGPARSER_H_

#include <json/json.h>
#include <opendatacon/Platform.h>
#include <unordered_map>
#include <memory>

class DataConcentrator;
class ConfigParser
{
	friend class DataConcentrator;
public:
	ConfigParser(const std::string& aConfFilename, const Json::Value& aConfOverrides = Json::Value());
	virtual ~ConfigParser(){}
	Json::Value GetConfiguration() const;

protected:
	void ProcessFile();
	virtual void ProcessElements(const Json::Value& JSONRoot) = 0;

	std::string ConfFilename;
	const Json::Value ConfOverrides;

private:
	void ProcessInherits(const std::string& FileName);

	//static members
	static std::shared_ptr<const Json::Value> RecallOrCreate(const std::string& FileName);
	static void AddInherits(Json::Value& JSONRoot, const Json::Value& Inherits);
	static Json::Value GetConfiguration(const std::string& aConfFilename, const Json::Value& aConfOverrides);

	//On windows we still need to decorate data member exports for DLL import
	//All other symbols are exported/imported automatically using cmake WINDOWS_EXPORT_ALL_SYMBOLS
	//JSONFileCache is used in the main exe - so we need to mark it
	DllImport static std::unordered_map<std::string,std::shared_ptr<Json::Value>> JSONFileCache;
};

#endif /* CONFIGPARSER_H_ */
