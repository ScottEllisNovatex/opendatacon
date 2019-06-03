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

#ifndef PYPORT_H_
#define PYPORT_H_

// If we are compiling for external testing (or production) define this.
// If we are using VS and its test framework, don't define this.
//#define NONVSTESTING

#include <Python.h>
#include <unordered_map>
#include <opendatacon/DataPort.h>
#include <opendatacon/util.h>

#include "PyPortConf.h"


typedef asio::basic_waitable_timer<std::chrono::steady_clock> Timer_t;
typedef std::shared_ptr<Timer_t> pTimer_t;


// Hide some of the code to make Logging cleaner
#define LOGDEBUG(...) \
	if (auto log = odc::spdlog_get("CBPort")) \
		log->debug(__VA_ARGS__);
#define LOGERROR(...) \
	if (auto log = odc::spdlog_get("CBPort")) \
		log->error(__VA_ARGS__);
#define LOGWARN(...) \
	if (auto log = odc::spdlog_get("CBPort"))  \
		log->warn(__VA_ARGS__);
#define LOGINFO(...) \
	if (auto log = odc::spdlog_get("CBPort")) \
		log->info(__VA_ARGS__);

using namespace odc;

class PyPort: public DataPort
{

public:
	PyPort(const std::string& aName, const std::string& aConfFilename, const Json::Value& aConfOverrides);
	~PyPort() override;
	void ProcessElements(const Json::Value& JSONRoot) final;

	void Enable() override;
	void Disable() override;
	void Build() override;

	void Event(std::shared_ptr<const EventInfo> event, const std::string& SenderName, SharedStatusCallback_t pStatusCallback) override;

protected:
	static PyMethodDef PyModuleMethods[];
	static PyMethodDef PyPortMethods[];
	static PyTypeObject PyDataPortType;

	typedef struct
	{
		PyObject_HEAD
		/* Type-specific fields go here. */
	} PyDataPortObject;

	// Worker functions to try and clean up the code...
	PyPortConf* MyConf;

private:
	// Worker methods
	void PostCallbackCall(const odc::SharedStatusCallback_t& pStatusCallback, CommandStatus c);

	/// Python interface definition
	static PyObject* Py_init(PyObject *self, PyObject *args);
	PyObject* GetFunction(PyObject* pyInstance, std::string& sFunction);

	static PyObject* pyPublishEvent(PyObject *self, PyObject *args);

	PyObject *pyModule = nullptr;
	PyObject *pyInstance = nullptr;
	PyObject *pyFuncEnable = nullptr;
	PyObject *pyFuncDisable = nullptr;
	PyObject *pyFuncEvent = nullptr;

	static std::unordered_map<PyObject*, PyPort*> PyPorts;

	// Pulled in from PyPortManager
	void PyErrOutput();
	void PostPyCall(PyObject* pyFunction, PyObject* pyArgs);
	void PostPyCall(PyObject* pyFunction, PyObject* pyArgs, SharedStatusCallback_t pStatusCallback);
};

#endif /* PYPORT_H_ */