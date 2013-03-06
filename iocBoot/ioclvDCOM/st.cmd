#!../../bin/windows-x64/lvexport

## You may have to change lvexport to something else
## everywhere it appears in this file

< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase "dbd/lvexport.dbd"
lvexport_registerRecordDeviceDriver pdbbase

cd ${TOP}/iocBoot/${IOC}

# Turn on asynTraceFlow and asynTraceError for global trace, i.e. no connected asynUser.
#asynSetTraceMask("", 0, 17)

testAsynPortDriverConfigure("beamlogger", "C:/development/EPICS/ISIS/lvexportApp/src/lvinput.xml")
testAsynPortDriverConfigure("remotebl", "C:/development/EPICS/ISIS/lvexportApp/src/lvinput2.xml", "ndxtestfaa")

dbLoadRecords("../../db/lvexport.db","P=beamlogger:,R=scope1:,PORT=beamlogger,ADDR=0,TIMEOUT=1,NPOINTS=1000")
dbLoadRecords("$(ASYN)/db/asynRecord.db","P=beamlogger:,R=asyn1,PORT=beamlogger,ADDR=0,OMAX=80,IMAX=80")
#asynSetTraceMask("beamlogger",0,0xff)
asynSetTraceIOMask("beamlogger",0,0x2)

iocInit
