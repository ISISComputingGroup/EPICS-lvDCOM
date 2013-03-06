TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# build a support library

LIBRARY_IOC += TinyXPath

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += xxxRecord
# install TinyXPath.dbd into <top>/dbd
#DBD += TinyXPath.dbd

# specify all source files to be compiled and added to the library
TinyXPath_SRCS += tinystr.cpp tinyxml.cpp tinyxmlerror.cpp tinyxmlparser.cpp \
   action_store.cpp lex_util.cpp node_set.cpp tokenlist.cpp xml_util.cpp \
   xpath_expression.cpp xpath_processor.cpp xpath_stack.cpp xpath_stream.cpp xpath_syntax.cpp xpath_static.cpp

INC += xpath_processor.h action_store.h xpath_stack.h xpath_stream.h xpath_expression.h tinyxpath_conf.h \
   node_set.h lex_util.h byte_stream.h xpath_syntax.h tokenlist.h lex_token.h xpath_static.h

TinyXPath_LIBS += $(EPICS_BASE_IOC_LIBS)
#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

