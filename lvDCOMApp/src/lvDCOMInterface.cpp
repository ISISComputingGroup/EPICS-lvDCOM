/*************************************************************************\ 
* Copyright (c) 2013 Science and Technology Facilities Council (STFC), GB. 
* All rights reverved. 
* This file is distributed subject to a Software License Agreement found 
* in the file LICENSE.txt that is included with this distribution. 
\*************************************************************************/ 

/// @file lvDCOMInterface.cpp Implementation of #lvDCOMInterface class.
/// @author Freddie Akeroyd, STFC ISIS Facility, GB

///  @example example.db
///  Example EPICS db file for use with example.vi - an initial version can be generated from @link lvinput.xml @endlink via the 
///  XSLT stylesheet @link lvinput2db.xsl @endlink. For the records below the asyn port \b ex1 will have been mapped to 
///  the \b frontpanel section of @link lvinput.xml @endlink via the lvDCOMConfigure() command in @link st.cmd @endlink and
///  individual driver parameters such as \b cont1 will correspond to param nodes in @link lvinput.xml @endlink.

///  @example st.cmd 
///  Example IOC Startup File. Calls lvDCOMConfigure() with appropriate arguments and #lvDCOMOptions to map the 
///  LabVIEW values defined in @link lvinput.xml @endlink, then 
///  loads @link example.db @endlink to export these LabVIEW values as process variables.

///  @example lvinput.xml
///  An lvDOM configuration file, loaded via lvDCOMConfigure() from @link st.cmd @endlink. This file specifies the mapping from
///  Asyn port name and associated driver parameters (used in @link example.db @endlink) -> LabVIEW front panel control/indicator.
///  An initial version of this file can be generated 
///  from @link controls.xml @endlink via the XSLT stylesheet @link lvstrings2input.xsl @endlink. This configuration file can also be used to 
///  generate an initial set of EPICS DB records @link example.db @endlink via the XSLT stylesheet @link lvinput2db.xsl @endlink 

///  @example controls.txt
///  Output of LabVIEW ExportVIStrings for example.vi - will next be processed by @link fix_xml.cmd @endlink to generate @link controls.xml @endlink

///  @example controls.xml
///  Output of running @link fix_xml.cmd @endlink on @link controls.txt @endlink - will next be used to generate @link lvinput.xml @endlink
///  via the XSLT stylesheet @link lvstrings2input.xsl @endlink  

///  @example fix_xml.cmd
///  Command file to adjust output of LabVIEW ExportVIStrings to allow parsing by XML utilities (@link controls.txt @endlink -> @link controls.xml @endlink)

///  @example lvDCOMinput.xsd
///  XML schema file for @link lvinput.xml @endlink

///  @example lvinput2db.xsl
///  XSLT stylesheet to generate an initial EPICS db file @link example.db @endlink from an @link lvinput.xml @endlink file

///  @example lvstrings2input.xsl
///  XSLT stylesheet to generate an initial @link lvinput.xml @endlink file from a @link controls.xml @endlink file

///  @example lvDCOM_int32.template
///  template file used by substitutions file generated from lvDCOMSECIConfigure()

///  @example lvDCOM_boolean.template
///  template file used by substitutions file generated from lvDCOMSECIConfigure()

///  @example lvDCOM_float64.template
///  template file used by substitutions file generated from lvDCOMSECIConfigure()

///  @example lvDCOM_string.template
///  template file used by substitutions file generated from lvDCOMSECIConfigure()

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
#include <tlhelp32.h>

#include <string>
#include <vector>
#include <map>
#include <list>
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>

#include "lvDCOMInterface.h"
#include "variant_utils.h"

#include <macLib.h>
#include <epicsGuard.h>
#include <cantProceed.h>
#include <errlog.h>

#if WITH_PCRE
#include <pcrecpp.h>
#else
/// dummy pcre implementation
namespace pcrecpp
{
    struct RE
	{
		RE(const char*) { }
		bool FullMatch(const char*) { return true; }
		bool GlobalReplace(const std::string, std::string*) { return true; }
	};
}
#endif

// replace characters invalid in XML
static std::string replaceWithEntities(const std::string& str)
{
	std::string res(str);
    pcrecpp::RE("&").GlobalReplace("&amp;", &res);
    pcrecpp::RE("<").GlobalReplace("&lt;", &res);
    pcrecpp::RE(">").GlobalReplace("&gt;", &res);
    pcrecpp::RE("\"").GlobalReplace("&quot;", &res);
    pcrecpp::RE("'").GlobalReplace("&apos;", &res);
    return res;
}

#define MAX_PATH_LEN 256

static epicsThreadOnceId onceId = EPICS_THREAD_ONCE_INIT;

/// The Microsoft ATL _com_error is not derived from std::exception hence this bit of code to throw our own COMexception() instead
static void __stdcall my_com_raise_error(HRESULT hr, IErrorInfo* perrinfo) 
{
	_com_error com_error(hr, perrinfo);
	//	std::string message = "(" + com_error.Source() + ") " + com_error.Description();
    _bstr_t desc = com_error.Description();
	std::string message = (desc.GetBSTR() != NULL ? desc : "");  // for LabVIEW generated messages, Description() already includes Source()
	throw COMexception(message, hr);
}

static void initCOM(void*)
{
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
	_set_com_error_handler(my_com_raise_error);    // replace default _com_raise_error
}

/// expand epics environment strings using previously saved environment  
/// based on EPICS macEnvExpand()
/// returns NULL if undefined macros
/// need to use free() on resturned string
char* lvDCOMInterface::envExpand(const char *str)
{
    long destCapacity = 128;
    char *dest = NULL;
    int n;
    do {
        destCapacity *= 2;
        /*
         * Use free/malloc rather than realloc since there's no need to
         * keep the original contents.
         */
        free(dest);
        dest = static_cast<char*>(mallocMustSucceed(destCapacity, "lvDCOMInterface::envExpand"));
        n = macExpandString(m_mac_env, str, dest, destCapacity);
    } while (n >= (destCapacity - 1));
    if (n < 0) {
        free(dest);
        dest = NULL;
    } else {
        size_t unused = destCapacity - ++n;

        if (unused >= 20)
            dest = static_cast<char*>(realloc(dest, n));
    }
    return dest;
}

// return "" if no value at path
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
            char* res = envExpand(CW2CT(bstrValue));
            if (res != NULL) {
                S_res = res;
                free(res);
            }
			SysFreeString(bstrValue);
		}
		pNode->Release();
	}
	//	else
	//	{
	//		throw std::runtime_error("doXPATH: cannot find " + xpath);
	//	}
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
            char* str = envExpand(CW2CT(bstrValue));
            if (str != NULL) {
                bool_str = str;
                free(str);
            }
			if (bool_str.size() == 0)
			{
				res = false;
			}
			// allow true / yes / non_zero_number
			// note: atol() returns 0 for non numeric strings, so OK in a test for "true"
			else if ( (bool_str[0] == 't') || (bool_str[0] == 'T') || (bool_str[0] == 'y') || (bool_str[0] == 'Y') || (atol(bool_str.c_str()) != 0) )
			{
				res = true;
			}
			else
			{
				res = false;
			}
			SysFreeString(bstrValue);
		}
		pNode->Release();
	}
	//	else
	//	{
	//		throw std::runtime_error("doXPATHbool: cannot find " + xpath);
	//	}
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

#endif /* #if 0 */

std::string lvDCOMInterface::doPath(const std::string& xpath)
{
	std::string S_res = doXPATH(xpath);
	std::replace(S_res.begin(), S_res.end(), '/', '\\');
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


/// \param[in] configSection @copydoc initArg1
/// \param[in] configFile @copydoc initArg2
/// \param[in] host @copydoc initArg3
/// \param[in] options @copydoc initArg4
/// \param[in] progid @copydoc initArg5
/// \param[in] username @copydoc initArg6
/// \param[in] password @copydoc initArg7
lvDCOMInterface::lvDCOMInterface(const char *configSection, const char* configFile, const char* host, int options, const char* progid, const char* username, const char* password) : 
m_configSection(configSection), m_pidentity(NULL), m_pxmldom(NULL), m_options(options), 
	m_progid(progid != NULL? progid : ""), m_username(username != NULL? username : ""), m_password(password != NULL ? password : ""),
	m_mac_env(NULL)
	
{
	epicsThreadOnce(&onceId, initCOM, NULL);
	if (host != NULL && host[0] != '\0') 
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
		m_host = "localhost";
	}
	if (macCreateHandle(&m_mac_env, NULL) != 0)
	{
		throw std::runtime_error("Cannot create mac handle");
	}
	// load current environment into m_mac_env, this is so we can create a macEnvExpand() equivalent 
	// but tied to the environment at a specific time. It is useful if we want to load the same 
	// XML file twice but with a macro defined differently in each case 
	for(char** cp = environ; *cp != NULL; ++cp)
	{
		char* str_tmp = strdup(*cp);
		char* equals_loc = strchr(str_tmp, '='); // split   name=value   string
		if (equals_loc != NULL)
		{
		    *equals_loc = '\0';
		    macPutValue(m_mac_env, str_tmp, equals_loc + 1);
		}
		free(str_tmp);
	}
	//	m_doc = new TiXmlDocument;
	//	if ( !m_doc->LoadFile(configFile) )
	//	{
	//		delete m_doc;
	//		m_doc = NULL;
	//		throw std::runtime_error("Cannot load " + std::string(configFile) + ": load failure");
	//	}
	//	m_root = m_doc->RootElement();
	if (configFile != NULL && *configFile != '\0')
	{
	    DomFromCOM();
	    short sResult = FALSE;
	    char* configFile_expanded = envExpand(configFile);
        if (configFile_expanded == NULL) {
            throw std::runtime_error("Cannot load XML \"" + m_configFile + "\" (expanded from \"" + std::string(configFile) + "\"): envExpand error");
        }
	    m_configFile = configFile_expanded;
	    HRESULT hr = m_pxmldom->load(_variant_t(configFile_expanded), &sResult);
	    free(configFile_expanded);
	    if(FAILED(hr))
	    {
		    throw std::runtime_error("Cannot load XML \"" + m_configFile + "\" (expanded from \"" + std::string(configFile) + "\"): load failure");
	    }
	    if (sResult != VARIANT_TRUE)
	    {
		    throw std::runtime_error("Cannot load XML \"" + m_configFile + "\" (expanded from \"" + std::string(configFile) + "\"): load failure");
	    }
	    std::cerr << "Loaded XML config file \"" << m_configFile << "\" (expanded from \"" << configFile << "\")" << std::endl;
	    m_extint = doPath("/lvinput/extint/@path").c_str();
	}
	epicsAtExit(epicsExitFunc, this);
	if (m_progid.size() > 0)
	{
		if ( CLSIDFromProgID(CT2W(m_progid.c_str()), &m_clsid) != S_OK )
		{
			throw std::runtime_error("Cannot find progId " + m_progid);
		}
	}
	else
	{
		m_clsid = LabVIEW::CLSID_Application;
		wchar_t* progid_str = NULL;
		if ( ProgIDFromCLSID(m_clsid, &progid_str) == S_OK )
		{
			m_progid = CW2CT(progid_str);
			CoTaskMemFree(progid_str);
		}
		else
		{
			m_progid = "LabVIEW.Application";
		}
	}
	wchar_t* clsid_str = NULL;
	if ( StringFromCLSID(m_clsid, &clsid_str) == S_OK )
	{
		std::cerr << "Using ProgID \"" << m_progid << "\" CLSID " << CW2CT(clsid_str) << std::endl;
		CoTaskMemFree(clsid_str);
	}
	else
	{
		std::cerr << "Using ProgID \"" << m_progid << "\" but StringFromCLSID() failed" << std::endl;
	}
	if ( checkOption(lvNoStart) )
	{
		waitForLabVIEW();
	}
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

/// wait for LabVIEW to have been running for m_minLVUptime seconds
double lvDCOMInterface::waitForLabVIEW()
{
	double lvuptime = getLabviewUptime();
	if (lvuptime < m_minLVUptime)
	{
	    errlogSevPrintf(errlogMinor, "LabVIEW not currently running - waiting for LabVIEW uptime of %.1f seconds...\n", m_minLVUptime);
        while ( (lvuptime = getLabviewUptime()) < m_minLVUptime )
	    {
		    epicsThreadSleep(5.0);
	    }
	}
	return lvuptime;
}
	
void lvDCOMInterface::getBlockDetails(std::vector< std::vector<std::string> >& values)
{
	CComBSTR vi_name("c:\\LabVIEW Modules\\dae\\monitor\\dae_monitor.vi");
	CComBSTR control_name("Parameter details");
	int n, nr, nc;

	waitForLabVIEW();
    // wait until table populated i.e. non zero number of rows, also non-black first block name
	do {
		epicsThreadSleep(5.0);
		CComVariant v;
	    getLabviewValue(vi_name, control_name, &v);
	    if ( v.vt != (VT_ARRAY | VT_BSTR) )
	    {
		    throw std::runtime_error("GetBlockDetails failed (type mismatch)");
	    }
		n = nr = nc = 0;
		values.clear();
	    CComSafeArray<BSTR> sa;
	    sa.Attach(v.parray);
		if (sa.GetDimensions() == 2)
		{
	        nr = sa.GetCount(0);
	        nc = sa.GetCount(1);
			if (nc > 5)
			{
				nc = 5; // we only want (and use) the first 5 columns
			}
	        values.resize(nr);
	        for(int i=0; i<nr; ++i)
	        {
				values[i].clear();
				values[i].reserve(nc);
	            for(int j=0; j<nc; ++j)
				{
					BSTR t = NULL;
					LONG dims[2] = { i, j };
					if (sa.MultiDimGetAt(dims, t) == S_OK)
					{
		                values[i].push_back(static_cast<const char*>(CW2CT(t)));
						SysFreeString(t);
						++n;
					}
				}
			}
	    }
	    sa.Detach();
	} while ( (nr * nc) == 0 || (nr * nc) != n || values[0][0].size() == 0 );
}

bool lvDCOMInterface::checkForNewBlockDetails()
{
	if ( !(m_options & static_cast<int>(lvDCOMOptions::lvSECIConfig)) )
	{
		return false;
	}
    std::vector< std::vector<std::string> > values;
	getBlockDetails(values);
	return (m_seci_values == values ? false : true);
}

/// generate XML and DB files for SECI blocks 
int lvDCOMInterface::generateFilesFromSECI(const char* portName, const char* macros, const char* configSection, const char* configFile, const char* dbSubFile, const char* blocks_match, bool no_setter)
{
	double lvuptime = waitForLabVIEW();
	errlogSevPrintf(errlogInfo, "LabVIEW has been running for %.1f seconds\n", lvuptime);
	std::cerr << "Waiting for block details to appear in dae_monitor.vi ..." << std::endl;
	getBlockDetails(m_seci_values);
	int nr = m_seci_values.size();
	std::cerr << "Found " << nr << " potential SECI blocks" << std::endl;
    char** pairs = NULL;
	macPushScope(m_mac_env);
	macParseDefns(m_mac_env, macros, &pairs);
	macInstallMacros(m_mac_env, pairs);
	
	std::fstream fs, fsdb;
	fs.open(configFile, std::ios::out);
	fsdb.open(dbSubFile, std::ios::out);
    fs << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    fs << "<lvinput xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://epics.isis.rl.ac.uk/lvDCOMinput/1.0\" xsi:schemaLocation=\"http://epics.isis.rl.ac.uk/lvDCOMinput/1.0 lvDCOMinput.xsd\">\n";
//	fs << "  <extint path=\"$(LVDCOM)/lvDCOMApp/src/extint/Main/Library/External Interface - Set Value.vi\"/>\n";
    // SECI instruments are already using this, but from a different source
    fs << "  <extint path=\"c:/labview modules/Common/External Interface/External Interface.llb/External Interface - Set Value.vi\"/>\n";    
    fs << "  <section name=\"" << configSection << "\">\n";
	if (blocks_match == NULL || *blocks_match == '\0')
	{
		blocks_match = ".*";
	}
    pcrecpp::RE blocks_re(blocks_match);
	int nblocks = 0;
	for(int i=0; i<nr; ++i)
	{
		std::string rsuffix, ssuffix, pv_type;
		std::string& name = m_seci_values[i][0];
		std::string vi_path = m_seci_values[i][1];
	    if ( blocks_re.FullMatch(name.c_str()) )
		{
		    std::cerr << "Processing block \"" << name << "\"" << std::endl;
			++nblocks;
		}
		else
		{
		    std::cerr << "Skipping block \"" << name << "\" as not matched by regular expression" << std::endl;
			continue;
		}
		std::string read_type("unknown"), set_type("unknown");
		if (m_seci_values[i][2] != "none")
		{
		    read_type = getLabviewValueType(CComBSTR(vi_path.c_str()), CComBSTR(m_seci_values[i][2].c_str()));
		}
		if (!no_setter && m_seci_values[i][3] != "none")
		{
		    set_type = getLabviewValueType(CComBSTR(vi_path.c_str()), CComBSTR(m_seci_values[i][3].c_str()));
		}
		std::replace(vi_path.begin(), vi_path.end(), '\\', '/');
        fs << "    <vi path=\"" << replaceWithEntities(vi_path) << "\">\n";
		if (set_type != "unknown")
		{
            fs << "      <param name=\"" << name << "_set\" type=\"" << set_type << "\">\n";
            fs << "        <read method=\"GCV\" target=\"" << replaceWithEntities(m_seci_values[i][3]) << "\"/>\n";
            fs << "        <set method=\"SCV\" extint=\"true\" target=\"" << replaceWithEntities(m_seci_values[i][3]) << "\"";
		    if (m_seci_values[i][4].size() > 0 && m_seci_values[i][4] != "none")
		    {
			    fs << " post_button=\"" << replaceWithEntities(m_seci_values[i][4]) << "\"";
		    }
		    fs << "/>\n";
		    fs << "      </param>\n";
			rsuffix = "_set";
			ssuffix = "_set";
			pv_type = set_type;
        }
		if (read_type != "unknown")
		{
            fs << "      <param name=\"" << name << "_read\" type=\"" << read_type << "\">\n";
            fs << "        <read method=\"GCV\" target=\"" << replaceWithEntities(m_seci_values[i][2]) << "\"/>\n";
		    fs << "      </param>\n";
			rsuffix = "_read";
			pv_type = read_type;
			if (ssuffix.size() == 0) // no set defined, generate a dummy one for PV template
			{
				ssuffix = "_read";
			}
		}
		fs << "    </vi>\n";
		// if you define a read for the seci block but not a set, map set -> read
		// if you define a set for the block but not a read, we will map the READ -> set
		// This is so scannng the read PV will always work 
		if (rsuffix.size() > 0 && ssuffix.size() > 0)
		{
		    fsdb  << "file \"${LVDCOM}/db/lvDCOM_" << pv_type << ".template\" {\n";
		    fsdb  << "    { P=\"" << envExpand("$(P=)") << "\",PORT=\"" << portName << "\",SCAN=\"" 
		          << envExpand("$(SCAN=1 second)") << "\",PARAM=\"" << name
				  << "\",NOSET=\"" << (no_setter ? "#" : " ")
				  << "\",RPARAM=\"" << name << rsuffix << "\",SPARAM=\"" << name << ssuffix << "\" }\n";
		    fsdb  << "}\n\n";
		}
		else
		{
			std::cerr << "Skipping DB for block \"" << name << "\" (unknown type)" << std::endl;			
			std::cerr << "Controls: \"" << m_seci_values[i][2] << "\" \"" << m_seci_values[i][3] << "\"" << std::endl;			
		}
	}
	fs << "  </section>\n";
	fs << "</lvinput>\n";
	fs.close();
	fsdb.close();
	macPopScope(m_mac_env);
	return nblocks;
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
	char control_name_xpath[MAX_PATH_LEN];
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
	char control_name_xpath[MAX_PATH_LEN];
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
	if (user.size() == 0)
	{
		return NULL;
	}
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

/// returns -1.0 if labview not running, else labview uptime in seconds
double lvDCOMInterface::getLabviewUptime()
{
  HANDLE hProcess;
  double lvUptime = -1.0;
  int lvCount = 0; // number of LabVIEW.exe processes found
  PROCESSENTRY32 pe32;
  FILETIME lvCreationTime, lvExitTime, lvKernelTime, lvUserTime, sysTime;
  epicsGuard<epicsMutex> _lock(m_lock); // just to restrict number of simultaneous snapshots
  HANDLE hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
  if( hProcessSnap == INVALID_HANDLE_VALUE )
  {
	return lvUptime;
  }
  pe32.dwSize = sizeof( PROCESSENTRY32 );
  if( !Process32First( hProcessSnap, &pe32 ) )
  {
    CloseHandle( hProcessSnap );
    return lvUptime;
  }
  do
  {
	if ( !stricmp(pe32.szExeFile, "LabVIEW.exe") )
	{
		++lvCount;
        hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
        if( hProcess != NULL )
	    {
		   if (GetProcessTimes(hProcess, &lvCreationTime, &lvExitTime, &lvKernelTime, &lvUserTime) != 0)
		   {
               GetSystemTimeAsFileTime(&sysTime);
		       lvUptime = diffFileTimes(sysTime, lvCreationTime);
		   }
           CloseHandle( hProcess );
	    }
	}
  } while( Process32Next( hProcessSnap, &pe32 ) );
  CloseHandle( hProcessSnap );
//  std::cerr << "lvuptime=" << lvUptime << "s lvcount=" << lvCount << std::endl;
  return lvUptime;
}

/// filetime uses 100ns units, returns difference in seconds
double lvDCOMInterface::diffFileTimes(const FILETIME& f1, const FILETIME& f2)
{
	ULARGE_INTEGER u1, u2;
	u1.LowPart = f1.dwLowDateTime;
	u1.HighPart = f1.dwHighDateTime;
	u2.LowPart = f2.dwLowDateTime;
	u2.HighPart = f2.dwHighDateTime;
	return static_cast<double>(u1.QuadPart - u2.QuadPart) / 1e7;
}

void lvDCOMInterface::maybeWaitForLabVIEWOrExit()
{
	if ( checkOption(lvNoStart) )
	{
		if (getLabviewUptime() < m_minLVUptime)
		{
			if ( checkOption(lvSECIConfig) )
			{
				// likely a seci restart, so exit and procServ will restart us ready for new config
				std::cerr << "Terminating as in SECI mode and no LabVIEW" << std::endl;
				epicsExit(0);
			}
			else
			{
				waitForLabVIEW();
				//throw std::runtime_error("LabVIEW not running and \"lvNoStart\" requested");
			}
		}
	}
}

// this is called with m_lock held
void lvDCOMInterface::createViRef(BSTR vi_name, bool reentrant, LabVIEW::VirtualInstrumentPtr& vi)
{
	epicsThreadOnce(&onceId, initCOM, NULL);
	std::wstring ws(vi_name, SysStringLen(vi_name));
	HRESULT hr = E_FAIL;
	// we do maybeWaitForLabVIEWOrExit() either side of this to try and avoid a race condition...
	maybeWaitForLabVIEWOrExit();
	if (m_lv != NULL)
	{
		try
		{
			hr = m_lv->CheckConnection();
		}
		catch(const std::exception&)
		{
			hr = E_FAIL;
		}
		if ( FAILED(hr) )
		{
			epicsThreadSleep(5.0);
		}
	}
	maybeWaitForLabVIEWOrExit();
	if (hr == S_OK)
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
		hr = CoCreateInstanceEx( m_clsid, NULL, CLSCTX_REMOTE_SERVER | CLSCTX_LOCAL_SERVER, &csi, 1, mq ); 
		if( FAILED( hr ) ) 
		{ 
			hr = CoCreateInstanceEx( m_clsid, NULL, CLSCTX_ALL, &csi, 1, mq );
		}
		if( FAILED( hr ) ) 
		{
			throw COMexception("CoCreateInstanceEx (LabVIEW) ", hr);
		} 
		if( S_OK != mq[ 0 ].hr || NULL == mq[ 0 ].pItf ) 
		{ 
			throw COMexception("CoCreateInstanceEx (LabVIEW)(mq) ", mq[ 0 ].hr);
		} 
		setIdentity(m_pidentity, mq[ 0 ].pItf);
		m_lv.Release();
		m_lv.Attach( reinterpret_cast< LabVIEW::_Application* >( mq[ 0 ].pItf ) ); 
		std::cerr << "Successfully connected to LabVIEW on " << m_host << std::endl;
	}
	else
	{
		std::cerr << "(Re)Making local connection to LabVIEW" << std::endl;
		m_pidentity = NULL;
		m_lv.Release();
		hr = m_lv.CoCreateInstance(m_clsid, NULL, CLSCTX_LOCAL_SERVER);
		if( FAILED( hr ) ) 
		{
			throw COMexception("CoCreateInstance (LabVIEW) ", hr);
		} 
		std::cerr << "Successfully connected to local LabVIEW" << std::endl;
	}
        if (checkOption(lvDCOMVerbose))
        {
	    std::cerr << "Attempting to access \"" << CW2CT(vi_name) << "\" on " << (m_host.size() > 0 ? m_host : "localhost") << std::endl;
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
	if (param == NULL || *param == '\0')
	{
		throw std::runtime_error("getLabviewValue: param is NULL");
	}
	CComVariant v;
	char vi_name_xpath[MAX_PATH_LEN], control_name_xpath[MAX_PATH_LEN];
	_snprintf(vi_name_xpath, sizeof(vi_name_xpath), "/lvinput/section[@name='%s']/vi[param[@name='%s']]/@path", m_configSection.c_str(), param);
	_snprintf(control_name_xpath, sizeof(control_name_xpath), "/lvinput/section[@name='%s']/vi/param[@name='%s']/read/@target", m_configSection.c_str(), param);
	CComBSTR vi_name(doPath(vi_name_xpath).c_str());
	CComBSTR control_name(doXPATH(control_name_xpath).c_str());
	if (vi_name.Length() == 0 || control_name.Length() == 0)
	{
		throw std::runtime_error("getLabviewValue: vi or control is NULL");
	}
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
	if (param == NULL || *param == '\0')
	{
		throw std::runtime_error("getLabviewValue: param is NULL");
	}
	CComVariant v;
	char vi_name_xpath[MAX_PATH_LEN], control_name_xpath[MAX_PATH_LEN];
	_snprintf(vi_name_xpath, sizeof(vi_name_xpath), "/lvinput/section[@name='%s']/vi[param[@name='%s']]/@path", m_configSection.c_str(), param);
	_snprintf(control_name_xpath, sizeof(control_name_xpath), "/lvinput/section[@name='%s']/vi/param[@name='%s']/read/@target", m_configSection.c_str(), param);
	CComBSTR vi_name(doPath(vi_name_xpath).c_str());
	CComBSTR control_name(doXPATH(control_name_xpath).c_str());
	if (vi_name.Length() == 0 || control_name.Length() == 0)
	{
		throw std::runtime_error("getLabviewValue: vi or control is NULL");
	}
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
	if (param == NULL || *param == '\0')
	{
		throw std::runtime_error("getLabviewValue: param is NULL");
	}
	CComVariant v;
	char vi_name_xpath[MAX_PATH_LEN], control_name_xpath[MAX_PATH_LEN];
	_snprintf(vi_name_xpath, sizeof(vi_name_xpath), "/lvinput/section[@name='%s']/vi[param[@name='%s']]/@path", m_configSection.c_str(), param);
	_snprintf(control_name_xpath, sizeof(control_name_xpath), "/lvinput/section[@name='%s']/vi/param[@name='%s']/read/@target", m_configSection.c_str(), param);
	CComBSTR vi_name(doPath(vi_name_xpath).c_str());
	CComBSTR control_name(doXPATH(control_name_xpath).c_str());
	if (vi_name.Length() == 0 || control_name.Length() == 0)
	{
		throw std::runtime_error("getLabviewValue: vi or control is NULL");
	}
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

/// determine best epics type for a labvier variable, this will be used
/// to choose the appropriate EPICS record template to use
std::string lvDCOMInterface::getLabviewValueType(BSTR vi_name, BSTR control_name)
{
	CComVariant v;
	try 
	{
	    getLabviewValue(vi_name, control_name, &v);
	}
	catch (const std::exception& ex)
	{
		std::cerr << "getLabviewValueType: Unable to read \"" << COLE2CT(control_name) << "\" on \"" << COLE2CT(vi_name) << "\": " << ex.what() << std::endl;
		return "unknown";		
	}
	VARTYPE vt = (&v)->vt;
	switch(vt)
	{
	    case VT_BOOL:
			return "boolean";		

		case VT_BSTR:
			return "string";		

		case VT_I1:
		case VT_I2:
		case VT_I4:
		case VT_INT:
		case VT_UI1:
		case VT_UI2:
		case VT_UI4:
		case VT_UINT:
			return "int32";

		default:
		    break;
	}
	if ( v.ChangeType(VT_R8) == S_OK )
	{
		return "float64";
	}
	else 
	{
		return "unknown";
	}	
}

class StringItem 
{
	CComBSTR m_bstr;
public:
	StringItem(lvDCOMInterface* dcom, const char* xpath, const char* config_section, const char* param, bool filepath = false)
	{
		char base_xpath[MAX_PATH_LEN];
		_snprintf(base_xpath, sizeof(base_xpath), xpath, config_section, param);
		if (filepath)
		{
			m_bstr = dcom->doPath(base_xpath).c_str();
		}
		else
		{
			m_bstr = dcom->doXPATH(base_xpath).c_str();
		}
	};
	const CComBSTR& bstr() { return m_bstr; }
	size_t size() { return m_bstr.Length(); }
	operator BSTR() { return m_bstr; }
};

class BoolItem 
{
	bool m_value;
public:
	BoolItem(lvDCOMInterface* dcom, const char* xpath, const char* config_section, const char* param)
	{
		char base_xpath[MAX_PATH_LEN];
		_snprintf(base_xpath, sizeof(base_xpath), xpath, config_section, param);
		m_value = dcom->doXPATHbool(base_xpath);
	};
	operator bool() { return m_value; }
};

template <>
void lvDCOMInterface::setLabviewValue(const char* param, const std::string& value)
{
	CComVariant v(value.c_str()), results, button_value(true);
	if (param == NULL || *param == '\0')
	{
		throw std::runtime_error("setLabviewValue: param is NULL");
	}
	StringItem vi_name(this, "/lvinput/section[@name='%s']/vi/@path", m_configSection.c_str(), "", true);
	StringItem control_name(this, "/lvinput/section[@name='%s']/vi/param[@name='%s']/set/@target", m_configSection.c_str(), param);
	StringItem post_button(this, "/lvinput/section[@name='%s']/vi/param[@name='%s']/set/@post_button", m_configSection.c_str(), param);
	BoolItem post_button_wait(this, "/lvinput/section[@name='%s']/vi/param[@name='%s']/set/@post_button_wait", m_configSection.c_str(), param);
	BoolItem use_ext(this, "/lvinput/section[@name='%s']/vi/param[@name='%s']/set/@extint", m_configSection.c_str(), param);
	if (vi_name.size() == 0 || control_name.size() == 0)
	{
		throw std::runtime_error("setLabviewValue: vi or control is NULL");
	}
	if (use_ext)
	{
		setLabviewValueExt(vi_name, control_name, v, &results);	
		if (post_button.size() > 0)
		{
			setLabviewValueExt(vi_name, post_button, button_value, &results);
		}
	}
	else
	{
		setLabviewValue(vi_name, control_name, v);	
		if (post_button.size() > 0)
		{
			setLabviewValue(vi_name, post_button, button_value);
		}
	}
	if (post_button_wait && (post_button.size() > 0) )
	{
		waitForLabviewBoolean(vi_name, post_button, false);	
	}
}

void lvDCOMInterface::waitForLabviewBoolean(BSTR vi_name, BSTR control_name, bool value)
{
	CComVariant v;
	bool done = false;
	while(!done)
	{
		getLabviewValue(vi_name, control_name, &v);
		if ( v.ChangeType(VT_BOOL) == S_OK )
		{
			done = ( v.boolVal == (value ? VARIANT_TRUE : VARIANT_FALSE) );
			v.Clear();
		}
		epicsThreadSleep(0.1);
	}	
}	

template <typename T>
void lvDCOMInterface::setLabviewValue(const char* param, const T& value)
{
	CComVariant v(value), results, button_value(true);
	if (param == NULL || *param == '\0')
	{
		throw std::runtime_error("setLabviewValue: param is NULL");
	}
	StringItem vi_name(this, "/lvinput/section[@name='%s']/vi/@path", m_configSection.c_str(), "", true);
	StringItem control_name(this, "/lvinput/section[@name='%s']/vi/param[@name='%s']/set/@target", m_configSection.c_str(), param);
	StringItem post_button(this, "/lvinput/section[@name='%s']/vi/param[@name='%s']/set/@post_button", m_configSection.c_str(), param);
	BoolItem post_button_wait(this, "/lvinput/section[@name='%s']/vi/param[@name='%s']/set/@post_button_wait", m_configSection.c_str(), param);
	BoolItem use_ext(this, "/lvinput/section[@name='%s']/vi/param[@name='%s']/set/@extint", m_configSection.c_str(), param);
	if (vi_name.size() == 0 || control_name.size() == 0)
	{
		throw std::runtime_error("setLabviewValue: vi or control is NULL");
	}
	if (use_ext)
	{
		setLabviewValueExt(vi_name, control_name, v, &results);	
		if (post_button.size() > 0)
		{
			setLabviewValueExt(vi_name, post_button,button_value, &results);
		}
	}
	else
	{
		setLabviewValue(vi_name, control_name, v);	
		if (post_button.size() > 0)
		{
			setLabviewValue(vi_name, post_button, button_value);
		}
	}
	if (post_button_wait && (post_button.size() > 0) )
	{
		waitForLabviewBoolean(vi_name, post_button, false);	
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

/// Helper for EPICS driver report function
void lvDCOMInterface::report(FILE* fp, int details)
{
	fprintf(fp, "XML ConfigFile: \"%s\"\n", m_configFile.c_str());
	fprintf(fp, "XML ConfigFile section: \"%s\"\n", m_configSection.c_str());
	fprintf(fp, "lvDCOMConfigure() Options: %d\n", m_options);
	fprintf(fp, "DCOM Target ProgID: \"%s\"\n", m_progid.c_str());
	fprintf(fp, "DCOM Target Host: \"%s\"\n", m_host.c_str());
	fprintf(fp, "DCOM Target Username: \"%s\"\n", m_username.c_str());
//	fprintf(fp, "Password: %s\n", m_password.c_str());
	std::string vi_name;
	for(vi_map_t::const_iterator it = m_vimap.begin(); it != m_vimap.end(); ++it)
	{
		vi_name = CW2CT(it->first.c_str());
		fprintf(fp, "LabVIEW VI: \"%s\"\n", vi_name.c_str());
	}
	if (details > 0)
	{
		for(std::map<std::string,std::string>::const_iterator it = m_xpath_map.begin(); it != m_xpath_map.end(); ++it)
		{
			fprintf(fp, "Config XPath: \"%s\" = \"%s\"\n", it->first.c_str(), it->second.c_str());
		}
		for(std::map<std::string,bool>::const_iterator it = m_xpath_bool_map.begin(); it != m_xpath_bool_map.end(); ++it)
		{
			fprintf(fp, "Config XPath: \"%s\" = %s\n", it->first.c_str(), (it->second ? "true" : "false") );
		}
	}
}

/// in seconds
double lvDCOMInterface::m_minLVUptime = 60.0;

std::vector< std::vector<std::string> > lvDCOMInterface::m_seci_values;

#ifndef DOXYGEN_SHOULD_SKIP_THIS

template void lvDCOMInterface::setLabviewValue(const char* param, const double& value);
template void lvDCOMInterface::setLabviewValue(const char* param, const int& value);

template void lvDCOMInterface::getLabviewValue(const char* param, double* value);
template void lvDCOMInterface::getLabviewValue(const char* param, int* value);

template void lvDCOMInterface::getLabviewValue(const char* param, double* value, size_t nElements, size_t& nIn);

template void lvDCOMInterface::getLabviewValue(const char* param, int* value, size_t nElements, size_t& nIn);

#endif /* DOXYGEN_SHOULD_SKIP_THIS */
