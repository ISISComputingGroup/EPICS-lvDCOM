#ifndef LVDCOMDRIVER_H
#define LVDCOMDRIVER_H
 
#include "asynPortDriver.h"

class lvDCOMInterface;

class lvDCOMDriver : public asynPortDriver 
{
public:
    lvDCOMDriver(lvDCOMInterface* dcomint, const char *portName);
                 
    // These are the methods that we override from asynPortDriver
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
	virtual asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    virtual asynStatus readFloat64(asynUser *pasynUser, epicsFloat64 *value);
	virtual asynStatus readOctet(asynUser *pasynUser, char *value, size_t maxChars, size_t *nActual, int *eomReason);
	virtual asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t maxChars, size_t *nActual);
    virtual asynStatus readFloat64Array(asynUser *pasynUser, epicsFloat64 *value, size_t nElements, size_t *nIn);
    virtual asynStatus readInt32Array(asynUser *pasynUser, epicsInt32 *value, size_t nElements, size_t *nIn);

private:
	lvDCOMInterface* m_lvdcom;

	template<typename T> asynStatus writeValue(asynUser *pasynUser, const char* functionName, T value);
    template<typename T> asynStatus readValue(asynUser *pasynUser, const char* functionName, T* value);
    template<typename T> asynStatus readArray(asynUser *pasynUser, const char* functionName, T *value, size_t nElements, size_t *nIn);

	static void lvDCOMTask(void* arg) { lvDCOMDriver* driver = (lvDCOMDriver*)arg; }	
};

#endif /* LVDCOMDRIVER_H */
