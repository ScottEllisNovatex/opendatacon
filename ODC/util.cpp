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
 * util.cpp
 *
 *  Created on: 14/03/2014
 *      Author: Neil Stephens <dearknarl@gmail.com>
 */

#include <opendatacon/util.h>
#include <regex>

using namespace std;

namespace ODC
{

	bool GetBool(const string& value)
	{
		if (value == "true")
		{
			return true;
		}
		else if (value == "false")
		{
			return false;
		}
		throw new std::runtime_error("Expected true or false after element name");
	}

	bool getline_noncomment(istream& is, string& line)
	{
		//chew up blank lines and comments
		do
		{
			std::getline(is, line);
		} while (std::regex_match(line, std::regex("^[:space:]*#.*|^[^_[:alnum:]]*$", std::regex::extended)) && !is.eof());

		//fail if we hit the end
		if (is.eof())
			return false;
		//success!
		return true;
	}

	bool extract_delimited_string(istream& ist, string& extracted)
	{
		extracted.clear();
		char delim;
		//The first char is the delimiter
		if (!(ist >> delim))
			return true; //nothing to extract - return successfully extracted nothing
		char ch;
		while (ist.get(ch))
		{
			//return success once we find the second delimiter
			if (ch == delim)
				return true;
			//otherwise keep extracting
			extracted.push_back(ch);
		}
		//if we get to here, there wasn't a matching end delimiter - return failed
		return false;
	}

}
