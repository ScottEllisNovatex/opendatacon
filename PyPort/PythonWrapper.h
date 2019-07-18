/*	opendatacon
 *
 *	Copyright (c) 2017:
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
 * PyPort.h
 *
 *  Created on: 26/03/2017
 *      Author: Alan Murray <alan@atmurray.net>
 */

#ifndef PYWRAPPER_H_
#define PYWRAPPER_H_

#include "Py.h"
#include <mutex>
#include <shared_mutex>
#include <unordered_set>
#include <opendatacon/util.h>
#include <opendatacon/DataPort.h>

using namespace odc;

typedef std::function<void (uint32_t, uint32_t)> SetTimerFnType;
typedef std::function<void ( const char*, uint32_t, const char*, const char*)> PublishEventCallFnType;

class PythonInitWrapper
{
public:
	PythonInitWrapper();
	~PythonInitWrapper();
};

class PythonWrapper
{
private:
	friend class PythonInitWrapper;
	static PythonInitWrapper PythonInit;

public:
	PythonWrapper(const std::string& aName, SetTimerFnType SetTimerFn, PublishEventCallFnType PublishEventCallFn);
	~PythonWrapper();
	void Build(const std::string& modulename, const std::string& pyPathName, const std::string& pyLoadModuleName,
		const std::string& pyClassName, const std::string& PortName);
	void Config(const std::string& JSONMain, const std::string& JSONOverride);
	void Enable();
	void Disable();
	CommandStatus Event(std::shared_ptr<const EventInfo> event, const std::string& SenderName);
	void CallTimerHandler(uint32_t id);
	std::string RestHandler(const std::string& url, const std::string& content);

	SetTimerFnType GetPythonPortSetTimerFn() { return PythonPortSetTimerFn; };                         // Protect set access, only allow get.
	PublishEventCallFnType GetPythonPortPublishEventCallFn() { return PythonPortPublishEventCallFn; }; // Protect set access, only allow get.

	static void PyErrOutput();

	std::string Name;

	// Do this to make sure it is always a valid pointer - the python code could pass anything back...
	static PythonWrapper* GetThisFromPythonSelf(uint64_t guid)
	{
		std::shared_lock<std::shared_timed_mutex> lck(PythonWrapper::WrapperHashMutex);
		if (PyWrappers.count(guid))
		{
			return (PythonWrapper*)guid;
		}
		return nullptr;
	}

private:
	void StoreWrapperMapping()
	{
		std::unique_lock<std::shared_timed_mutex> lck(PythonWrapper::WrapperHashMutex);
		PyWrappers.emplace((uint64_t)this);
	}
	void RemoveWrapperMapping()
	{
		std::unique_lock<std::shared_timed_mutex> lck(PythonWrapper::WrapperHashMutex);
		PyWrappers.erase((uint64_t)this);
	}

	PyObject* GetFunction(PyObject* pyInstance, const std::string& sFunction);
	PyObject* PyCall(PyObject* pyFunction, PyObject* pyArgs);

	// Keep track of each PyWrapper so static methods can get access to the correct PyPort instance
	static std::unordered_set<uint64_t> PyWrappers;
	static std::shared_timed_mutex WrapperHashMutex;
	static PyThreadState* threadState;

	// Keep pointers to the methods in out Python code that we want to be able to call.
	PyObject* pyModule = nullptr;
	PyObject* pyInstance = nullptr;
	PyObject* pyFuncConfig = nullptr;
	PyObject* pyFuncEnable = nullptr;
	PyObject* pyFuncDisable = nullptr;
	PyObject* pyFuncEvent = nullptr;
	PyObject* pyTimerHandler = nullptr;
	PyObject* pyRestHandler = nullptr;

	SetTimerFnType PythonPortSetTimerFn;
	PublishEventCallFnType PythonPortPublishEventCallFn;
};

#endif /* PYWRAPPER_H_ */
