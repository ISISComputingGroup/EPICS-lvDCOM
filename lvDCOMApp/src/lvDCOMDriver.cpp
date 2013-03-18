#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <exception>
#include <iostream>

#include <epicsTypes.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsString.h>
#include <epicsTimer.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <iocsh.h>

#include "lvDCOMDriver.h"
#include <epicsExport.h>

#include "isis_stuff.h"

static const char *driverName="lvDCOMDriver";

/** Called when asyn clients call pasynFloat64->write().
  * This function sends a signal to the simTask thread if the value of P_UpdateTime has changed.
  * For all  parameters it  sets the value in the parameter library and calls any registered callbacks.
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus lvDCOMDriver::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int addr;
    const char *paramName = NULL;
	getParamName(function, &paramName);
    const char* functionName = "writeFloat64";
	try
	{
		this->getAddress(pasynUser, &addr);
		if (m_stuff == NULL)
		{
			throw std::runtime_error("m_stuff is NULL");
		}
		m_stuff->setLabviewValue(this->portName, paramName, value);
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, name=%s, value=%f\n", 
              driverName, functionName, function, paramName, value);
		return asynSuccess;
	}
	catch(const std::exception& ex)
	{
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, name=%s, value=%f, error=%s", 
                  driverName, functionName, status, function, paramName, value, ex.what());
		return asynError;
	}
}

asynStatus lvDCOMDriver::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int addr;
    const char *paramName = NULL;
	getParamName(function, &paramName);
    const char* functionName = "writeInt32";

	try
	{
		this->getAddress(pasynUser, &addr);
		if (m_stuff == NULL)
		{
			throw std::runtime_error("m_stuff is NULL");
		}
		m_stuff->setLabviewValue(this->portName, paramName, value);
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, name=%s, value=%d\n", 
              driverName, functionName, function, paramName, value);
		return asynSuccess;
	}
	catch(const std::exception& ex)
	{
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, name=%s, value=%d, error=%s", 
                  driverName, functionName, status, function, paramName, value, ex.what());
		return asynError;
	}
}

asynStatus lvDCOMDriver::readFloat64Array(asynUser *pasynUser, epicsFloat64 *value, size_t nElements, size_t *nIn)
{
  int function = pasynUser->reason;
  int addr;
  int status=0;
  const char *paramName = "";
  static const char *functionName = "readFloat64Array";

	try
	{
		this->getAddress(pasynUser, &addr);
		if (m_stuff == NULL)
		{
			throw std::runtime_error("m_stuff is NULL");
		}
		m_stuff->getLabviewValue(this->portName, paramName, value, nElements, *nIn);
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


asynStatus lvDCOMDriver::readInt32Array(asynUser *pasynUser, epicsInt32 *value, size_t nElements, size_t *nIn)
{
  int function = pasynUser->reason;
  int addr;
  int status=0;
  const char *paramName = NULL;
	getParamName(function, &paramName);
  static const char *functionName = "readInt32Array";

	try
	{
		this->getAddress(pasynUser, &addr);
		if (m_stuff == NULL)
		{
			throw std::runtime_error("m_stuff is NULL");
		}
		m_stuff->getLabviewValue(this->portName, paramName, value, nElements, *nIn);
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


asynStatus lvDCOMDriver::readFloat64(asynUser *pasynUser, epicsFloat64 *value)
{
	int addr;
	int function = pasynUser->reason;
	int status=0;
	const char *functionName = "readFloat64";
    const char *paramName = NULL;
	getParamName(function, &paramName);
	try
	{
		this->getAddress(pasynUser, &addr);
		if (m_stuff == NULL)
		{
			throw std::runtime_error("m_stuff is NULL");
		}
		m_stuff->getLabviewValue(this->portName, paramName, value);
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, name=%s, value=%f\n", 
              driverName, functionName, function, paramName, *value);
		return asynSuccess;
	}
	catch(const std::exception& ex)
	{
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, name=%s, value=%f, error=%s", 
                  driverName, functionName, status, function, paramName, *value, ex.what());
		return asynError;
	}
}

asynStatus lvDCOMDriver::readInt32(asynUser *pasynUser, epicsInt32 *value)
{
	int addr;
	int function = pasynUser->reason;
	int status=0;
	const char *functionName = "readInt32";
    const char *paramName = NULL;
	getParamName(function, &paramName);
	try
	{
		this->getAddress(pasynUser, &addr);
		if (m_stuff == NULL)
		{
			throw std::runtime_error("m_stuff is NULL");
		}
		m_stuff->getLabviewValue(this->portName, paramName, value);
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, name=%s, value=%d\n", 
              driverName, functionName, function, paramName, *value);
		return asynSuccess;
	}
	catch(const std::exception& ex)
	{
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, name=%s, value=%d, error=%s", 
                  driverName, functionName, status, function, paramName, *value, ex.what());
		return asynError;
	}
}

asynStatus lvDCOMDriver::readOctet(asynUser *pasynUser, char *value, size_t maxChars, size_t *nActual, int *eomReason)
{
	int addr;
	int function = pasynUser->reason;
	int status=0;
	const char *functionName = "readOctet";
    const char *paramName = NULL;
	getParamName(function, &paramName);
	std::string value_s;
	try
	{
		this->getAddress(pasynUser, &addr);
		if (m_stuff == NULL)
		{
			throw std::runtime_error("m_stuff is NULL");
		}
		m_stuff->getLabviewValue(this->portName, paramName, &value_s);
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
    int addr;
    const char *paramName = NULL;
	getParamName(function, &paramName);
    const char* functionName = "writeOctet";
	std::string value_s(value, maxChars);
	try
	{
		this->getAddress(pasynUser, &addr);
		if (m_stuff == NULL)
		{
			throw std::runtime_error("m_stuff is NULL");
		}
		m_stuff->setLabviewValue(this->portName, paramName, value_s);
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


/* Report

/** Constructor for the lvDCOMDriver class.
  * Calls constructor for the asynPortDriver base class.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] configFile The maximum  number of points in the volt and time arrays */
lvDCOMDriver::lvDCOMDriver(ISISSTUFF* stuff, const char *portName, const char *configFile, const char *host) 
   : asynPortDriver(portName, 
                    0, /* maxAddr */ 
                    stuff->nParams(),
                    asynInt32Mask | asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynOctetMask | asynDrvUserMask, /* Interface mask */
                    asynInt32Mask | asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynOctetMask,  /* Interrupt mask */
                    ASYN_CANBLOCK, /* asynFlags.  This driver can block but it is not multi-device */
                    1, /* Autoconnect */
                    0, /* Default priority */
                    0),	/* Default stack size*/
					m_stuff(stuff)
{
    asynStatus status;
    int i;
    const char *functionName = "lvDCOMDriver";
	std::map<std::string,std::string> res;
	m_stuff->getParams(res);
	std::map<std::string,std::string>::const_iterator it;
	for(it=res.begin(); it != res.end(); ++it)
	{
		if (it->second == "float64")
		{
            createParam(it->first.c_str(), asynParamFloat64, &i);
		}
		else if (it->second == "int32")
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
			std::cerr << "unknown type " << it->second << " for parameter " << it->first << std::endl;
		}
	}

    /* Create the thread that computes the waveforms in the background */
    if (epicsThreadCreate("lvDCOMDriverTask",
                          epicsThreadPriorityMedium,
                          epicsThreadGetStackSize(epicsThreadStackMedium),
                          (EPICSTHREADFUNC)task, this) == 0)
    {
        printf("%s:%s: epicsThreadCreate failure\n", driverName, functionName);
        return;
    }
}


/* Configuration routine.  Called directly, or from the iocsh function below */

extern "C" {

/** EPICS iocsh callable function to call constructor for the lvDCOMDriver class.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] maxPoints The maximum  number of points in the volt and time arrays */
int lvDCOMConfigure(const char *portName, const char *configFile, const char *host, int warnViIdle, int autostartVi)
{
	try
	{
		ISISSTUFF* stuff = new ISISSTUFF(portName, configFile, host, warnViIdle, autostartVi);
		new lvDCOMDriver(stuff, portName, configFile, host);
		return(asynSuccess);
	}
	catch(const std::exception& ex)
	{
		std::cerr << "isis_setup failed: " << ex.what() << std::endl;
	}
}

/* EPICS iocsh shell commands */

static const iocshArg initArg0 = { "portName", iocshArgString};
static const iocshArg initArg1 = { "configFile", iocshArgString};
static const iocshArg initArg2 = { "host", iocshArgString};
static const iocshArg initArg3 = { "warnViIdle", iocshArgInt};
static const iocshArg initArg4 = { "autostartVi", iocshArgInt};

static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
											&initArg4};
static const iocshFuncDef initFuncDef = {"lvDCOMConfigure",5,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    lvDCOMConfigure(args[0].sval, args[1].sval, args[2].sval, args[3].ival, args[4].ival);
}

void lvDCOMRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

epicsExportRegistrar(lvDCOMRegister);

}

