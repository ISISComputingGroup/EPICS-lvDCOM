/*
 * testAsynPortDriver.cpp
 * 
 * Asyn driver that inherits from the asynPortDriver class to demonstrate its use.
 * It simulates a digital scope looking at a 1kHz 1000-point noisy sine wave.  Controls are
 * provided for time/division, volts/division, volt offset, trigger delay, noise amplitude, update time,
 * and run/stop.
 * Readbacks are provides for the waveform data, min, max and mean values.
 *
 * Author: Mark Rivers
 *
 * Created Feb. 5, 2009
 */

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

#include "testAsynPortDriver.h"
#include <epicsExport.h>

#include "isis_stuff.h"

static const char *driverName="testAsynPortDriver";

/** Called when asyn clients call pasynFloat64->write().
  * This function sends a signal to the simTask thread if the value of P_UpdateTime has changed.
  * For all  parameters it  sets the value in the parameter library and calls any registered callbacks.
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus testAsynPortDriver::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int addr;
    const char *paramName = "";
    const char* functionName = "writeFloat64";
	try
	{
		this->getAddress(pasynUser, &addr);
		if (m_stuff == NULL)
		{
			throw std::runtime_error("m_stuff is NULL");
		}
		m_stuff->setLabviewValue(this->portName, addr, value);
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

asynStatus testAsynPortDriver::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int addr;
    const char *paramName = "";
    const char* functionName = "writeInt32";

	try
	{
		this->getAddress(pasynUser, &addr);
		if (m_stuff == NULL)
		{
			throw std::runtime_error("m_stuff is NULL");
		}
		m_stuff->setLabviewValue(this->portName, addr, value);
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

asynStatus testAsynPortDriver::readFloat64Array(asynUser *pasynUser, epicsFloat64 *value, size_t nElements, size_t *nIn)
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
		m_stuff->getLabviewValue(this->portName, addr, value, nElements, *nIn);
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


asynStatus testAsynPortDriver::readInt32Array(asynUser *pasynUser, epicsInt32 *value, size_t nElements, size_t *nIn)
{
  int function = pasynUser->reason;
  int addr;
  int status=0;
  const char *paramName = "";
  static const char *functionName = "readInt32Array";

	try
	{
		this->getAddress(pasynUser, &addr);
		if (m_stuff == NULL)
		{
			throw std::runtime_error("m_stuff is NULL");
		}
		m_stuff->getLabviewValue(this->portName, addr, value, nElements, *nIn);
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


asynStatus testAsynPortDriver::readFloat64(asynUser *pasynUser, epicsFloat64 *value)
{
	int addr;
	int function = pasynUser->reason;
	int status=0;
	const char *functionName = "readFloat64";
    const char *paramName = "";
	try
	{
		this->getAddress(pasynUser, &addr);
		if (m_stuff == NULL)
		{
			throw std::runtime_error("m_stuff is NULL");
		}
		m_stuff->getLabviewValue(this->portName, addr, value);
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

asynStatus testAsynPortDriver::readInt32(asynUser *pasynUser, epicsInt32 *value)
{
	int addr;
	int function = pasynUser->reason;
	int status=0;
	const char *functionName = "readInt32";
    const char *paramName = "";
	try
	{
		this->getAddress(pasynUser, &addr);
		if (m_stuff == NULL)
		{
			throw std::runtime_error("m_stuff is NULL");
		}
		m_stuff->getLabviewValue(this->portName, addr, value);
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

asynStatus testAsynPortDriver::readOctet(asynUser *pasynUser, char *value, size_t maxChars, size_t *nActual, int *eomReason)
{
	int addr;
	int function = pasynUser->reason;
	int status=0;
	const char *functionName = "readOctet";
    const char *paramName = "";
	std::string value_s;
	try
	{
		this->getAddress(pasynUser, &addr);
		if (m_stuff == NULL)
		{
			throw std::runtime_error("m_stuff is NULL");
		}
		m_stuff->getLabviewValue(this->portName, addr, &value_s);
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

asynStatus testAsynPortDriver::writeOctet(asynUser *pasynUser, const char *value, size_t maxChars, size_t *nActual)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int addr;
    const char *paramName = "";
    const char* functionName = "writeOctet";
	std::string value_s(value, maxChars);
	try
	{
		this->getAddress(pasynUser, &addr);
		if (m_stuff == NULL)
		{
			throw std::runtime_error("m_stuff is NULL");
		}
		m_stuff->setLabviewValue(this->portName, addr, value_s);
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

/** Constructor for the testAsynPortDriver class.
  * Calls constructor for the asynPortDriver base class.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] maxPoints The maximum  number of points in the volt and time arrays */
testAsynPortDriver::testAsynPortDriver(const char *portName, const char *configFile, const char *host) 
   : asynPortDriver(portName, 
                    MAX_NUM_LV_CONTROLS, /* maxAddr */ 
                    NUM_LV_PARAMS,
                    asynInt32Mask | asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynOctetMask | asynDrvUserMask, /* Interface mask */
                    asynInt32Mask | asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynOctetMask,  /* Interrupt mask */
                    ASYN_MULTIDEVICE | ASYN_CANBLOCK, /* asynFlags.  This driver does not block and it is not multi-device, so flag is 0 */
                    1, /* Autoconnect */
                    0, /* Default priority */
                    0)	/* Default stack size*/
{
    asynStatus status;
    int i;
    const char *functionName = "testAsynPortDriver";

    createParam(P_LvRunString,                asynParamInt32,         &P_LvRun);
    createParam(P_LvRun2String,                asynParamInt32,         &P_LvRun2);
	try
	{
		m_stuff = new ISISSTUFF(portName, configFile, host);
	}
	catch(const std::exception& ex)
	{
		std::cerr << "isis_setup failed: " << ex.what() << std::endl;
		m_stuff = NULL;
	}

    
    /* Set the initial values of some parameters */
 //   setIntegerParam(addr, P_LvRun,               0);

    /* Create the thread that computes the waveforms in the background */
 //   status = (asynStatus)(epicsThreadCreate("testAsynPortDriverTask",
 //                         epicsThreadPriorityMedium,
 //                         epicsThreadGetStackSize(epicsThreadStackMedium),
 //                         (EPICSTHREADFUNC)::simTask,
 //                         this) == NULL);
 //   if (status) {
 //       printf("%s:%s: epicsThreadCreate failure\n", driverName, functionName);
//        return;
//    }
}


/* Configuration routine.  Called directly, or from the iocsh function below */

extern "C" {

/** EPICS iocsh callable function to call constructor for the testAsynPortDriver class.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] maxPoints The maximum  number of points in the volt and time arrays */
int testAsynPortDriverConfigure(const char *portName, const char *configFile, const char *host)
{
		new testAsynPortDriver(portName, configFile, host);
		return(asynSuccess);
}


/* EPICS iocsh shell commands */

static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "configFile",iocshArgString};
static const iocshArg initArg2 = { "host",iocshArgString};

static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
											&initArg2};
static const iocshFuncDef initFuncDef = {"testAsynPortDriverConfigure",3,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    testAsynPortDriverConfigure(args[0].sval, args[1].sval, args[2].sval);
}

void testAsynPortDriverRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

epicsExportRegistrar(testAsynPortDriverRegister);

}

