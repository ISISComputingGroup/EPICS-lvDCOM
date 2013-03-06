TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# build a support library

LIBRARY_IOC += TinyXML

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += xxxRecord
# install TinyXML.dbd into <top>/dbd
#DBD += TinyXML.dbd

#USR_INCLUDES += -DTIXML_USE_STL
USR_CPPFLAGS += -DTIXML_USE_STL

# specify all source files to be compiled and added to the library
TinyXML_SRCS += tinyxml.cpp tinyxmlparser.cpp xmltest.cpp tinyxmlerror.cpp tinystr.cpp
INC += tinyxml.h tinystr.h

TinyXML_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

