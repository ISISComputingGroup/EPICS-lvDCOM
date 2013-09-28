/*************************************************************************\ 
* Copyright (c) 2013 Science and Technology Facilities Council (STFC), GB. 
* All rights reverved. 
* This file is distributed subject to a Software License Agreement found 
* in the file LICENSE.txt that is included with this distribution. 
\*************************************************************************/ 

/** @file lvDCOMMain.cpp 
*  @author Freddie Akeroyd, STFC ISIS Facility, GB
*  @example lvDCOM.db
*  @example st.cmd
*  @example lvinput.xml
*/

#include <stddef.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "epicsThread.h"
#include "epicsExit.h"
#include "iocsh.h"

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
