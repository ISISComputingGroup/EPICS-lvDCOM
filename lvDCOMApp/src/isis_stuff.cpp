#include <stdio.h>

//#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit
#include <atlbase.h>
#include <atlstr.h>
#include <atlcom.h>
#include <atlwin.h>
#include <atltypes.h>
#include <atlctl.h>
#include <atlhost.h>
#include <atlconv.h>
#include <atlsafe.h>
#include <comdef.h>

#include <string>
#include <vector>
#include <map>
#include <list>
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <iostream>

#include "isis_stuff.h"
#include "variant_utils.h"

class ScopedLock
{
private:
	epicsMutex& m_lock;
	ScopedLock() : m_lock(*(new epicsMutex)) { throw std::runtime_error("ScopedLock error"); }
	void operator=(const ScopedLock&) { throw std::runtime_error("ScopedLock error"); }
	
public:
	explicit ScopedLock(epicsMutex& lock) : m_lock(lock)
	{
		m_lock.lock();
	}
	~ScopedLock()
	{
		m_lock.unlock();
	}
};

std::string ISISSTUFF::doXPATH(const std::string& xpath)
{
	if (m_pxmldom == NULL)
	{
		throw std::runtime_error("m_cfg is NULL");
	}
	std::map<std::string,std::string>::const_iterator it = m_xpath_map.find(xpath);
	if (it != m_xpath_map.end())
	{
		return it->second;
	}
	ScopedLock _lock(m_lock);
	IXMLDOMNode *pNode = NULL;
	std::string S_res;
    BSTR bstrValue = NULL;
	HRESULT hr = m_pxmldom->selectSingleNode(_bstr_t(xpath.c_str()), &pNode);
	if (SUCCEEDED(hr) && pNode != NULL)
	{
		hr=pNode->get_text(&bstrValue);
		if (SUCCEEDED(hr))
		{
			S_res = CW2CT(bstrValue);
			SysFreeString(bstrValue);
		}
		pNode->Release();
	}
	else
	{
		throw std::runtime_error("m_cfg is NULL");
	}
	m_xpath_map[xpath] = S_res;
	return S_res;
}

bool ISISSTUFF::doXPATHbool(const std::string& xpath)
{
	if (m_pxmldom == NULL)
	{
		throw std::runtime_error("m_cfg is NULL");
	}
	std::map<std::string,bool>::const_iterator it = m_xpath_bool_map.find(xpath);
	if (it != m_xpath_bool_map.end())
	{
		return it->second;
	}
	ScopedLock _lock(m_lock);
	IXMLDOMNode *pNode = NULL;
	bool res = false;
    BSTR bstrValue = NULL;
	std::string bool_str;
	HRESULT hr = m_pxmldom->selectSingleNode(_bstr_t(xpath.c_str()), &pNode);
	if (SUCCEEDED(hr) && pNode != NULL)
	{
		hr=pNode->get_text(&bstrValue);
		if (SUCCEEDED(hr))
		{
			bool_str = CW2CT(bstrValue);
			if ( (bool_str.size() == 0) || (bool_str[0] == 'f') || (bool_str[0] == 'F') || (atol(bool_str.c_str()) == 0) )
			{
				res = false;
			}
			else
			{
				res = true;
			}
			SysFreeString(bstrValue);
		}
		pNode->Release();
	}
	else
	{
		throw std::runtime_error("m_cfg is NULL");
	}
	m_xpath_bool_map[xpath] = res;
	return res;
}

#if 0

std::string ISISSTUFF::doXPATH_old(const std::string& xpath)
{
	if (m_doc == NULL)
	{
		throw std::runtime_error("m_cfg is NULL");
	}
	std::map<std::string,std::string>::const_iterator it = m_xpath_map.find(xpath);
	if (it != m_xpath_map.end())
	{
		return it->second;
	}
	m_lock.lock();
	TinyXPath::xpath_processor xp_proc(m_root, xpath.c_str());
//	TIXML_STRING S_res = TinyXPath::S_xpath_string(m_doc->RootElement(), xpath.c_str());
	std::string S_res = xp_proc.S_compute_xpath().c_str();
	m_xpath_map[xpath] = S_res;
	m_lock.unlock();
	return S_res;
}

bool ISISSTUFF::doXPATHbool_old(const std::string& xpath)
{
	if (m_doc == NULL)
	{
		throw std::runtime_error("m_cfg is NULL");
	}
	std::map<std::string,bool>::const_iterator it = m_xpath_bool_map.find(xpath);
	if (it != m_xpath_bool_map.end())
	{
		return it->second;
	}
	m_lock.lock();
	TinyXPath::xpath_processor xp_proc(m_root, xpath.c_str());
	bool res = xp_proc.o_compute_xpath();
	m_xpath_bool_map[xpath] = res;
	m_lock.unlock();
	return res;
}

#endif
// Use Poco::Path to convert to native (windows) style path as config file is UNIX style

std::string ISISSTUFF::doPath(const std::string& xpath)
{
    std::string S_res = doXPATH(xpath);
	for(int i=0; i<S_res.size(); ++i)
	{
	    if (S_res[i] == '/')
		{
			S_res[i] = '\\';
		}
	}
	return S_res;
}

void ISISSTUFF::DomFromCOM()
{
	m_pxmldom = NULL;
    HRESULT hr=CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_SERVER,
               IID_IXMLDOMDocument2, (void**)&m_pxmldom);
	if (FAILED(hr))
	{
		throw std::runtime_error("Cannot load DomFromCom");
	}
	if (m_pxmldom != NULL)
	{
	    m_pxmldom->put_async(VARIANT_FALSE);
        m_pxmldom->put_validateOnParse(VARIANT_FALSE);
	    m_pxmldom->put_resolveExternals(VARIANT_FALSE); 
	}
	else
	{
		throw std::runtime_error("Cannot load DomFromCom");
	}
}


ISISSTUFF::ISISSTUFF(const char *portName, const char *configFile, const char* host) : /*m_doc(NULL),*/ m_pidentity(NULL), m_pxmldom(NULL)
{
		CoInitializeEx(NULL, COINIT_MULTITHREADED);
//		std::ifstream in(configFile);
//		Poco::XML::InputSource src(in);
//		Poco::XML::DOMParser parser;
//		Poco::AutoPtr<Poco::XML::Document> pDoc = parser.parse(&src);
//		Poco::XML::NodeIterator it(pDoc, Poco::XML::NodeFilter::SHOW_ALL);   // or SHOW_ELEMENTS ?
//		Poco::XML::Node* pNode = it.nextNode();

//		while (pNode)
//		{
//			std::cout<<pNode->nodeName()<<":"<< pNode->nodeValue()<<std::endl;
//			pNode = it.nextNode();
//		}
    if (host != NULL)
	{
	    m_host = host;
	}
	else
	{
//		char name_buffer[MAX_COMPUTERNAME_LENGTH + 1];
//		DWORD name_size = MAX_COMPUTERNAME_LENGTH + 1;
//		if ( GetComputerNameEx(ComputerNameNetBIOS, name_buffer, &name_size) != 0 )
//		{
//			m_host = name_buffer;
//		}
//		else
//		{
//			m_host = "localhost";
//		}			
		m_host = "";
	}
//	m_doc = new TiXmlDocument;
//	if ( !m_doc->LoadFile(configFile) )
//	{
//		delete m_doc;
//		m_doc = NULL;
//		throw std::runtime_error("Cannot load " + std::string(configFile) + ": load failure");
//	}
//	m_root = m_doc->RootElement();
	DomFromCOM();
	short sResult = FALSE;
	HRESULT hr = m_pxmldom->load(_variant_t(configFile), &sResult);
    if(FAILED(hr))
	{
		throw std::runtime_error("Cannot load " + std::string(configFile) + ": load failure");
	}
	if (sResult != VARIANT_TRUE)
	{
		throw std::runtime_error("Cannot load " + std::string(configFile) + ": load failure");
	}
  	m_extint = doPath("/lvinput/extint/@path").c_str();
}

COAUTHIDENTITY* ISISSTUFF::createIdentity(const std::string& user, const std::string&  domain, const std::string& pass)
{
    COAUTHIDENTITY* pidentity = new COAUTHIDENTITY;
    pidentity->Domain = (USHORT*)strdup(domain.c_str());
    pidentity->DomainLength = strlen((const char*)pidentity->Domain);
    pidentity->Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
    pidentity->Password = (USHORT*)strdup(pass.c_str());
    pidentity->PasswordLength = strlen((const char*)pidentity->Password);
    pidentity->User = (USHORT*)strdup(user.c_str());
    pidentity->UserLength = strlen((const char*)pidentity->User);
    return pidentity;
}

HRESULT ISISSTUFF::setIdentity(COAUTHIDENTITY* pidentity, IUnknown* pUnk)
{
    HRESULT hr;
    if (pidentity != NULL)
    {
       hr = CoSetProxyBlanket(pUnk, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, 
            RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, pidentity, EOAC_NONE);
        if (FAILED(hr))
        {
			std::cerr << "setIdentity failed" << std::endl;
            return hr;
        }
    }
    return S_OK;
}

int ISISSTUFF::testlv()
{
	CComPtr<LabVIEW::_Application> lv;
	HRESULT hr = lv.CoCreateInstance(LabVIEW::CLSID_Application, 0, CLSCTX_LOCAL_SERVER);

	if (FAILED(hr))
	{
//		AtlReportError(GetObjectCLSID(), "CoCreateInstance failed"); 
		return -1;
	}

	CComBSTR ccombstr((char *)lv->GetVersion());

	if (ccombstr.Length() == 0)
	{
		//Did not talk to LabVIEW
//		AtlReportError(GetObjectCLSID(), "Failed to communicate with LabVIEW"); 
		return -1;
	}
	
	return 0;
}

void ISISSTUFF::getViRef(BSTR vi_name, bool reentrant, LabVIEW::VirtualInstrumentPtr& vi)
{
	UINT len = SysStringLen(vi_name);
	std::wstring ws(vi_name, SysStringLen(vi_name));

	ScopedLock _lock(m_lock);
	vi_map_t::iterator it = m_vimap.find(ws);
	if(it != m_vimap.end())
	{
		vi = it->second.vi_ref;
		try
		{
			vi->GetExecState();
		}
		catch(...)
		{
			//Gets here if VI ref is not longer valid
			createViRef(vi_name, reentrant, vi);
		}
	}
	else
	{
		createViRef(vi_name, reentrant, vi);
	}
}


void ISISSTUFF::createViRef(BSTR vi_name, bool reentrant, LabVIEW::VirtualInstrumentPtr& vi)
{
	std::wstring ws(vi_name, SysStringLen(vi_name));
	HRESULT hr;
	if ( (m_lv != NULL) && (m_lv->CheckConnection() == S_OK) )
	{
		;
	}
	else if (m_host.size() > 0)
	{
		std::cerr << "(Re)Making connection to LabVIEW on " << m_host << std::endl;
		CComBSTR host(m_host.c_str());
		m_pidentity = createIdentity("spudulike", m_host, "reliablebeam");
		COAUTHINFO* pauth = new COAUTHINFO;
		COSERVERINFO csi = { 0, NULL, NULL, 0 };
		pauth->dwAuthnSvc = RPC_C_AUTHN_WINNT;
		pauth->dwAuthnLevel = RPC_C_AUTHN_LEVEL_DEFAULT;
		pauth->dwAuthzSvc = RPC_C_AUTHZ_NONE;
		pauth->dwCapabilities = EOAC_NONE;
		pauth->dwImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
		pauth->pAuthIdentityData = m_pidentity;
		pauth->pwszServerPrincName = NULL;
		csi.pwszName = host;
		csi.pAuthInfo = pauth;
		MULTI_QI mq[ 1 ] = { 0 }; 
		mq[ 0 ].pIID = &IID_IDispatch;  // &LabVIEW::DIID__Application; // &IID_IDispatch; 
		mq[ 0 ].pItf = NULL; 
		mq[ 0 ].hr   = S_OK; 
		hr = CoCreateInstanceEx( LabVIEW::CLSID_Application, NULL, CLSCTX_REMOTE_SERVER | CLSCTX_LOCAL_SERVER, &csi, 1, mq ); 
		if( FAILED( hr ) ) 
		{ 
			hr = CoCreateInstanceEx( LabVIEW::CLSID_Application, NULL, CLSCTX_ALL, &csi, 1, mq );
		}
		if( FAILED( hr ) ) 
		{
 			throw COMexception("CoCreateInstanceEx (LabVIEW) ", hr);
		} 
		if( S_OK != mq[ 0 ].hr || NULL == mq[ 0 ].pItf ) 
		{ 
 			throw COMexception("CoCreateInstanceEx (LabVIEW) ", mq[ 0 ].hr);
		} 
		setIdentity(m_pidentity, mq[ 0 ].pItf);
		m_lv.Release();
		m_lv.Attach( reinterpret_cast< LabVIEW::_Application* >( mq[ 0 ].pItf ) ); 
	}
	else
	{
		std::cerr << "(Re)Making connection to LabVIEW on localhost" << std::endl;
		m_pidentity = NULL;
		m_lv.Release();
		hr = m_lv.CoCreateInstance(LabVIEW::CLSID_Application, NULL, CLSCTX_LOCAL_SERVER);
		if( FAILED( hr ) ) 
		{
 			throw COMexception("CoCreateInstanceEx (LabVIEW) ", hr);
		} 
	}
	if (reentrant)
	{
		vi = m_lv->GetVIReference(vi_name, "", 1, 8);
		setIdentity(m_pidentity, vi);
	}
	else
	{
		//If a VI is reentrant then always get it as reentrant
		vi = m_lv->GetVIReference(vi_name, "", 0, 0);
		setIdentity(m_pidentity, vi);
		if (vi->IsReentrant)
		{
			vi = m_lv->GetVIReference(vi_name, "", 1, 8);
			setIdentity(m_pidentity, vi);
			reentrant = true;
		}
	}
	ViRef viref;
	viref.vi_ref = vi;
	viref.reentrant = reentrant;
	m_vimap[ws] = viref;
}


template <>
void ISISSTUFF::getLabviewValue(const std::string& portName, int addr, std::string* value)
{
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (value == NULL)
	{
		throw std::runtime_error("getLabviewValue failed (NULL)");
	}
	CComVariant v;
	char vi_name_xpath[256], control_name_xpath[256];
	_snprintf(vi_name_xpath, sizeof(vi_name_xpath), "/lvinput/item[@name='%s']/vi/@path", portName.c_str());
	_snprintf(control_name_xpath, sizeof(control_name_xpath), "/lvinput/item[@name='%s']/vi/control[@id=%d]/read/@target", portName.c_str(), addr);
	// Use Poco::Path to convert to native (windows) style path as config file is UNIX style
	CComBSTR vi_name(doPath(vi_name_xpath).c_str());
	CComBSTR control_name(doXPATH(control_name_xpath).c_str());
    getLabviewValue(vi_name, control_name, &v);
	if ( v.ChangeType(VT_BSTR) == S_OK )
	{
		*value = CW2CT(v.bstrVal);
	}
	else
	{
		throw std::runtime_error("getLabviewValue failed (ChangeType BSTR)");
	}
}

template<typename T> 
void ISISSTUFF::getLabviewValue(const std::string& portName, int addr, T* value, size_t nElements, size_t& nIn)
{
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (value == NULL)
	{
		throw std::runtime_error("getLabviewValue failed (NULL)");
	}
	CComVariant v;
	char vi_name_xpath[256], control_name_xpath[256];
	_snprintf(vi_name_xpath, sizeof(vi_name_xpath), "/lvinput/item[@name='%s']/vi/@path", portName.c_str());
	_snprintf(control_name_xpath, sizeof(control_name_xpath), "/lvinput/item[@name='%s']/vi/control[@id=%d]/read/@target", portName.c_str(), addr);
	// Use Poco::Path to convert to native (windows) style path as config file is UNIX style
	CComBSTR vi_name(doPath(vi_name_xpath).c_str());
	CComBSTR control_name(doXPATH(control_name_xpath).c_str());
    getLabviewValue(vi_name, control_name, &v);
	if ( v.vt != (VT_ARRAY | CVarTypeInfo<T>::VT) )
	{
		throw std::runtime_error("getLabviewValue failed (type mismatch)");
	}
	CComSafeArray<T> sa;
	sa.Attach(v.parray);
	nIn = ( sa.GetCount() > nElements ? nElements : sa.GetCount() );
	for(size_t i=0; i<nIn; ++i)
	{
		value[i] = sa.GetAt(i);
	}
	sa.Detach();
}

template <typename T>
void ISISSTUFF::getLabviewValue(const std::string& portName, int addr, T* value)
{
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (value == NULL)
	{
		throw std::runtime_error("getLabviewValue failed (NULL)");
	}
	CComVariant v;
	char vi_name_xpath[256], control_name_xpath[256];
	_snprintf(vi_name_xpath, sizeof(vi_name_xpath), "/lvinput/item[@name='%s']/vi/@path", portName.c_str());
	_snprintf(control_name_xpath, sizeof(control_name_xpath), "/lvinput/item[@name='%s']/vi/control[@id=%d]/read/@target", portName.c_str(), addr);
	// Use Poco::Path to convert to native (windows) style path as config file is UNIX style
	CComBSTR vi_name(doPath(vi_name_xpath).c_str());
	CComBSTR control_name(doXPATH(control_name_xpath).c_str());
    getLabviewValue(vi_name, control_name, &v);
	if ( v.ChangeType(CVarTypeInfo<T>::VT) == S_OK )
	{
		*value = v.*(CVarTypeInfo<T>::pmField);	
	}
	else
	{
		throw std::runtime_error("getLabviewValue failed (ChangeType)");
	}
}

void ISISSTUFF::getLabviewValue(BSTR vi_name, BSTR control_name, VARIANT* value)
{
	HRESULT hr = S_OK;
	LabVIEW::VirtualInstrumentPtr vi;
	getViRef(vi_name, false, vi);
	*value = vi->GetControlValue(control_name).Detach();
	vi.Detach();
	if (FAILED(hr))
	{
		throw std::runtime_error("getLabviewValue failed");
	}
}


template <>
void ISISSTUFF::setLabviewValue(const std::string& portName, int addr, const std::string& value)
{
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
	CComVariant v(value.c_str()), results;
	char vi_name_xpath[256], control_name_xpath[256], use_extint_xpath[256];
	_snprintf(vi_name_xpath, sizeof(vi_name_xpath), "/lvinput/item[@name='%s']/vi/@path", portName.c_str());
	_snprintf(control_name_xpath, sizeof(control_name_xpath), "/lvinput/item[@name='%s']/vi/control[@id=%d]/set/@target", portName.c_str(), addr);
	_snprintf(use_extint_xpath, sizeof(use_extint_xpath), "/lvinput/item[@name='%s']/vi/control[@id=%d]/set/@extint", portName.c_str(), addr);
	// Use Poco::Path to convert to native (windows) style path as config file is UNIX style
	CComBSTR vi_name(doPath(vi_name_xpath).c_str());
	CComBSTR control_name(doXPATH(control_name_xpath).c_str());
	bool use_ext = doXPATHbool(use_extint_xpath); 
	if (use_ext)
	{
		setLabviewValueExt(vi_name, control_name, v, &results);	
	}
	else
	{
		setLabviewValue(vi_name, control_name, v);	
	}
}

template <typename T>
void ISISSTUFF::setLabviewValue(const std::string& portName, int addr, const T& value)
{
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
	CComVariant v(value), results;
	char vi_name_xpath[256], control_name_xpath[256], use_extint_xpath[256];
	_snprintf(vi_name_xpath, sizeof(vi_name_xpath), "/lvinput/item[@name='%s']/vi/@path", portName.c_str());
	_snprintf(control_name_xpath, sizeof(control_name_xpath), "/lvinput/item[@name='%s']/vi/control[@id=%d]/set/@target", portName.c_str(), addr);
	_snprintf(use_extint_xpath, sizeof(use_extint_xpath), "/lvinput/item[@name='%s']/vi/control[@id=%d]/set/@extint", portName.c_str(), addr);
	// Use Poco::Path to convert to native (windows) style path as config file is UNIX style
	CComBSTR vi_name(doPath(vi_name_xpath).c_str());
	CComBSTR control_name(doXPATH(control_name_xpath).c_str());
	bool use_ext = doXPATHbool(use_extint_xpath); 
	if (use_ext)
	{
		setLabviewValueExt(vi_name, control_name, v, &results);	
	}
	else
	{
		setLabviewValue(vi_name, control_name, v);	
	}
}

void ISISSTUFF::setLabviewValue(BSTR vi_name, BSTR control_name, const VARIANT& value)
{
	HRESULT hr = S_OK;
	LabVIEW::VirtualInstrumentPtr vi;
	getViRef(vi_name, false, vi);
	hr = vi->SetControlValue(control_name, value);
	vi.Detach();
	if (FAILED(hr))
	{
		throw std::runtime_error("SetLabviewValue failed");
	}
}

void ISISSTUFF::setLabviewValueExt(BSTR vi_name, BSTR control_name, const VARIANT& value, VARIANT* results)
{

	CComSafeArray<BSTR> names(6);
	names[0].AssignBSTR(_bstr_t(L"VI Name"));
	names[1].AssignBSTR(_bstr_t(L"Control Name"));
	names[2].AssignBSTR(_bstr_t(L"String Control Value"));
	names[3].AssignBSTR(_bstr_t(L"Variant Control Value"));
	names[4].AssignBSTR(_bstr_t(L"Machine Name"));
	names[5].AssignBSTR(_bstr_t(L"Return Message"));

	_variant_t n;
	n.vt = VT_ARRAY | VT_BSTR;
	n.parray = names.Detach();

	CComSafeArray<VARIANT> values(6);
	values[0] = vi_name;
	values[1] = control_name;
	//values[2] = 
	values[3] = value;
	//values[4] = 
	//values[5] = 

	_variant_t v;
	v.vt = VT_ARRAY | VT_VARIANT;
	v.parray = values.Detach();
	//Must be called as reentrant!
	callLabview(m_extint, n, v, true, results);
}

void ISISSTUFF::callLabview(BSTR vi_name, VARIANT& names, VARIANT& values, VARIANT_BOOL reentrant, VARIANT* results)
{
	HRESULT hr = S_OK;

		LabVIEW::VirtualInstrumentPtr vi;

		if (reentrant)
		{
			getViRef(vi_name, true, vi);
		}
		else
		{
			getViRef(vi_name, false, vi);
		}

		hr = vi->Call(&names, &values);
		vi.Detach();

		CComVariant var(values);
		var.Detach(results);

	if (FAILED(hr))
	{
		throw std::runtime_error("CallLabviewValue failed");
	}
}


void ISISSTUFF::getViState(BSTR vi_name, VARIANT* value)
{
	HRESULT hr = S_OK;

		LabVIEW::VirtualInstrumentPtr vi;
		getViRef(vi_name, false, vi);
		CComVariant wrapper;
		wrapper.ChangeType(VT_INT);
		wrapper.Detach(value);
		value->intVal = vi->ExecState;
		vi.Detach();

	if (FAILED(hr))
	{
		throw std::runtime_error("GetViState failed");
	}
}

void ISISSTUFF::startVi(BSTR vi_name)
{
	HRESULT hr = S_OK;

	try
	{
		LabVIEW::VirtualInstrumentPtr vi;
		getViRef(vi_name, false, vi);
		// LabVIEW::ExecStateEnum.eIdle = 1
		if (vi->ExecState == 1)
		{
			vi->Run(true);
		}
		vi.Detach();
	}
	catch(...)
	{
		hr = -1;
	}

	if (FAILED(hr))
	{
		throw std::runtime_error("StartVi failed");
	}
}

void ISISSTUFF::stopVi(BSTR vi_name)
{
	HRESULT hr = S_OK;

	try
	{
		LabVIEW::VirtualInstrumentPtr vi;
		getViRef(vi_name, false, vi);
		// LabVIEW::ExecStateEnum.eIdle = 1
		// LabVIEW::ExecStateEnum.eRunTopLevel = 2
		if (vi->ExecState == 1 || vi->ExecState == 2)
		{
			vi->Abort();
		}
		vi.Detach();
	}
	catch(...)
	{
		hr = -1;
	}

	if (FAILED(hr))
	{
		throw std::runtime_error("StopVi failed");
	}
}

void ISISSTUFF::closeViFrontPanel(BSTR vi_name)
{
	HRESULT hr = S_OK;

	try
	{
		LabVIEW::VirtualInstrumentPtr vi;
		getViRef(vi_name, false, vi);
		hr = vi->CloseFrontPanel();
		vi.Detach();
	}
	catch(...)
	{
		hr = -1;
	}

	if (FAILED(hr))
	{
		throw std::runtime_error("CloseViFrontPanel failed");
	}
}

template void ISISSTUFF::setLabviewValue(const std::string& portName, int addr, const double& value);
template void ISISSTUFF::setLabviewValue(const std::string& portName, int addr, const int& value);

template void ISISSTUFF::getLabviewValue(const std::string& portName, int addr, double* value);
template void ISISSTUFF::getLabviewValue(const std::string& portName, int addr, int* value);

template void ISISSTUFF::getLabviewValue(const std::string& portName, int addr, double* value, size_t nElements, size_t& nIn);
template void ISISSTUFF::getLabviewValue(const std::string& portName, int addr, int* value, size_t nElements, size_t& nIn);
