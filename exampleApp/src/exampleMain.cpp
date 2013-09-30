/* exampleMain.cpp */
/* Author:  Marty Kraimer Date:    17MAR2000 */

/** @file exampleMMain.cpp IOC Example Main Program
 *  
 */  
#include <stddef.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "epicsExit.h"
#include "epicsThread.h"
#include "iocsh.h"

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
