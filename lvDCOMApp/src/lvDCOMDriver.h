#ifndef LVDCOMDRIVER_H
#define LVDCOMDRIVER_H
 
#include "asynPortDriver.h"

/* These are the drvInfo strings that are used to identify the parameters.
 * They are used by asyn clients, including standard asyn device support */
#define P_LvRunString                	"LV_RUN" 
#define P_LvRun2String                	"LV_RUN2" 

class ISISSTUFF;

/** Class that demonstrates the use of the asynPortDriver base class to greatly simplify the task
  * of writing an asyn port driver.
  * This class does a simple simulation of a digital oscilloscope.  It computes a waveform, computes
  * statistics on the waveform, and does callbacks with the statistics and the waveform data itself. 
  * I have made the methods of this class public in order to generate doxygen documentation for them,
  * but they should really all be private. */
class lvDCOMDriver : public asynPortDriver {
public:
    lvDCOMDriver(const char *portName, const char *configFile, const char* host);
                 
    /* These are the methods that we override from asynPortDriver */
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
	virtual asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    virtual asynStatus readFloat64(asynUser *pasynUser, epicsFloat64 *value);
	virtual asynStatus readOctet(asynUser *pasynUser, char *value, size_t maxChars, size_t *nActual, int *eomReason);
	virtual asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t maxChars, size_t *nActual);
    virtual asynStatus readFloat64Array(asynUser *pasynUser, epicsFloat64 *value, size_t nElements, size_t *nIn);
    virtual asynStatus readInt32Array(asynUser *pasynUser, epicsInt32 *value, size_t nElements, size_t *nIn);

protected:
    /** Values used for pasynUser->reason, and indexes into the parameter library. */
    int P_LvRun;
    int P_LvRun2;
	
    #define FIRST_LV_COMMAND P_LvRun
    #define LAST_LV_COMMAND P_LvRun2
 
private:
    /* Our data */
//    epicsEventId eventId;
//    epicsFloat64 *pData;
//    epicsFloat64 *pTimeBase;
	ISISSTUFF* m_stuff;
};


#define NUM_LV_PARAMS (&LAST_LV_COMMAND - &FIRST_LV_COMMAND + 1)
#define MAX_NUM_LV_CONTROLS		100

#endif /* LVDCOMDRIVER_H */
