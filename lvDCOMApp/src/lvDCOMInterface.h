#ifndef _ISIS_STUFF_H
#define _ISIS_STUFF_H

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

#import "LabVIEW.tlb" named_guids

#include <msxml2.h>

/// \file lvDCOMInterface.h header for #lvDCOMInterface class.

// TinyXPath
//#include <xpath_processor.h>  
//#include <xpath_static.h>  

struct ViRef
{
	LabVIEW::VirtualInstrumentPtr vi_ref;
	bool reentrant;  ///< is the VI reentrant
	bool started;    ///< did we start this vi because it was idle and #viStartIfIdle was specified  
	ViRef(LabVIEW::VirtualInstrumentPtr vi_ref_, bool reentrant_, bool started_) : vi_ref(vi_ref_), reentrant(reentrant_), started(started_) { }
	ViRef() : vi_ref(NULL), reentrant(false), started(false) { }
};

/// Options that can be passed from EPICS iocsh via #lvDCOMConfigure command.
/// In the iocBoot st.cmd file you will need to add the relevant integer enum values together and pass this single integer value.
enum lvDCOMOptions
{
    viWarnIfIdle = 1, 				///< If the LabVIEW VI is idle when we connect to it, issue a warning message  
	viStartIfIdle = 2, 				///< If the LabVIEW VI is idle when we connect to it, attempt to start it
	viStopOnExitIfStarted = 4, 		///< On IOC exit, stop any LabVIEW VIs that we started due to #viStartIfIdle being specified
	viAlwaysStopOnExit = 8			///< On IOC exit, stop any LabVIEW VIs that we have connected to
};	

class lvDCOMInterface
{
public:
	lvDCOMInterface(const char* configSection, const char *configFile, const char* host, int options, const char* username, const char* password);
	long nParams();
	void getParams(std::map<std::string,std::string>& res);
	template<typename T> void setLabviewValue(const char* param, const T& value);
	template<typename T> void getLabviewValue(const char* param, T* value);
	template<typename T> void getLabviewValue(const char* param, T* value, size_t nElements, size_t& nIn);
	~lvDCOMInterface() { if (m_pxmldom != NULL) { m_pxmldom->Release(); m_pxmldom = 0; } }
private:
	std::string m_configSection;  ///< section of \a configFile to load information from
	std::string m_host;
	std::string m_username;
	std::string m_password;
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

	void DomFromCOM();
	std::string doPath(const std::string& xpath);
	std::string doXPATH(const std::string& xpath);
	bool doXPATHbool(const std::string& xpath);
	void getViRef(BSTR vi_name, bool reentrant, LabVIEW::VirtualInstrumentPtr &vi);
	void createViRef(BSTR vi_name, bool reentrant, LabVIEW::VirtualInstrumentPtr &vi);
	void getLabviewValue(BSTR vi_name, BSTR control_name, VARIANT* value);
	void setLabviewValue(BSTR vi_name, BSTR control_name, const VARIANT& value);
	void setLabviewValueExt(BSTR vi_name, BSTR control_name, const VARIANT& value, VARIANT* results);
	void callLabview(BSTR vi_name, VARIANT& names, VARIANT& values, VARIANT_BOOL reentrant, VARIANT* results);
	COAUTHIDENTITY* createIdentity(const std::string& user, const std::string& domain, const std::string& pass);
	HRESULT setIdentity(COAUTHIDENTITY* pidentity, IUnknown* pUnk);
	static void epicsExitFunc(void* arg);
	void stopVis(bool only_ones_we_started);
	bool checkOption(lvDCOMOptions option) { return ( m_options & static_cast<int>(option) ) != 0; }
};

#endif /* _ISIS_STUFF_H */
