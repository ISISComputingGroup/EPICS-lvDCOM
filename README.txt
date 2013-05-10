<B>LvDCOM - an IOC to export National Instruments LabVIEW front panel variables as EPICS process variables</B>
<PRE>
The program allows you to "EPICS enable" LabVIEW without either purchasing the LabVIEW DSC module licence or needing 
to modify the Vi themselves. Features of the program are:
* Able to talk to either native LabVIEW Vis or a compiled LabVIEW application (.EXE)
  - For a compiled LabVIEW application, you need to select the "advanced properties" and check the 
    "enable ActiveX server" box and specify an "ActiveX server name" when you build the application. If you give, for example, 
	"MyApp" here then you would specify "MyApp.Application" as the progid parameter to lvDCOMConfigure() in your IOC st.cmd file 
* Can (optionally) automatically start and/or stop Vis on IOC startup/shutdown
* Can communicate with remote computers using standard DCOM authentication or supplIed username + password
</PRE>
<!--
    @author Freddie Akeroyd, STFC ISIS facility, UK (freddie.akeroyd at stfc.ac.uk)
-->
