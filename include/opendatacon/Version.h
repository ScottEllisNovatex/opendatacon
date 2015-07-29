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
* Version.h
*
*  Created on: 26/11/2014
*      Author: Alan Murray
*/

#ifndef ODC_VERSION_H_
#define ODC_VERSION_H_

#define ODC_VERSION_MAJOR 0
#define ODC_VERSION_MINOR 3
#define ODC_VERSION_MICRO 3

#define STR_HELPER(x) # x
#define STR(x) STR_HELPER(x)

#define ODC_VERSION_STRING "" STR(ODC_VERSION_MAJOR) "." STR(ODC_VERSION_MINOR) "." STR(ODC_VERSION_MICRO)

#endif
