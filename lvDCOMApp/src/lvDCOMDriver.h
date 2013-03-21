#ifndef LVDCOMDRIVER_H
#define LVDCOMDRIVER_H
 
#include "asynPortDriver.h"

//#include <map>

class lvDCOMInterface;

/** Class that demonstrates the use of the asynPortDriver base class to greatly simplify the task
  * of writing an asyn port driver.
  * This class does a simple simulation of a digital oscilloscope.  It computes a waveform, computes
  * statistics on the waveform, and does callbacks with the statistics and the waveform data itself. 
  * I have made the methods of this class public in order to generate doxygen documentation for them,
  * but they should really all be private. */
class lvDCOMDriver : public asynPortDriver {
public:
    lvDCOMDriver(lvDCOMInterface* stuff, const char *portName);
                 
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
	template<typename T> asynStatus writeValue(asynUser *pasynUser, const char* functionName, T value);
    template<typename T> asynStatus readValue(asynUser *pasynUser, const char* functionName, T* value);
    template<typename T> asynStatus readArray(asynUser *pasynUser, const char* functionName, T *value, size_t nElements, size_t *nIn);

	static void lvDCOMTask(void* arg) { lvDCOMDriver* driver = (lvDCOMDriver*)arg; }
	
private:
    /* Our data */
//    epicsEventId eventId;
//    epicsFloat64 *pData;
//    epicsFloat64 *pTimeBase;
	lvDCOMInterface* m_lvdcom;
};

#endif /* LVDCOMDRIVER_H */
