#!../../bin/windows-x64/lvdcom

< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase "dbd/lvDCOM.dbd"
lvDCOM_registerRecordDeviceDriver pdbbase

cd ${TOP}/iocBoot/${IOC}

# Turn on asynTraceFlow and asynTraceError for global trace, i.e. no connected asynUser.
#asynSetTraceMask("", 0, 17)

## main args are:  portName, configSection, configFile, host, options (see lvDCOMConfigure() documentation in lvDCOMDriver.cpp)
##
## there are additional optional args to specify a DCOM ProgID for a compiled LabVIEW application 
## and a different username + password for remote host if that is required 
##
## the "options" argument is a combination of the following flags (as per the #lvDCOMOptions enum in lvDCOMInterface.h)
##    viWarnIfIdle=1, viStartIfIdle=2, viStopOnExitIfStarted=4, viAlwaysStopOnExit=8
lvDCOMConfigure("ex1", "example", "$(TOP)/lvDCOMApp/src/examples/example_lvinput.xml", "", 6)
#lvDCOMConfigure("ex1", "example", "$(TOP)/lvDCOMApp/src/examples/example_lvinput.xml", "", 6, "LvDCOMex.Application")
#lvDCOMConfigure("ex1", "example", "$(TOP)/lvDCOMApp/src/examples/example_lvinput.xml", "ndxtestfaa", 6, "", "username", "password")

dbLoadRecords("$(TOP)/db/lvDCOM.db","P=ex1:")
#dbLoadRecords("$(ASYN)/db/asynRecord.db","P=ex1:,R=asyn1,PORT=ex1,ADDR=0,OMAX=80,IMAX=80")
#asynSetTraceMask("ex1",0,0xff)
asynSetTraceIOMask("ex1",0,0x2)

iocInit
