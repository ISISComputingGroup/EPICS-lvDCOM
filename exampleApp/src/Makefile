TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = example
# example.dbd will be created and installed
DBD += example.dbd

# example.dbd will be made up from these files:
example_DBD += base.dbd

# Include dbd files from all support applications:
example_DBD += lvDCOM.dbd
# all of these are optional, just included for convenience
example_DBD += asSupport.dbd
example_DBD += devIocStats.dbd
example_DBD += caPutLog.dbd
example_DBD += asyn.dbd
example_DBD += drvAsynIPPort.dbd
example_DBD += drvAsynSerialPort.dbd
example_DBD += busySupport.dbd
example_DBD += calcSupport.dbd
example_DBD += sscanSupport.dbd
example_DBD += stdSupport.dbd

# Add all the support libraries needed by this IOC, lvDCOM itself just needs asyn
example_LIBS += lvDCOM calc sscan busy asyn std
example_LIBS += seqDev seq pv
example_LIBS += devIocStats
example_LIBS += caPutLog autosave
ifdef PCRE
example_LIBS += pcrecpp pcre
endif

# example_registerRecordDeviceDriver.cpp derives from example.dbd
example_SRCS += example_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
example_SRCS_DEFAULT += exampleMain.cpp
example_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#example_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
example_LIBS += $(EPICS_BASE_IOC_LIBS)

# msxml2 is used for parsing lvinput.xml
example_SYS_LIBS_WIN32 += msxml2

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
