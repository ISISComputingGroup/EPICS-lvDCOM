<B>LvDCOM - an IOC to export LabVIEW front panel variables to the EPICS environment</B>
<PRE>
* Able to talk to either native LabVIEW Vis or a compiled LabVIEW application (.EXE)
  - For a LabVIEW application, you need to select the "advanced properties" and check the 
    "enable ActiveX server" box and specify an "ActiveX server name". If you give, for example, 
	"MyApp" here then you should specify "MyApp.Application" as the progid parameter to lvDCOMConfigure 

* options to automatically start and/or stop Vis
* can communicate with remote computers using standard DCOM authentication or suppled username + password
* No need to purchase the LabVIEW DSC module licence to do epics
* no need to modify Vi at all
</PRE>