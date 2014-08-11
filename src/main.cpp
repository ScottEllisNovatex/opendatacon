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
 * main.cpp
 *
 *  Created on: 14/07/2014
 *      Author: Neil Stephens <dearknarl@gmail.com>
 */

#include "DataConcentrator.h"
#include "Console.h"

int main(int argc, char* argv[])
{
	try
	{
		//default for commandline args
		std::string ConfFileName = "datacon.conf";

		if (argc>1)
			ConfFileName = argv[1];

		DataConcentrator TheDataConcentrator(ConfFileName);
		TheDataConcentrator.BuildOrRebuild();
		TheDataConcentrator.Enable();

		Console console("datacon> ");

		console.run();

		TheDataConcentrator.Disable();
	}
	catch(std::exception& e)
	{
		std::cout<<"Caught exception: "<<e.what()<<std::endl;
		return 1;
	}

	return 0;
}


