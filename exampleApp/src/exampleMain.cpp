/* exampleMain.cpp */
/* Author:  Marty Kraimer Date:    17MAR2000 */

/** @file exampleMain.cpp IOC Example Main Program.
 * This program will load @link st.cmd @endlink which in turn will call lvDCOMConfigure()
 * to export LabVIEW values into the EPICS world as defined via @link lvinput.xml @endlink and @link example.db @endlink
 */  
#include <stddef.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "epicsExit.h"
#include "epicsThread.h"
#include "iocsh.h"

/// IOC main program.
int main(int argc,char *argv[])
{
    if(argc>=2) {    
        iocsh(argv[1]);
        epicsThreadSleep(.2);
    }
    iocsh(NULL);
    epicsExit(EXIT_SUCCESS);
    return(0);
}
