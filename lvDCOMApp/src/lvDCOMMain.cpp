/*************************************************************************\ 
* Copyright (c) 2013 Science and Technology Facilities Council (STFC), GB. 
* All rights reverved. 
* This file is distributed subject to a Software License Agreement found 
* in the file LICENSE.txt that is included with this distribution. 
\*************************************************************************/ 

/** @file lvDCOMMain.cpp IOC Main Program
 *  @author Freddie Akeroyd, STFC ISIS Facility, GB
 *  
 */  

///  @example lvDCOM.db
///  Example EPICS db file for use with example.vi

///  @example st.cmd 
///  Example IOC Startup File.

///  @example lvinput.xml
///  An lvDOM configuration file, can be generated from @link controls.xml @endlink via @link lvstrings2input.xsl @endlink

///  @example controls.txt
///  Output of ExportVIStrings on example.vi

///  @example controls.xml
///  Output of running @link fix_xml.cmd @endlink on @link controls.txt @endlink

///  @example fix_xml.cmd
///  Command file to correct output of ExportVIStrings (@link controls.txt @endlink -> @link controls.xml @endlink)

///  @example lvDCOMinput.xsd
///  XML schema file for @link lvinput.xml @endlink

///  @example lvinput2db.xsl
///  Generate an initial EPICS db file from an @link lvinput.xml @endlink file

///  @example lvstrings2input.xsl
///  Generate an initial @link lvinput.xml @endlink file from an exported @link controls.xml @endlink file

#include <stddef.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "epicsThread.h"
#include "epicsExit.h"
#include "iocsh.h"

/// IOC Main Program
int main(int argc, char *argv[])
{
    if(argc >= 2) {    
        iocsh(argv[1]);
        epicsThreadSleep(.2);
    }
    iocsh(NULL);
    epicsExit(EXIT_SUCCESS);  // ensures epics exit handlers are called
    // Note that the following statement will never be executed
    return 0;
}
