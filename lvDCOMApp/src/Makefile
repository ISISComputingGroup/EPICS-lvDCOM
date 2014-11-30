TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#FILE_TYPE += pdb
#INSTALL_pdb = $(INSTALL_BIN)

#USR_CXXFLAGS += /Zi
#USR_LDFLAGS += /DEBUG

USR_CXXFLAGS += /EHa

#=============================
# Build the IOC support library

LIBRARY_IOC += lvDCOM

#pdb += lvDCOM.pdb

DBD += lvDCOM.dbd

# Compile and add the code to the support library
lvDCOM_SRCS += lvDCOMDriver.cpp lvDCOMInterface.cpp variant_utils.cpp convertToString.cpp

lvDCOM_LIBS += asyn
lvDCOM_LIBS += $(EPICS_BASE_IOC_LIBS)

# msxml2 is used for parsing lvinput.xml
lvDCOM_SYS_LIBS_WIN32 += msxml2

SCRIPTS += fix_xml.cmd fix_xml.sh

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

