#include <stddef.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "epicsThread.h"
#include "epicsExit.h"
#include "iocsh.h"

/** @mainpage lvDCOM (LabVIEW DCOM IOC)

\section into_sec Introduction
The introduction

\example example_lvinput.xml

\htmlinclude README.txt

*/
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
