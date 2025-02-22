/*************************************************************************\ 
* Copyright (c) 2013 Science and Technology Facilities Council (STFC), GB. 
* All rights reverved. 
* This file is distributed subject to a Software License Agreement found 
* in the file LICENSE.txt that is included with this distribution. 
\*************************************************************************/ 

/// @file lvDCOMInterface.h header for #lvDCOMInterface class. 
/// @author Freddie Akeroyd, STFC ISIS Facility, GB

#ifndef LV_DCOM_INTERFACE_H
#define LV_DCOM_INTERFACE_H

#include <stdio.h>

//#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <tchar.h>
//#include <sys/stat.h>
//#include <process.h>
//#include <fcntl.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit
#include <atlbase.h>
#include <atlstr.h>
#include <atlcom.h>
#include <atlwin.h>
#include <atltypes.h>
#include <atlctl.h>
#include <atlhost.h>

#include <string>
#include <vector>
#include <map>
#include <list>
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <iostream>

#include <epicsMutex.h>
#include <epicsThread.h>
#include <epicsExit.h>
#include <macLib.h>

//#import "LabVIEW.tlb" named_guids
// The above statement would generate labview.tlh and labview.tli from an installed copy of LabVIEW, but we include pre-built versions in the source
#include "labview.tlh"

#include <msxml2.h>

// TinyXPath, we tried this but it did not work, so use msxml as we will always be on Windows
//#include <xpath_processor.h>  
//#include <xpath_static.h>  

/// Hold a reference to a LabVIEW VI
struct ViRef
{
	LabVIEW::VirtualInstrumentPtr vi_ref;
	bool reentrant;  ///< is the VI reentrant
	bool started;    ///< did we start this vi because it was idle and #viStartIfIdle was specified  
	ViRef(LabVIEW::VirtualInstrumentPtr vi_ref_, bool reentrant_, bool started_) : vi_ref(vi_ref_), reentrant(reentrant_), started(started_) { }
	ViRef() : vi_ref(NULL), reentrant(false), started(false) { }
};

/// Options that can be passed from EPICS iocsh via #lvDCOMConfigure command.
/// In the iocBoot @link st.cmd @endlink file you will need to add the relevant integer enum values together and pass this single integer value.
enum lvDCOMOptions
{
	viWarnIfIdle = 1, 				///< (1)  If the LabVIEW VI is idle when we connect to it, issue a warning message  
	viStartIfIdle = 2, 				///< (2)  If the LabVIEW VI is idle when we connect to it, attempt to start it
	viStopOnExitIfStarted = 4, 		///< (4)  On IOC exit, stop any LabVIEW VIs that we started due to #viStartIfIdle being specified
	viAlwaysStopOnExit = 8,			///< (8)  On IOC exit, stop any LabVIEW VIs that we have connected to
	lvNoStart = 16,                  ///< (16) Do not start LabVIEW, connect to existing instance otherwise fail. As loading a Vi starts labview, vis will not be loaded or started until a labview instance is detected. Automatically set for lvDCOMSECIConfigure() 
	lvSECIConfig = 32,                  ///< (32) Automatically set if lvDCOMSECIConfigure() has been used
	lvSECINoSetter = 64,                  ///< (64) Do not generate setter XML / :SP PVs in SECI mode
	lvDCOMVerbose = 128                   ///< (128) print extra messages
};	

/// Manager class for LabVIEW DCOM Interaction. Parses an @link lvinput.xml @endlink file and provides access to the LabVIEW VI controls/indicators described within. 
class lvDCOMInterface
{
public:
	lvDCOMInterface(const char* configSection, const char *configFile, const char* host, int options, const char* progid, const char* username, const char* password);
	long nParams();
	void getParams(std::map<std::string,std::string>& res);
	template<typename T> void setLabviewValue(const char* param, const T& value);
	template<typename T> void getLabviewValue(const char* param, T* value);
	template<typename T> void getLabviewValue(const char* param, T* value, size_t nElements, size_t& nIn);
	~lvDCOMInterface() { if (m_pxmldom != NULL) { m_pxmldom->Release(); m_pxmldom = 0; } }
	std::string doPath(const std::string& xpath);
	std::string doXPATH(const std::string& xpath);
	bool doXPATHbool(const std::string& xpath);
	void report(FILE* fp, int details);
	static double diffFileTimes(const FILETIME& f1, const FILETIME& f2);
	int generateFilesFromSECI(const char* portName, const char* macros, const char* configSection, const char* configFile, 
	    const char* dbSubFile, const char* blocks_match, bool no_setter);
	bool checkForNewBlockDetails();

private:
	std::string m_configSection;  ///< section of \a configFile to load information from
	std::string m_configFile;   
	std::string m_host;
	std::string m_progid;
	CLSID m_clsid;
	std::string m_username;
	std::string m_password;
	static double m_minLVUptime; ///< minimum time labview must be running before connection made in "lvNoStart" mode
	int m_options; ///< the various #lvDCOMOptions currently in use
	typedef std::map<std::wstring, ViRef> vi_map_t;
	vi_map_t m_vimap;
	epicsMutex m_lock;
	//	TiXmlDocument* m_doc;
	//	TiXmlElement* m_root;
	IXMLDOMDocument2 *m_pxmldom;
	CComBSTR m_extint;
	CComPtr<LabVIEW::_Application> m_lv;
	COAUTHIDENTITY* m_pidentity;
	std::map<std::string,std::string> m_xpath_map;
	std::map<std::string,bool> m_xpath_bool_map;
	MAC_HANDLE *m_mac_env;
	static std::vector< std::vector<std::string> > m_seci_values; ///< horrible - do properly some time

	void DomFromCOM();
	char* envExpand(const char *str);
	void getViRef(BSTR vi_name, bool reentrant, LabVIEW::VirtualInstrumentPtr &vi);
	void createViRef(BSTR vi_name, bool reentrant, LabVIEW::VirtualInstrumentPtr &vi);
	void getLabviewValue(BSTR vi_name, BSTR control_name, VARIANT* value);
	void setLabviewValue(BSTR vi_name, BSTR control_name, const VARIANT& value);
	void setLabviewValueExt(BSTR vi_name, BSTR control_name, const VARIANT& value, VARIANT* results);
	void callLabview(BSTR vi_name, VARIANT& names, VARIANT& values, VARIANT_BOOL reentrant, VARIANT* results);
	void waitForLabviewBoolean(BSTR vi_name, BSTR control_name, bool value);
	COAUTHIDENTITY* createIdentity(const std::string& user, const std::string& domain, const std::string& pass);
	HRESULT setIdentity(COAUTHIDENTITY* pidentity, IUnknown* pUnk);
	static void epicsExitFunc(void* arg);
	void stopVis(bool only_ones_we_started);
	bool checkOption(lvDCOMOptions option) { return ( m_options & static_cast<int>(option) ) != 0; }
    double getLabviewUptime();
	std::string getLabviewValueType(BSTR vi_name, BSTR control_name);
	double waitForLabVIEW();
	void maybeWaitForLabVIEWOrExit();
	void getBlockDetails(std::vector< std::vector<std::string> >& values);
};

#endif /* LV_DCOM_INTERFACE_H */
