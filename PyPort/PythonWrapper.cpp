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
 * PyPortPythonSupport.cpp
 *
 *  Created on: 26/03/2017
 *      Author: Alan Murray <alan@atmurray.net>
 *		Extensively modifed: Scott Ellis <scott.ellis@novatex.com.au>
 */


// Python Interface:
// We need to pass the config JSON (both main and overrides) to the Python class, so that it has all the information that we have.
// We need to call a Python method every time we get an event, we assume it will not call network or other long running functions. We wait for it to finish.
// We need to export a function to the Python code, so that it can Post an Event into the ODC event bus.
// Do we need to call a Python like timer method, that will allow the Python code to set how long it should be until it is next called.
// So leave the extension bit out for the moment, just get to the pont where we can load the class and call its methods...


#include "PythonWrapper.h"
#include <chrono>
#include <ctime>
#include <datetime.h> //PyDateTime
#include <time.h>
#include <iomanip>
#include <exception>
#include "PyPort.h"


using namespace odc;

msSinceEpoch_t PyDateTime_to_msSinceEpoch_t(PyObject* pyTime);

std::atomic_uint PythonWrapper::InterpreterUseCount = 0;
std::unordered_map<uint64_t, int> PythonWrapper::PyWrappers;


#pragma region odc support module
// This is where we expose ODC methods to our script, so they can be called by the script when needed.

// This is an extension method that we have provided to our embedded Python. It will feed a message into our logging framework
// It is static, so we have to work out which instance of the PythonWrapper class should handle it.
static PyObject* odc_log(PyObject* self, PyObject* args)
{
	uint32_t logtype;
	uint64_t guid;
	const char* message;

	// Now parse the arguments provided, one Unsigned int (I) and a string (s) and the function name. DO NOT GET THIS WRONG!
	if (!PyArg_ParseTuple(args, "LIs:log",&guid, &logtype, &message))
	{
		if (PyErr_Occurred())
		{
			PyErr_Print();
		}
		return NULL;
	}

	std::string WholeMessage = "pyLog - ";

	// Work out which instance of our PyWrapper is talking to us.
	PythonWrapper* thisPyWrapper = PythonWrapper::GetThisFromPythonSelf(guid);

	LOGDEBUG("Lookup pyObject: {:#x}, {:#x}", guid, (uint64_t)thisPyWrapper);

	if (thisPyWrapper != nullptr)
	{
		WholeMessage += thisPyWrapper->Name + " - ";
	}
	else
	{
		LOGDEBUG("odc.Log called from Python code for unknown PyPort object");
	}

	WholeMessage += message;

	// Take appropriate action
	if (auto log = odc::spdlog_get("PyPort"))
	{
		// Ordinals match spdlog values - spdlog::level::level_enum::
		switch (logtype)
		{
			case 0: log->trace(WholeMessage);
				break;
			case 1:
				log->debug(WholeMessage);
				break;
			case 2:
				log->info(WholeMessage);
				break;
			case 3:
				log->warn(WholeMessage);
				break;
			case 4:
				log->error(WholeMessage);
				break;
			default:
				log->critical(WholeMessage);
		}
	}

	// Return a PyObject return value, in this case "none".
	return Py_BuildValue("");
}

// This is an extension method that we have provided to our embedded Python. It will post an event into the ODC bus.
// It is static, so we have to work out which instance of the PythonWrapper class should handle it.
static PyObject* odc_PublishEvent(PyObject* self, PyObject* args)
{
	//TODO Update to new general event type, so we pass any event. Then decode in Python
	uint32_t index;
	uint32_t value;
	uint32_t quality;
	uint64_t guid;
	PyObject* pyTime = nullptr;

	// Now parse the arguments provided, three Unsigned ints (I) and a pyObject (O) and the function name.
	if (!PyArg_ParseTuple(args, "LIIIO:PublishEvent", &guid, &index, &value, &quality, &pyTime))
	{
		if (PyErr_Occurred())
		{
			PyErr_Print();
		}
		return NULL;
	}
	LOGDEBUG("Python Publish Event Index: {}, Value {}, Quality {}", index, value, quality);
	// Take appropriate action

	//     Timestamp time = PyDateTime_to_Timestamp(pyTime);

	//    Binary newmeas(value, quality, time);

	// Update to new format     thisPyPort->PublishEvent(newmeas, index);

	// Return a PyObject return value.
	return Py_BuildValue("i", 0);
}

static PyMethodDef odcMethods[] = {
	{"log", odc_log, METH_VARARGS, "Process a log message, level and string message"},
	{"PublishEvent", odc_PublishEvent, METH_VARARGS, "Publish ODC event to subscribed ports"},
	{NULL, NULL, 0, NULL}
};

static PyModuleDef odcModule = {
	PyModuleDef_HEAD_INIT, "odc", NULL, -1, odcMethods,   NULL, NULL, NULL, NULL
};

static PyObject* PyInit_odc(void)
{
	return PyModule_Create(&odcModule);
}

// Load the module into the python interpreter before we initialise it.
void ImportODCModule()
{
	if (PyImport_AppendInittab("odc", &PyInit_odc) != 0)
	{
		LOGERROR("Unable to import odc module to Python Interpreter");
	}
}

#pragma endregion

#pragma region Startup/Setup Functions

PythonWrapper::PythonWrapper(const std::string& aName):
	Name(aName)
{
	if (InterpreterUseCount == 0)
	{
		InterpreterUseCount++;
		InitialisePyInterpreter(); // We only need one for a running ODC instance...
	}
}

PythonWrapper::~PythonWrapper()
{
	LOGDEBUG("Destructing PythonWrapper");

	Py_XDECREF(pyFuncEvent);
	Py_XDECREF(pyFuncEnable);
	Py_XDECREF(pyFuncDisable);

	RemoveWrapperMapping();
	Py_XDECREF(pyInstance);

	if (pyModule != nullptr)
	{
		Py_DECREF(pyModule);
	}
	// Only shut down the interpreter when there are no more users left.
	InterpreterUseCount--;
	if (InterpreterUseCount == 0)
	{
		LOGDEBUG("Py_Finalize");
		Py_Finalize();
	}
}
// Startup the interpreter - need to have matching tear down in destructor.
void PythonWrapper::InitialisePyInterpreter()
{
	//PyEval_InitThreads();	// Setup interpreter to support threading (could choose not to do this if we use strand protection??)
	// This grabs the GIL, so we should release it when we dont need it. We should also get our PyThreadState and maintain it
	// This is for an old python version - need to check if this still applies.

	// Seems we really want to use a strand, this would then mean we dont need the GIL, so would be faster. Just need to make sure any python code we
	// run does not do any threading, which should be ok. If we use the c interface to program scheduled tasks, they will be strand protected
	// and we can do all we want, but rely on ASIO to coordinate it.

	LOGDEBUG("Py_Initialize");

	// Load our odc module exposing our internal methods to python (i.e. loggin commands)
	ImportODCModule();

	Py_Initialize(); // Get the Python interpreter running

	// Now execute some commands to get the environment ready.
	if (PyRun_SimpleString("import sys") != 0)
	{
		LOGERROR("Unable to import python sys library");
		return;
	}
	// Append current working directory to the sys.path variable - so we ca find the module.
	if (PyRun_SimpleString("sys.path.append(\".\")") != 0)
	{
		LOGERROR("Unable to append to sys path in python sys library");
		return;
	}
	PyDateTime_IMPORT;
}


void PythonWrapper::ImportModuleAndCreateClassInstance(const std::string& pyModuleName, const std::string& pyClassName, const std::string& PortName)
{
	const auto pyUniCodeModuleName = PyUnicode_FromString(pyModuleName.c_str());

	pyModule = PyImport_Import(pyUniCodeModuleName);
	if (pyModule == nullptr)
	{
		LOGERROR("Could not load Python Module - {}", pyModuleName);
		PyErrOutput();
		throw std::runtime_error::runtime_error("Could not load Python Module");
	}
	Py_DECREF(pyUniCodeModuleName);

	PyObject* pyDict = PyModule_GetDict(pyModule);
	if (pyDict == nullptr)
	{
		LOGERROR("Could not load Python Dictionary Reference");
		PyErrOutput();
		throw std::runtime_error::runtime_error("Could not load Python Dictionary Reference");
	}

	// Build the name of a callable class
	PyObject* pyClass = PyDict_GetItemString(pyDict, pyClassName.c_str());

	// Py_XDECREF(pyDict);	// Borrowed reference, dont destruct

	if (pyClass == nullptr)
	{
		LOGERROR("Could not load Python Class Reference - {}", pyClassName);
		PyErrOutput();
		throw std::runtime_error::runtime_error("Could not load Python Class");
	}
	// Create an instance of the class, with a name matching the port name
	if (pyClass && PyCallable_Check(pyClass))
	{
		auto pyArgs = PyTuple_New(2);
		auto pyguid = PyLong_FromUnsignedLongLong((uint64_t)this); // Pass a this pointer into our constructor, so we can idenify ourselves on calls back into C land
		auto pyObjectName = PyUnicode_FromString(PortName.c_str());
		PyTuple_SetItem(pyArgs, 0, pyguid);
		PyTuple_SetItem(pyArgs, 1, pyObjectName);

		pyInstance = PyObject_CallObject(pyClass, pyArgs);
		if (pyInstance == nullptr)
		{
			LOGDEBUG("Could not get instance pointer for Python Class");
			PyErrOutput();

			throw std::runtime_error::runtime_error("Could not get instance pointer for Python Class");
		}

		StoreWrapperMapping();

		Py_DECREF(pyArgs); // pyObjectName, pyguid is stolen into pyArgs, so dealt with in this call
	}
	else
	{
		PyErrOutput();
		LOGERROR("pyClass not callable");
		throw std::runtime_error::runtime_error("pyClass not callable");
	}
	// Py_XDECREF(pyClass);	// Borrowed reference, dont destruct

	pyFuncEnable = GetFunction(pyInstance, "Enable");
	pyFuncDisable = GetFunction(pyInstance, "Disable");
	pyFuncEvent = GetFunction(pyInstance, "EventHandler");
	//TODO: Call the config function in script ProcessJSONConfig(self, MainJSON, OverrideJSON)
}

void PythonWrapper::Build(const std::string& modulename, std::string& pyLoadModuleName, std::string& pyClassName, std::string& PortName)
{
	// Throws exceptions on fail
//	CreateBasePyModule(modulename);

	ImportModuleAndCreateClassInstance(pyLoadModuleName, pyClassName, PortName);
}

void PythonWrapper::Enable()
{
	auto pyArgs = PyTuple_New(0);
	PostPyCall(pyFuncEnable, pyArgs); // No passed variables
	Py_DECREF(pyArgs);
};

void PythonWrapper::Disable()
{
	auto pyArgs = PyTuple_New(0);
	PostPyCall(pyFuncDisable, pyArgs); // No passed variables
	Py_DECREF(pyArgs);
};
#pragma region


#pragma region WorkerFunctions

// Get a PyObject handle for the function name given, in the Python instance given.
PyObject* PythonWrapper::GetFunction(PyObject* pyInstance, const std::string& sFunction)
{
	PyObject* pyFunc = PyObject_GetAttrString(pyInstance, sFunction.c_str());

	if (!pyFunc || PyErr_Occurred() || !PyCallable_Check(pyFunc))
	{
		PyErrOutput();
		LOGERROR("Cannot resolve function \"{}\"\n", sFunction);
		return nullptr;
	}
	return pyFunc;
}

void PythonWrapper::PyErrOutput()
{
	//TODO Look at https://stackoverflow.com/questions/1796510/accessing-a-python-traceback-from-the-c-api

	PyObject* ptype, * pvalue, * ptraceback;
	PyErr_Fetch(&ptype, &pvalue, &ptraceback);
	//pvalue contains error message
	//ptraceback contains stack snapshot and many other information
	//(see python traceback structure)

	if (pvalue)
	{
		PyObject* pstr = PyObject_Str(pvalue);
		if (pstr)
		{
			const char* err_msg = PyUnicode_AsUTF8(pstr);
			Py_DECREF(pstr);

			if (err_msg)
			{
				std::cout << "Python Error " << err_msg << std::endl;
				LOGERROR("Python Error {}", err_msg);
			}
		}
		PyErr_Restore(ptype, pvalue, ptraceback); //TODO: Do we need to do this or DECREF the variables?
	}
}
// Post a call to a python method, that will be executed by ASIO.
//TODO: Think that we may need to strand protect this to prevent problems. There is also the Python GIL (global interpreter lock) to consider..
void PythonWrapper::PostPyCall(PyObject* pyFunction, PyObject* pyArgs)
{
	//	pIOS->post([&, pyArgs, pyFunction]
	{
		if (pyFunction && PyCallable_Check(pyFunction))
		{
			PyObject_CallObject(pyFunction, pyArgs);
			PyErrOutput();
		}
		else
		{
			LOGERROR("Python Method is not valid");
		}
	} //);
}
msSinceEpoch_t PyDateTime_to_msSinceEpoch_t(PyObject* pyTime)
{
	if (!pyTime || !PyDateTime_Check(pyTime))
		throw std::exception();

	tm timeinfo;
	timeinfo.tm_sec = PyDateTime_DATE_GET_SECOND(pyTime);
	timeinfo.tm_min = PyDateTime_DATE_GET_MINUTE(pyTime);
	timeinfo.tm_hour = PyDateTime_DATE_GET_HOUR(pyTime);
	timeinfo.tm_mday = PyDateTime_GET_DAY(pyTime);
	timeinfo.tm_mon = PyDateTime_GET_MONTH(pyTime) - 1;
	timeinfo.tm_year = PyDateTime_GET_YEAR(pyTime) - 1900;

	return msSinceEpoch_t(0); //TODO DateTime convert (std::chrono::seconds(my_mktime(&timeinfo)) + std::chrono::microseconds(PyDateTime_DATE_GET_MICROSECOND(pyTime)));
}

#pragma endregion