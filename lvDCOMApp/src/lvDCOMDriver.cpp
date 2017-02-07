/*************************************************************************\ 
* Copyright (c) 2013 Science and Technology Facilities Council (STFC), GB. 
* All rights reverved. 
* This file is distributed subject to a Software License Agreement found 
* in the file LICENSE.txt that is included with this distribution. 
\*************************************************************************/ 

/// @file lvDCOMDriver.cpp Implementation of #lvDCOMDriver class and lvDCOMConfigure() iocsh command
/// @author Freddie Akeroyd, STFC ISIS Facility, GB

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <exception>
#include <iostream>
#include <map>

#include <epicsTypes.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsString.h>
#include <epicsTimer.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <errlog.h>
#include <iocsh.h>

#include "lvDCOMDriver.h"
#include <epicsExport.h>

#include "lvDCOMInterface.h"
#include "convertToString.h"
#include "variant_utils.h"

static const char *driverName="lvDCOMDriver"; ///< Name of driver for use in message printing 

/// Function to translate a Win32 structured exception into a standard C++ exception. 
/// This is registered via registerStructuredExceptionHandler()
static void seTransFunction(unsigned int u, EXCEPTION_POINTERS* pExp)
{
	throw Win32StructuredException(u, pExp);
}

/// Register a handler for Win32 structured exceptions. This needs to be done on a per thread basis.
static void registerStructuredExceptionHandler()
{
	_set_se_translator(seTransFunction);
}

template<typename T>
asynStatus lvDCOMDriver::writeValue(asynUser *pasynUser, const char* functionName, T value)
{
	int function = pasynUser->reason;
	asynStatus status = asynSuccess;
	const char *paramName = NULL;
	registerStructuredExceptionHandler();
	getParamName(function, &paramName);
	try
	{
		if (m_lvdcom == NULL)
		{
			throw std::runtime_error("m_lvdcom is NULL");
		}
		m_lvdcom->setLabviewValue(paramName, value);
		asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
			"%s:%s: function=%d, name=%s, value=%s\n", 
			driverName, functionName, function, paramName, convertToString(value).c_str());
		return asynSuccess;
	}
	catch(const std::exception& ex)
	{
		epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
			"%s:%s: status=%d, function=%d, name=%s, value=%s, error=%s", 
			driverName, functionName, status, function, paramName, convertToString(value).c_str(), ex.what());
		return asynError;
	}
}

template<typename T>
asynStatus lvDCOMDriver::readValue(asynUser *pasynUser, const char* functionName, T* value)
{
	int function = pasynUser->reason;
	asynStatus status = asynSuccess;
	const char *paramName = NULL;
	registerStructuredExceptionHandler();
	getParamName(function, &paramName);
	try
	{
		if (m_lvdcom == NULL)
		{
			throw std::runtime_error("m_lvdcom is NULL");
		}
		m_lvdcom->getLabviewValue(paramName, value);
		asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
			"%s:%s: function=%d, name=%s, value=%s\n", 
			driverName, functionName, function, paramName, convertToString(*value).c_str());
		return asynSuccess;
	}
	catch(const std::exception& ex)
	{
		epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
			"%s:%s: status=%d, function=%d, name=%s, value=%s, error=%s", 
			driverName, functionName, status, function, paramName, convertToString(*value).c_str(), ex.what());
		return asynError;
	}
}

template<typename T>
asynStatus lvDCOMDriver::readArray(asynUser *pasynUser, const char* functionName, T *value, size_t nElements, size_t *nIn)
{
	int function = pasynUser->reason;
	asynStatus status = asynSuccess;
	const char *paramName = NULL;
	registerStructuredExceptionHandler();
	getParamName(function, &paramName);

	try
	{
		if (m_lvdcom == NULL)
		{
			throw std::runtime_error("m_lvdcom is NULL");
		}
		m_lvdcom->getLabviewValue(paramName, value, nElements, *nIn);
		asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
			"%s:%s: function=%d, name=%s\n", 
			driverName, functionName, function, paramName);
		return asynSuccess;
	}
	catch(const std::exception& ex)
	{
		*nIn = 0;
		epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
			"%s:%s: status=%d, function=%d, name=%s, error=%s", 
			driverName, functionName, status, function, paramName, ex.what());
		return asynError;
	}
}

asynStatus lvDCOMDriver::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
	return writeValue(pasynUser, "writeFloat64", value);
}

asynStatus lvDCOMDriver::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
	return writeValue(pasynUser, "writeInt32", value);
}

asynStatus lvDCOMDriver::readFloat64Array(asynUser *pasynUser, epicsFloat64 *value, size_t nElements, size_t *nIn)
{
	return readArray(pasynUser, "readFloat64Array", value, nElements, nIn);
}

asynStatus lvDCOMDriver::readInt32Array(asynUser *pasynUser, epicsInt32 *value, size_t nElements, size_t *nIn)
{
	return readArray(pasynUser, "readInt32Array", value, nElements, nIn);
}

asynStatus lvDCOMDriver::readFloat64(asynUser *pasynUser, epicsFloat64 *value)
{
	return readValue(pasynUser, "readFloat64", value);
}

asynStatus lvDCOMDriver::readInt32(asynUser *pasynUser, epicsInt32 *value)
{
	return readValue(pasynUser, "readInt32", value);
}

asynStatus lvDCOMDriver::readOctet(asynUser *pasynUser, char *value, size_t maxChars, size_t *nActual, int *eomReason)
{
	int function = pasynUser->reason;
	int status=0;
	const char *functionName = "readOctet";
	const char *paramName = NULL;
	registerStructuredExceptionHandler();
	getParamName(function, &paramName);
	std::string value_s;
	try
	{
		if (m_lvdcom == NULL)
		{
			throw std::runtime_error("m_lvdcom is NULL");
		}
		m_lvdcom->getLabviewValue(paramName, &value_s);
		if ( value_s.size() > maxChars ) // did we read more than we have space for?
		{
			*nActual = maxChars;
			if (eomReason) { *eomReason = ASYN_EOM_CNT | ASYN_EOM_END; }
			asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
				"%s:%s: function=%d, name=%s, value=\"%s\" (TRUNCATED from %d chars)\n", 
				driverName, functionName, function, paramName, value_s.substr(0,*nActual).c_str(), value_s.size());
		}
		else
		{
			*nActual = value_s.size();
			if (eomReason) { *eomReason = ASYN_EOM_END; }
			asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
				"%s:%s: function=%d, name=%s, value=\"%s\"\n", 
				driverName, functionName, function, paramName, value_s.c_str());
		}
		strncpy(value, value_s.c_str(), maxChars); // maxChars  will NULL pad if possible, change to  *nActual  if we do not want this
		return asynSuccess;
	}
	catch(const std::exception& ex)
	{
		epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
			"%s:%s: status=%d, function=%d, name=%s, value=\"%s\", error=%s", 
			driverName, functionName, status, function, paramName, value_s.c_str(), ex.what());
		*nActual = 0;
		if (eomReason) { *eomReason = ASYN_EOM_END; }
		value[0] = '\0';
		return asynError;
	}
}

asynStatus lvDCOMDriver::writeOctet(asynUser *pasynUser, const char *value, size_t maxChars, size_t *nActual)
{
	int function = pasynUser->reason;
	asynStatus status = asynSuccess;
	const char *paramName = NULL;
	registerStructuredExceptionHandler();
	getParamName(function, &paramName);
	const char* functionName = "writeOctet";
	std::string value_s(value, maxChars);
	try
	{
		if (m_lvdcom == NULL)
		{
			throw std::runtime_error("m_lvdcom is NULL");
		}
		m_lvdcom->setLabviewValue(paramName, value_s);
		asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
			"%s:%s: function=%d, name=%s, value=%s\n", 
			driverName, functionName, function, paramName, value_s.c_str());
		*nActual = value_s.size();
		return asynSuccess;
	}
	catch(const std::exception& ex)
	{
		epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
			"%s:%s: status=%d, function=%d, name=%s, value=%s, error=%s", 
			driverName, functionName, status, function, paramName, value_s.c_str(), ex.what());
		*nActual = 0;
		return asynError;
	}
}

/// EPICS driver report function for iocsh dbior command
void lvDCOMDriver::report(FILE* fp, int details)
{
//	fprintf(fp, "lvDCOM report\n");
	for(std::map<std::string,std::string>::const_iterator it=m_params.begin(); it != m_params.end(); ++it)
	{
		fprintf(fp, "Asyn param \"%s\" lvdcom type \"%s\"\n", it->first.c_str(), it->second.c_str());
	}
	if (m_lvdcom != NULL)
	{
		m_lvdcom->report(fp, details);
	}
	else
	{
		fprintf(fp, "DCOM pointer is NULL\n");
	}
	asynPortDriver::report(fp, details);
}


/// Constructor for the lvDCOMDriver class.
/// Calls constructor for the asynPortDriver base class and sets up driver parameters.
///
/// \param[in] dcomint DCOM interface pointer created by lvDCOMConfigure()
/// \param[in] portName @copydoc initArg0
lvDCOMDriver::lvDCOMDriver(lvDCOMInterface* dcomint, const char *portName) 
	: asynPortDriver(portName, 
	0, /* maxAddr */ 
	dcomint->nParams(),
	asynInt32Mask | asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynOctetMask | asynDrvUserMask, /* Interface mask */
	asynInt32Mask | asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynOctetMask,  /* Interrupt mask */
	ASYN_CANBLOCK, /* asynFlags.  This driver can block but it is not multi-device */
	1, /* Autoconnect */
	0, /* Default priority */
	0),	/* Default stack size*/
	m_lvdcom(dcomint)
{
	int i;
	const char *functionName = "lvDCOMDriver";
	m_lvdcom->getParams(m_params);
	for(std::map<std::string,std::string>::const_iterator it=m_params.begin(); it != m_params.end(); ++it)
	{
		if (it->second == "float64")
		{
			createParam(it->first.c_str(), asynParamFloat64, &i);
		}
		else if (it->second == "int32" || it->second == "enum" || it->second == "ring" || it->second == "boolean")
		{
			createParam(it->first.c_str(), asynParamInt32, &i);
		}
		else if (it->second == "string")
		{
			createParam(it->first.c_str(), asynParamOctet, &i);
		}
		else if (it->second == "float64array")
		{
			createParam(it->first.c_str(), asynParamFloat64Array, &i);
		}
		else if (it->second == "int32array")
		{
			createParam(it->first.c_str(), asynParamInt32Array, &i);
		}
		else
		{
			errlogSevPrintf(errlogMajor, "%s:%s: unknown type %s for parameter %s\n", driverName, functionName, it->second.c_str(), it->first.c_str());
//			std::cerr << driverName << ":" << functionName << ": unknown type " << it->second << " for parameter " << it->first << std::endl;
		}
	}

	// Create the thread for background tasks (not used at present, could be used for I/O intr scanning) 
	if (epicsThreadCreate("lvDCOMDriverTask",
		epicsThreadPriorityMedium,
		epicsThreadGetStackSize(epicsThreadStackMedium),
		(EPICSTHREADFUNC)lvDCOMTask, this) == 0)
	{
		printf("%s:%s: epicsThreadCreate failure\n", driverName, functionName);
		return;
	}
}

/// @todo Might use this for background polling if implementing I/O Intr scanning
void lvDCOMDriver::lvDCOMTask(void* arg) 
{ 
	lvDCOMDriver* driver = (lvDCOMDriver*)arg; 	
	registerStructuredExceptionHandler();
}


extern "C" {

	/// EPICS iocsh callable function to call constructor of lvDCOMInterface().
	/// The function is registered via lvDCOMRegister().
	///
	/// @param[in] portName @copydoc initArg0
	/// @param[in] configSection @copydoc initArg1
	/// @param[in] configFile @copydoc initArg2
	/// @param[in] host @copydoc initArg3
	/// @param[in] options @copydoc initArg4
	/// @param[in] progid @copydoc initArg5
	/// @param[in] username @copydoc initArg6
	/// @param[in] password @copydoc initArg7
	int lvDCOMConfigure(const char *portName, const char* configSection, const char *configFile, const char *host, int options, 
		const char* progid, const char* username, const char* password)
	{
		registerStructuredExceptionHandler();
		try
		{
			lvDCOMInterface* dcomint = new lvDCOMInterface(configSection, configFile, host, options, progid, username, password);
			if (dcomint != NULL)
			{
				new lvDCOMDriver(dcomint, portName);
				return(asynSuccess);
			}
			else
			{
				errlogSevPrintf(errlogFatal, "lvDCOMConfigure failed (NULL)\n");
				return(asynError);
			}

		}
		catch(const std::exception& ex)
		{
			errlogSevPrintf(errlogFatal, "lvDCOMConfigure failed: %s\n", ex.what());
			return(asynError);
		}
	}

	/// @param[in] portName @copydoc initArg0
	/// @param[in] host @copydoc initArg1
	/// @param[in] options @copydoc initArg2
	/// @param[in] progid @copydoc initArg3
	/// @param[in] username @copydoc initArg4
	/// @param[in] password @copydoc initArg5
	int lvDCOMSECIConfigure(const char *portName, const char* macros, const char* configSection, const char *configFile,
	      const char* dbSubFile,const char *host, int options, const char* progid, const char* username, const char* password)
	{
		registerStructuredExceptionHandler();
		try
		{
			// need to specify both of these so we also get restarted when labview disappears (SECI restart / config change)
			options |= static_cast<int>(lvDCOMOptions::lvNoStart);
			options |= static_cast<int>(lvDCOMOptions::lvSECIConfig);
			lvDCOMInterface* dcomint = new lvDCOMInterface("", "", host, 0x0, progid, username, password);
			if (dcomint != NULL)
			{
			    dcomint->generateFilesFromSECI(portName, macros, configSection, configFile, dbSubFile);
			    return lvDCOMConfigure(portName, configSection, configFile, host, options, progid, username, password);
			}
			else
			{
				errlogSevPrintf(errlogFatal, "lvDCOMSECIConfigure failed (NULL)\n");
				return(asynError);
			}
		}
		catch(const std::exception& ex)
		{
			errlogSevPrintf(errlogFatal, "lvDCOMSECIConfigure failed: %s\n", ex.what());
			return(asynError);
		}
	}
	// EPICS iocsh shell commands 

	static const iocshArg initArg0 = { "portName", iocshArgString};			///< A name for the asyn driver instance we will create - used to refer to it from EPICS DB files
	static const iocshArg initArg1 = { "configSection", iocshArgString};	///< section name of \a configFile we will load settings from
	static const iocshArg initArg2 = { "configFile", iocshArgString};		///< Path to the XML input file to load configuration information from
	static const iocshArg initArg3 = { "host", iocshArgString};				///< host name where LabVIEW is running ("" for localhost) 
	static const iocshArg initArg4 = { "options", iocshArgInt};			    ///< options as per #lvDCOMOptions enum
	static const iocshArg initArg5 = { "progid", iocshArgString};			///< (optional) DCOM ProgID (required if connecting to a compiled LabVIEW application)
	static const iocshArg initArg6 = { "username", iocshArgString};			///< (optional) remote username for \a host
	static const iocshArg initArg7 = { "password", iocshArgString};			///< (optional) remote password for \a username on \a host

	static const iocshArg initArgSECI0 = { "portName", iocshArgString};			///< A name for the asyn driver instance we will create - used to refer to it from EPICS DB files
	static const iocshArg initArgSECI1 = { "macros", iocshArgString};	///< section name of \a configFile we will load 
	static const iocshArg initArgSECI2 = { "configSection", iocshArgString};	///< section name of \a configFile settings from
	static const iocshArg initArgSECI3 = { "configFile", iocshArgString};		///< Path to the XML input file to load configuration information from
	static const iocshArg initArgSECI4 = { "dbSubFile", iocshArgString};		///< Path to the XML input file to load configuration information from
	static const iocshArg initArgSECI5 = { "host", iocshArgString};				///< host name where LabVIEW is running ("" for localhost) 
	static const iocshArg initArgSECI6 = { "options", iocshArgInt};			    ///< options as per #lvDCOMOptions enum
	static const iocshArg initArgSECI7 = { "progid", iocshArgString};			///< (optional) DCOM ProgID (required if connecting to a compiled LabVIEW application)
	static const iocshArg initArgSECI8 = { "username", iocshArgString};			///< (optional) remote username for \a host
	static const iocshArg initArgSECI9 = { "password", iocshArgString};			///< (optional) remote password for \a username on \a host

	static const iocshArg * const initArgs[] = { &initArg0,
		&initArg1,
		&initArg2,
		&initArg3,
		&initArg4,
		&initArg5,
		&initArg6,
		&initArg7 };

	static const iocshArg * const initArgsSECI[] = { &initArgSECI0,
		&initArgSECI1,
		&initArgSECI2,
		&initArgSECI3,
		&initArgSECI4,
		&initArgSECI5,
		&initArgSECI6,
		&initArgSECI7,
		&initArgSECI8,
		&initArgSECI9 };

	static const iocshFuncDef initFuncDef = { "lvDCOMConfigure", sizeof(initArgs) / sizeof(iocshArg*), initArgs};
	static const iocshFuncDef initFuncDefSECI = { "lvDCOMSECIConfigure", sizeof(initArgsSECI) / sizeof(iocshArg*), initArgsSECI};

	static void initCallFunc(const iocshArgBuf *args)
	{
		lvDCOMConfigure(args[0].sval, args[1].sval, args[2].sval, args[3].sval, args[4].ival, args[5].sval, args[6].sval, args[7].sval);
	}
	
	static void initCallFuncSECI(const iocshArgBuf *args)
	{
		lvDCOMSECIConfigure(args[0].sval, args[1].sval, args[2].sval, args[3].sval, args[4].sval, args[5].sval, args[6].ival, args[7].sval, args[8].sval, args[9].sval);
	}

	/// Register new commands with EPICS IOC shell
	static void lvDCOMRegister(void)
	{
		iocshRegister(&initFuncDef, initCallFunc);
		iocshRegister(&initFuncDefSECI, initCallFuncSECI);
	}

	epicsExportRegistrar(lvDCOMRegister);

}

