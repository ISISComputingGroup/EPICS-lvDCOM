TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = lvDCOM
# lvDCOM.dbd will be created and installed
DBD += lvDCOM.dbd

# lvDCOM.dbd will be made up from these files:
lvDCOM_DBD += base.dbd

# Include dbd files from all support applications:
#lvDCOM_DBD += xxx.dbd

# Add all the support libraries needed by this IOC
#lvDCOM_LIBS += xxx

# lvDCOM_registerRecordDeviceDriver.cpp derives from lvDCOM.dbd
lvDCOM_SRCS += lvDCOM_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
lvDCOM_SRCS_DEFAULT += lvDCOMMain.cpp
lvDCOM_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#lvDCOM_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
lvDCOM_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

