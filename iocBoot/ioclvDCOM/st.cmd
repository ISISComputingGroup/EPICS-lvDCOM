#!../../bin/windows-x64/lvdcom

## You may have to change lvdcom to something else
## everywhere it appears in this file

< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase "dbd/lvDCOM.dbd"
lvDCOM_registerRecordDeviceDriver pdbbase

cd ${TOP}/iocBoot/${IOC}

# Turn on asynTraceFlow and asynTraceError for global trace, i.e. no connected asynUser.
#asynSetTraceMask("", 0, 17)

## args are:  portName, configSection, configFile, host, warnViIdle, autoStartVi
lvDCOMConfigure("ex1", "example", "$(TOP)/lvDCOMApp/src/examples/example_lvinput.xml", "", 1, 1)
#lvDCOMConfigure("ex2", "example", "$(TOP)/lvDCOMApp/src/examples/example_lvinput.xml", "ndxtestfaa", 1, 1)

dbLoadRecords("../../db/lvDCOM.db","P=ex1:,R=scope1:,PORT=ex1,ADDR=0,TIMEOUT=1,NPOINTS=1000")
dbLoadRecords("$(ASYN)/db/asynRecord.db","P=ex1:,R=asyn1,PORT=ex1,ADDR=0,OMAX=80,IMAX=80")
#asynSetTraceMask("ex1",0,0xff)
asynSetTraceIOMask("ex1",0,0x2)

iocInit
