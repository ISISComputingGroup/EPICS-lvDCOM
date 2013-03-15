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

lvDCOMConfigure("beamlogger", "../../lvDCOMApp/src/lvinput.xml")
lvDCOMConfigure("remotebl", "../../lvDCOMApp/src/lvinput2.xml", "ndxtestfaa")

dbLoadRecords("../../db/lvDCOM.db","P=beamlogger:,R=scope1:,PORT=beamlogger,ADDR=0,TIMEOUT=1,NPOINTS=1000")
dbLoadRecords("$(ASYN)/db/asynRecord.db","P=beamlogger:,R=asyn1,PORT=beamlogger,ADDR=0,OMAX=80,IMAX=80")
#asynSetTraceMask("beamlogger",0,0xff)
asynSetTraceIOMask("beamlogger",0,0x2)

iocInit
