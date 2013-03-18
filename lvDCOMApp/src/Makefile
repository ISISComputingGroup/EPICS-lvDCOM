TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

FILE_TYPE += pdb
INSTALL_pdb = $(INSTALL_BIN)

USR_INCLUDES += -I../tlb

USR_CXXFLAGS += /Zi
#USR_CPPFLAGS
USR_LDFLAGS += /DEBUG

#=============================
# Build the IOC support library

LIBRARY_IOC += lvDCOMSupport

pdb += lvDCOMSupport.pdb

DBD += lvDCOM.dbd

# Compile and add the code to the support library
lvDCOMSupport_SRCS += lvDCOMDriver.cpp isis_stuff.cpp variant_utils.cpp

lvDCOMSupport_LIBS += asyn # tinyxpath
lvDCOMSupport_LIBS += $(EPICS_BASE_IOC_LIBS)
lvDCOMSupport_SYS_LIBS_WIN32 += msxml2
#=============================
# Build the IOC application

PROD_IOC = lvDCOM

# lvexport_registerRecordDeviceDriver.cpp derives from lvexport.dbd
lvDCOM_SRCS_DEFAULT += lvDCOM_registerRecordDeviceDriver.cpp lvDCOMMain.cpp

# Link in the code from the support library
lvDCOM_LIBS += lvDCOMSupport asyn

# Finally link to the EPICS Base libraries
lvDCOM_LIBS += $(EPICS_BASE_IOC_LIBS)

pdb += lvDCOM.pdb generateConfig.pdb

PROD = generateConfig
generateConfig_SRCS += generateConfig.cpp
generateConfig_LIBS += lvDCOMSupport

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

