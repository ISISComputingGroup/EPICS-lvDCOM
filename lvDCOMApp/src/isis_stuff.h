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

// TinyXPath
//#include <xpath_processor.h>  
//#include <xpath_static.h>  

struct ViRef
{
	LabVIEW::VirtualInstrumentPtr vi_ref;
	bool reentrant;
	bool started;    // did we start this vi because it was idle and autostart was enabled? 
	ViRef(LabVIEW::VirtualInstrumentPtr vi_ref_, bool reentrant_, bool started_) : vi_ref(vi_ref_), reentrant(reentrant_), started(started_) { }
	ViRef() : vi_ref(NULL), reentrant(false), started(false) { }
};


class ISISSTUFF
{
public:
	ISISSTUFF(const char *portName, const char *configFile, const char* host, int warnViIdle, int autostartVi);
	long nParams();
	void getParams(std::map<std::string,std::string>& res);
	template<typename T> void setLabviewValue(const std::string& portName, const char* param, const T& value);
	template<typename T> void getLabviewValue(const std::string& portName, const char* param, T* value);
	template<typename T> void getLabviewValue(const std::string& portName, const char* param, T* value, size_t nElements, size_t& nIn);
	~ISISSTUFF() { if (m_pxmldom != NULL) { m_pxmldom->Release(); m_pxmldom = 0; } }
private:
	bool m_warnViIdle;
	bool m_autostartVi;
	typedef std::map<std::wstring, ViRef> vi_map_t;
	vi_map_t m_vimap;
	epicsMutex m_lock;
//	TiXmlDocument* m_doc;
//	TiXmlElement* m_root;
	IXMLDOMDocument2 *m_pxmldom;
	CComBSTR m_extint;
	std::string m_host;
	std::string m_port;
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
	void stopAutoStartedVis();
};

#endif /* _ISIS_STUFF_H */
