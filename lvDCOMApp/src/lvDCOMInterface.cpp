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

#include "lvDCOMInterface.h"
#include "variant_utils.h"

#include <macLib.h>
#include <epicsGuard.h>

static epicsThreadOnceId onceId = EPICS_THREAD_ONCE_INIT;

static void initCOM(void*)
{
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
}

/// The Microsoft ATL _com_error is not derived from std::exception hence this bit of code to throw our own COMexception() instead
void __stdcall _com_raise_error(HRESULT hr, IErrorInfo* perrinfo) 
{
	_com_error com_error(hr, perrinfo);
//	std::string message = "(" + com_error.Source() + ") " + com_error.Description();
	std::string message = com_error.Description();  // for LabVIEW generated messages, Description() already includes Source()
    throw COMexception(message, hr);
}

std::string lvDCOMInterface::doXPATH(const std::string& xpath)
{
	if (m_pxmldom == NULL)
	{
		throw std::runtime_error("m_pxmldom is NULL");
	}
	epicsGuard<epicsMutex> _lock(m_lock);
	std::map<std::string,std::string>::const_iterator it = m_xpath_map.find(xpath);
	if (it != m_xpath_map.end())
	{
		return it->second;
	}
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
		throw std::runtime_error("doXPATH: cannot find " + xpath);
	}
	m_xpath_map[xpath] = S_res;
	return S_res;
}

bool lvDCOMInterface::doXPATHbool(const std::string& xpath)
{
	if (m_pxmldom == NULL)
	{
		throw std::runtime_error("m_pxmldom is NULL");
	}
	epicsGuard<epicsMutex> _lock(m_lock);
	std::map<std::string,bool>::const_iterator it = m_xpath_bool_map.find(xpath);
	if (it != m_xpath_bool_map.end())
	{
		return it->second;
	}
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
		throw std::runtime_error("doXPATHbool: cannot find " + xpath);
	}
	m_xpath_bool_map[xpath] = res;
	return res;
}

#if 0

std::string lvDCOMInterface::doXPATH_old(const std::string& xpath)
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

bool lvDCOMInterface::doXPATHbool_old(const std::string& xpath)
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

std::string lvDCOMInterface::doPath(const std::string& xpath)
{
    std::string S_res = doXPATH(xpath);
	char* exp_str = macEnvExpand(S_res.c_str());
	S_res = exp_str;
	free(exp_str);
	for(int i=0; i<S_res.size(); ++i)
	{
	    if (S_res[i] == '/')
		{
			S_res[i] = '\\';
		}
	}
	return S_res;
}

void lvDCOMInterface::DomFromCOM()
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

/// 
/// \param[in] configSection @copydoc initArg1
/// \param[in] configFile @copydoc initArg2
/// \param[in] host @copydoc initArg3
/// \param[in] options @copydoc initArg4
/// \param[in] username @copydoc initArg5
/// \param[in] password @copydoc initArg6
lvDCOMInterface::lvDCOMInterface(const char *configSection, const char* configFile, const char* host, int options, const char* username, const char* password) : 
            m_configSection(configSection), m_pidentity(NULL), m_pxmldom(NULL), m_options(options), 
			m_username(username != NULL? username : ""), m_password(password != NULL ? password : "")
{
	epicsThreadOnce(&onceId, initCOM, NULL);
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
	char* configFile_expanded = macEnvExpand(configFile);
	HRESULT hr = m_pxmldom->load(_variant_t(configFile_expanded), &sResult);
	free(configFile_expanded);
    if(FAILED(hr))
	{
		throw std::runtime_error("Cannot load " + std::string(configFile) + ": load failure");
	}
	if (sResult != VARIANT_TRUE)
	{
		throw std::runtime_error("Cannot load " + std::string(configFile) + ": load failure");
	}
  	m_extint = doPath("/lvinput/extint/@path").c_str();
	epicsAtExit(epicsExitFunc, this);
}

void lvDCOMInterface::epicsExitFunc(void* arg)
{
    lvDCOMInterface* dcomint = static_cast<lvDCOMInterface*>(arg);
	if (dcomint == NULL)
	{
		return;
	}
	if ( dcomint->checkOption(viAlwaysStopOnExit) )
	{
		dcomint->stopVis(false);
	}
	else if ( dcomint->checkOption(viStopOnExitIfStarted) )
	{
		dcomint->stopVis(true);
	}
}

void lvDCOMInterface::stopVis(bool only_ones_we_started)
{
   for(vi_map_t::const_iterator it = m_vimap.begin(); it != m_vimap.end(); ++it)
	{
		LabVIEW::VirtualInstrumentPtr vi_ref = it->second.vi_ref;
	    if ( (!only_ones_we_started || it->second.started) && (vi_ref != NULL) )
		{
			if (vi_ref->ExecState != LabVIEW::eIdle) // don't try to stop it if it is already stopped
			{
				std::cerr << "stopping \"" << CW2CT(it->first.c_str()) << "\" as it was auto-started and is still running" << std::endl;
				try
				{
					vi_ref->Abort();
				}
				catch(const std::exception& ex)
				{
					std::cerr << "error stopping vi: " << ex.what() << std::endl;
				}
				catch(...)  
				{ 
					std::cerr << "error stopping vi: unknown" << std::endl;
				}
			}
		}
	}
}

long lvDCOMInterface::nParams()
{
	long n = 0;
	char control_name_xpath[256];
	_snprintf(control_name_xpath, sizeof(control_name_xpath), "/lvinput/section[@name='%s']/vi/param", m_configSection.c_str());
	IXMLDOMNodeList* pXMLDomNodeList = NULL;
	HRESULT hr = m_pxmldom->selectNodes(_bstr_t(control_name_xpath), &pXMLDomNodeList);
	if (SUCCEEDED(hr) && pXMLDomNodeList != NULL)
	{
		pXMLDomNodeList->get_length(&n);
		pXMLDomNodeList->Release();
	}
	return n;
}

void lvDCOMInterface::getParams(std::map<std::string,std::string>& res)
{
	res.clear();
	char control_name_xpath[256];
	_snprintf(control_name_xpath, sizeof(control_name_xpath), "/lvinput/section[@name='%s']/vi/param", m_configSection.c_str());
	IXMLDOMNodeList* pXMLDomNodeList = NULL;
	HRESULT hr = m_pxmldom->selectNodes(_bstr_t(control_name_xpath), &pXMLDomNodeList);
	if (FAILED(hr) || pXMLDomNodeList == NULL)
	{
		return;
	}
	IXMLDOMNode *pNode, *pAttrNode1, *pAttrNode2;
	long n = 0;
	pXMLDomNodeList->get_length(&n);
	for(long i=0; i<n; ++i)
	{
		pNode = NULL;
		hr = pXMLDomNodeList->get_item(i, &pNode);
		if (SUCCEEDED(hr) && pNode != NULL)
		{
			IXMLDOMNamedNodeMap *attributeMap = NULL;
			pAttrNode1 = pAttrNode2 = NULL;
			pNode->get_attributes(&attributeMap);
			hr = attributeMap->getNamedItem(_bstr_t("name"), &pAttrNode1);
			hr = attributeMap->getNamedItem(_bstr_t("type"), &pAttrNode2);
			BSTR bstrValue1 = NULL, bstrValue2 = NULL;
			hr=pAttrNode1->get_text(&bstrValue1);
			hr=pAttrNode2->get_text(&bstrValue2);
			res[std::string(COLE2CT(bstrValue1))] = COLE2CT(bstrValue2);
			SysFreeString(bstrValue1);
			SysFreeString(bstrValue2);
			pAttrNode1->Release();
			pAttrNode2->Release();
			attributeMap->Release();
			pNode->Release();
		}
	}	
	pXMLDomNodeList->Release();
}

COAUTHIDENTITY* lvDCOMInterface::createIdentity(const std::string& user, const std::string&  domain, const std::string& pass)
{
    COAUTHIDENTITY* pidentity = new COAUTHIDENTITY;
    pidentity->Domain = (USHORT*)strdup(domain.c_str());
    pidentity->DomainLength = static_cast<ULONG>(strlen((const char*)pidentity->Domain));
    pidentity->Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
    pidentity->Password = (USHORT*)strdup(pass.c_str());
    pidentity->PasswordLength = static_cast<ULONG>(strlen((const char*)pidentity->Password));
    pidentity->User = (USHORT*)strdup(user.c_str());
    pidentity->UserLength = static_cast<ULONG>(strlen((const char*)pidentity->User));
    return pidentity;
}

HRESULT lvDCOMInterface::setIdentity(COAUTHIDENTITY* pidentity, IUnknown* pUnk)
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


void lvDCOMInterface::getViRef(BSTR vi_name, bool reentrant, LabVIEW::VirtualInstrumentPtr& vi)
{
	UINT len = SysStringLen(vi_name);
	std::wstring ws(vi_name, SysStringLen(vi_name));

	epicsGuard<epicsMutex> _lock(m_lock);
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


void lvDCOMInterface::createViRef(BSTR vi_name, bool reentrant, LabVIEW::VirtualInstrumentPtr& vi)
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
		m_pidentity = createIdentity(m_username, m_host, m_password);
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
	ViRef viref(vi, reentrant, false);
	// LabVIEW::ExecStateEnum::eIdle = 1
	// LabVIEW::ExecStateEnum::eRunTopLevel = 2
	if (vi->ExecState == LabVIEW::eIdle)
	{
		if ( checkOption(viStartIfIdle) ) 
		{
			std::cerr << "Starting \"" << CW2CT(vi_name) << "\" on " << (m_host.size() > 0 ? m_host : "localhost") << std::endl;
			vi->Run(true);
			viref.started = true;
		}
		else if ( checkOption(viWarnIfIdle) )
		{
			std::cerr << "\"" << CW2CT(vi_name) << "\" is not running on " << (m_host.size() > 0 ? m_host : "localhost") << " and autostart is disabled" << std::endl;
		}
	}
	m_vimap[ws] = viref;
}


template <>
void lvDCOMInterface::getLabviewValue(const char* param, std::string* value)
{
	if (value == NULL)
	{
		throw std::runtime_error("getLabviewValue failed (NULL)");
	}
	CComVariant v;
	char vi_name_xpath[256], control_name_xpath[256];
	_snprintf(vi_name_xpath, sizeof(vi_name_xpath), "/lvinput/section[@name='%s']/vi/@path", m_configSection.c_str());
	_snprintf(control_name_xpath, sizeof(control_name_xpath), "/lvinput/section[@name='%s']/vi/param[@name='%s']/read/@target", m_configSection.c_str(), param);
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
void lvDCOMInterface::getLabviewValue(const char* param, T* value, size_t nElements, size_t& nIn)
{
	if (value == NULL)
	{
		throw std::runtime_error("getLabviewValue failed (NULL)");
	}
	CComVariant v;
	char vi_name_xpath[256], control_name_xpath[256];
	_snprintf(vi_name_xpath, sizeof(vi_name_xpath), "/lvinput/section[@name='%s']/vi/@path", m_configSection.c_str());
	_snprintf(control_name_xpath, sizeof(control_name_xpath), "/lvinput/section[@name='%s']/vi/param[@name='%s']/read/@target", m_configSection.c_str(), param);
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
	for(LONG i=0; i<nIn; ++i)
	{
		value[i] = sa.GetAt(i);
	}
	sa.Detach();
}

template <typename T>
void lvDCOMInterface::getLabviewValue(const char* param, T* value)
{
	if (value == NULL)
	{
		throw std::runtime_error("getLabviewValue failed (NULL)");
	}
	CComVariant v;
	char vi_name_xpath[256], control_name_xpath[256];
	_snprintf(vi_name_xpath, sizeof(vi_name_xpath), "/lvinput/section[@name='%s']/vi/@path", m_configSection.c_str());
	_snprintf(control_name_xpath, sizeof(control_name_xpath), "/lvinput/section[@name='%s']/vi/param[@name='%s']/read/@target", m_configSection.c_str(), param);
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

void lvDCOMInterface::getLabviewValue(BSTR vi_name, BSTR control_name, VARIANT* value)
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
void lvDCOMInterface::setLabviewValue(const char* param, const std::string& value)
{
	CComVariant v(value.c_str()), results;
	char vi_name_xpath[256], control_name_xpath[256], use_extint_xpath[256];
	_snprintf(vi_name_xpath, sizeof(vi_name_xpath), "/lvinput/section[@name='%s']/vi/@path", m_configSection.c_str());
	_snprintf(control_name_xpath, sizeof(control_name_xpath), "/lvinput/section[@name='%s']/vi/param[@name='%s']/set/@target", m_configSection.c_str(), param);
	_snprintf(use_extint_xpath, sizeof(use_extint_xpath), "/lvinput/section[@name='%s']/vi/param[@name='%s']/set/@extint", m_configSection.c_str(), param);
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
void lvDCOMInterface::setLabviewValue(const char* param, const T& value)
{
	CComVariant v(value), results;
	char vi_name_xpath[256], control_name_xpath[256], use_extint_xpath[256];
	_snprintf(vi_name_xpath, sizeof(vi_name_xpath), "/lvinput/section[@name='%s']/vi/@path", m_configSection.c_str());
	_snprintf(control_name_xpath, sizeof(control_name_xpath), "/lvinput/section[@name='%s']/vi/param[@name='%s']/set/@target", m_configSection.c_str(), param);
	_snprintf(use_extint_xpath, sizeof(use_extint_xpath), "/lvinput/section[@name='%s']/vi/param[@name='%s']/set/@extint", m_configSection.c_str(), param);
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

void lvDCOMInterface::setLabviewValue(BSTR vi_name, BSTR control_name, const VARIANT& value)
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

void lvDCOMInterface::setLabviewValueExt(BSTR vi_name, BSTR control_name, const VARIANT& value, VARIANT* results)
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

void lvDCOMInterface::callLabview(BSTR vi_name, VARIANT& names, VARIANT& values, VARIANT_BOOL reentrant, VARIANT* results)
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

template void lvDCOMInterface::setLabviewValue(const char* param, const double& value);
template void lvDCOMInterface::setLabviewValue(const char* param, const int& value);

template void lvDCOMInterface::getLabviewValue(const char* param, double* value);
template void lvDCOMInterface::getLabviewValue(const char* param, int* value);

template void lvDCOMInterface::getLabviewValue(const char* param, double* value, size_t nElements, size_t& nIn);
template void lvDCOMInterface::getLabviewValue(const char* param, int* value, size_t nElements, size_t& nIn);
