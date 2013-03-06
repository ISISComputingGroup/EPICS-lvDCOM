#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <atlbase.h>
#include <atlcom.h>
#include <atlconv.h>

#include <comdef.h>

// TODO: reference additional headers your program requires here
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <sstream>

#include "variant_utils.h"


std::string COMexception::com_message(const std::string& message, HRESULT hr)
{
	_com_error ce(hr);
	std::ostringstream oss;
	oss << message << ": " << ce.ErrorMessage();
	return oss.str();
}

// 0 on success, -1 on error

int allocateArrayVariant(VARIANT* v, VARTYPE v_type, int* dims_array, int ndims)
{
			int i;
			V_VT(v) = VT_ARRAY | v_type;
			SAFEARRAYBOUND* sab = new SAFEARRAYBOUND[ndims];
			if (sab == NULL)
			{
				return -1;
			}
			// safe arrays are index other way round to C
			for(i=0; i<ndims; i++)
			{
				sab[i].lLbound = 1;
				sab[i].cElements = dims_array[ndims-i-1];
			}
		    V_UNION(v,parray) = SafeArrayCreate(v_type, ndims, sab);
			delete []sab;
			if (V_UNION(v,parray) == NULL)
			{
				return -1;
			}
			return 0;
}


static int accessArrayVariant(VARIANT* v, void** values, VARTYPE vt)
{
			VARTYPE vtt;
			HRESULT hr;
			*values = NULL;
			hr = SafeArrayGetVartype(V_UNION(v,parray), &vtt);
			if (!(vtt & vt))
			{
				return -1;
			}
			hr = SafeArrayAccessData(V_UNION(v,parray), values);
			if ( FAILED(hr) || (*values == NULL) )
			{
				*values = NULL;
				return -1;
			}
			return 0;
}

int accessArrayVariant(VARIANT* v, float** values)
{
	return accessArrayVariant(v, (void**)values, VT_R4);
}

int accessArrayVariant(VARIANT* v, double** values)
{
	return accessArrayVariant(v, (void**)values, VT_R8);
}

int accessArrayVariant(VARIANT* v, long** values)
{
	return accessArrayVariant(v, (void**)values, VT_I4);
}

int accessArrayVariant(VARIANT* v, BSTR** values)
{
	return accessArrayVariant(v, (void**)values, VT_BSTR);
}

int accessArrayVariant(VARIANT* v, VARIANT** values)
{
	return accessArrayVariant(v, (void**)values, VT_VARIANT);
}

int unaccessArrayVariant(VARIANT* v)
{
	HRESULT hr = SafeArrayUnaccessData(V_UNION(v,parray));
	if (FAILED(hr))
	{
		return -1;
	}
	return 0;
}


int arrayVariantLength(VARIANT* v)
{
	if (!(V_VT(v) & VT_ARRAY))
	{
		return 0;
	}
	SAFEARRAY* psa = V_UNION(v,parray);
	if (psa == NULL)
	{
		return 0;
	}
    int ndims = SafeArrayGetDim(psa);
	if (ndims <= 0)
	{
		return 0;
	}
	long lbounds, ubounds;
	int len = 1;
	int i;
	for(i=0; i<ndims; i++)
	{
		lbounds = ubounds = 0;
		SafeArrayGetLBound(psa, i+1, &lbounds);
		SafeArrayGetUBound(psa, i+1, &ubounds);
		len *= (ubounds - lbounds + 1);
	}
	return len;
}

int arrayVariantDimensions(VARIANT* v, int dims_array[], int& ndims)
{
	ndims = 0;
	dims_array[0] = 0;
	if (!(V_VT(v) & VT_ARRAY))
	{
		return -1;
	}
	SAFEARRAY* psa = V_UNION(v,parray);
	if (psa == NULL)
	{
		return -1;
	}
    ndims = SafeArrayGetDim(psa);
	long lbounds, ubounds;
	for(int i=0; i<ndims; i++)
	{
		lbounds = ubounds = 0;
		SafeArrayGetLBound(psa, i+1, &lbounds);
		SafeArrayGetUBound(psa, i+1, &ubounds);
		dims_array[i] = ubounds - lbounds + 1;
	}
	return 0;
}

template <typename T> 
int makeVariantFromArray(VARIANT* v, const std::vector<T>& the_array)
{
	return makeVariantFromArray(v, &(the_array[0]), the_array.size());
}

template <> 
int makeVariantFromArray(VARIANT* v, const std::vector<std::string>& the_array)
{
	int n = the_array.size();
	allocateArrayVariant(v, VT_BSTR, &n, 1);
	BSTR* v_array = NULL;
	accessArrayVariant(v, &v_array);
	for(int i=0; i<n; ++i)
	{
		CComBSTR bstr_wrapper(the_array[i].c_str());
		v_array[i] = bstr_wrapper.Detach();
	}
	unaccessArrayVariant(v);
	return 0;
}


template <const VARTYPE vt>
struct CPPType
{
	typedef int type;
};

template <>
struct CPPType<VT_I4>
{
	typedef int type;
};

template <>
struct CPPType<VT_R4>
{
	typedef float type;
};

template <typename T> 
int makeVariantFromArray(VARIANT* v, const T* the_array, int n)
{
	VARTYPE vt_type = CVarTypeInfo<T>::VT;
	allocateArrayVariant(v, vt_type, &n, 1);
	T* v_array = NULL;
	accessArrayVariant(v, &v_array);
	for(int i=0; i<n; ++i)
	{
		v_array[i] = the_array[i];
	}
	unaccessArrayVariant(v);
	return 0;
}

template <> 
int makeVariantFromArray(VARIANT* v, const char* the_array, int n)
{
	return -1;
}

template int makeVariantFromArray(VARIANT* v, const std::vector<float>& the_array);

#if 0

static void GenieVariableToVariant(GenieVariable& gv, VARIANT* v, const char* opt)
{
	int l, i;
	static OOP gxcominterface = typeNameToOOP("GXCominterface");
	HRESULT hr;
	VARTYPE vt_type;

	VariantInit(v);
    switch(gv.type())
	{
	    case GenieIntegerType:
		    V_VT(v) = VT_I4;
		    V_UNION(v,lVal) = (GenieInteger)gv;
			break;

		case GenieRealType:
			V_VT(v) = VT_R8;
			V_UNION(v,dblVal) = (GenieReal)gv;
			break;

		case GenieStringType:
			V_VT(v) = VT_BSTR;
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, 
			    (GenieString)gv, -1, olestr_buffer, MAX_OLE_SIZE);
			V_UNION(v,bstrVal) = SysAllocString(olestr_buffer);
			break;

		case GenieWorkspaceType:
		{
			GenieWorkspace& gw = (GenieWorkspace&)gv;
			if (gw.getClass() == gxcominterface)
			{
			    GenieIntegerArray gia = gw("value");
			    l = (GenieInteger)gw("type");
			    V_VT(v) = l;
			    switch(l)
				{
			        case VT_UNKNOWN:
					    memcpy(&(V_UNION(v,punkVal)), (g_integer*)gia, sizeof(IUnknown*));
					    break;

					case VT_DISPATCH:
						memcpy(&(V_UNION(v,pdispVal)), (g_integer*)gia, sizeof(IDispatch*));
						break;

					default:
						gerr << "DCOM: GenieVariableToVariant(workspace, unknown type) " << l << std::endl;
						break;
				}
			}
			else
			{
				gerr << "DCOM: Unsupported workspace type" << std::endl;
			}
		}
		break;

		case GenieWorkspaceArrayType:
			{
			GenieWorkspaceArray& gwa = (GenieWorkspaceArray&)gv;
			VARIANT* tmp;
			tmp = (VARIANT*)safeArraySetup(gwa, v, VT_VARIANT);
			if (tmp == NULL)
			{
				gerr << "DCOM: error" << std::endl;
				break;
			}
			for(i=0; i<gwa.length(); i++)
			{
				GenieWorkspace gw = gwa[i];
				GenieVariable gv = gw("value");
				GenieVariableToVariant(gv, &tmp[i], "");
			}
			hr = SafeArrayUnaccessData(V_UNION(v,parray));
			if (FAILED(hr))
			{
				gerr << "DCOM: failed" << std::endl;
			}

			}
			break;

		case GenieIntegerArrayType:
		{
			GenieIntegerArray& gia = (GenieIntegerArray&)gv;
			VARIANT* tmp;
			vt_type = (*opt == 'A' ? VT_I4 : VT_VARIANT);
			tmp = (VARIANT*)safeArraySetup(gia, v, vt_type);
			if (tmp == NULL)
			{
				gerr << "DCOM: error" << std::endl;
				break;
			}
			if (vt_type == VT_VARIANT)
			{
			    for(i=0; i<gia.length(); i++)
			    {
					V_VT(&tmp[i]) = VT_I4;
					V_UNION(&tmp[i],lVal) = gia[i];
			    }
			}
			else
			{
			    memcpy(tmp, (g_integer*)gia, gia.length() * 4);
			}
			hr = SafeArrayUnaccessData(V_UNION(v,parray));
			if (FAILED(hr))
			{
				gerr << "DCOM: failed" << std::endl;
			}
			break;
		}

		case GenieRealArrayType:
		{
			GenieRealArray& gra = (GenieRealArray&)gv;
			VARIANT* tmp;
			vt_type = (*opt == 'A' ? VT_R8 : VT_VARIANT);
			tmp = (VARIANT*)safeArraySetup(gra, v, vt_type);
			if (tmp == NULL)
			{
				gerr << "DCOM: error" << std::endl;
				break;
			}
			if (vt_type == VT_VARIANT)
			{
			    for(i=0; i<gra.length(); i++)
			    {
					V_VT(&tmp[i]) = VT_R8;
					V_UNION(&tmp[i],dblVal) = gra[i];
			    }
			}
			else
			{
			    memcpy(tmp, (g_real*)gra, gra.length() * 8);
			}
			hr = SafeArrayUnaccessData(V_UNION(v,parray));
			if (FAILED(hr))
			{
				gerr << "DCOM: failed" << std::endl;
			}
			break;
		}

		case GenieStringArrayType:
		{
		    GenieStringArray& gsa = (GenieStringArray&)gv;
			VARIANT* tmp;
			vt_type = (*opt == 'A' ? VT_BSTR : VT_VARIANT);
			tmp = (VARIANT*)safeArraySetup(gsa, v, vt_type);
			if (tmp == NULL)
			{
				gerr << "DCOM: error" << std::endl;
				break;
			}
			for(i=0; i<gsa.length(); i++)
			{
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, 
						(GenieString)gsa[i], -1, olestr_buffer, 
						MAX_OLE_SIZE);
				if (vt_type == VT_VARIANT)
				{
					V_VT(&tmp[i]) = VT_BSTR;
					V_UNION(&tmp[i],bstrVal) = SysAllocString(olestr_buffer);
				}
				else
				{
					((BSTR*)tmp)[i] = SysAllocString(olestr_buffer);
				}
			}
			hr = SafeArrayUnaccessData(V_UNION(v,parray));
			if (FAILED(hr))
			{
				gerr << "DCOM: failed" << std::endl;
			}
			break;
		}

		default:
			gerr << "DCOM: GenieVariableToVariant(unknown type) " << std::endl;
			break;
	}
}

static void VariantToGenieVariable(VARIANT* v, GenieVariable& gv);

static int getVariantArrayLength(VARIANT* v)
{
	if (!(V_VT(v) & VT_ARRAY))
	{
		return 0;
	}
	SAFEARRAY* psa = V_UNION(v,parray);
	if (psa == NULL)
	{
		return 0;
	}
    int ndims = SafeArrayGetDim(psa);
	long lbounds, ubounds;
	int len = 1;
	int i;
	for(i=0; i<ndims; i++)
	{
		lbounds = ubounds = 0;
		SafeArrayGetLBound(psa, i+1, &lbounds);
		SafeArrayGetUBound(psa, i+1, &ubounds);
		len *= (ubounds - lbounds + 1);
	}
	return len;
}

static void VariantToGenieVariableArray(VARIANT* v, GenieVariable& gv)
{
	SAFEARRAY* psa = V_UNION(v,parray);  // *(v->pparray) ????
	if (psa == NULL)
	{
		gerr << "DCOM: VariantToGenieVariableArray error" << std::endl;
		return;
	}
    int ndims = SafeArrayGetDim(psa);
	long *lbounds = new long[ndims];
	long *ubounds = new long[ndims];
	int* dims_array = new int[ndims];
	int len = 1;
	int i, j, l;
	HRESULT hr;
	for(i=0; i<ndims; i++)
	{
		SafeArrayGetLBound(psa, i+1, lbounds+i);
		SafeArrayGetUBound(psa, i+1, ubounds+i);
		dims_array[ndims - i - 1] = ubounds[i] - lbounds[i] + 1;
		len *= dims_array[ndims - i - 1];
	}
// check for variable type mismatch specified in array
	VARTYPE vt, vvt;
	vvt = (V_VT(v) & ~VT_ARRAY);
	void *pdata = NULL;
#if !defined(__VMS)
	hr = SafeArrayGetVartype(psa, &vt);
	if ( FAILED(hr) || (vt != vvt) )
	{
		gerr << "DCOM: VariantToGenieVariableArray - possible data type mismatch?" << std::endl;
	    return;
	}
#else
	vt = vvt;
#endif /* !defined(__VMS) */
	hr = SafeArrayAccessData(psa, &pdata);
	if ( FAILED(hr) || (pdata == NULL) )
	{
		gerr << "DCOM: error" << std::endl;
		return;
	}
	GenieIntegerArray* gia;
	GenieRealArray* gra;
	GenieStringArray *gsa;
	GenieWorkspaceArray *gwa;
	VARIANT *pv;
	if (vt == VT_VARIANT)
	{
//	ginf << "Variant Array length " << len << std::endl;
	gwa = new GenieWorkspaceArray(len);
	for(i=0; i<len; i++)
	{
		pv = (VARIANT*)pdata + i;
		GenieVariable gvv;
		VariantToGenieVariable(pv, gvv);
		GenieWorkspace gw;
		gw("value", gvv.value());
		(*gwa)(i, gw);
	}
	gv.newType(GenieWorkspaceArrayType, gwa->value());
	}
	else
	{
//	ginf << "Normal array length " << len << " type " << vt << std::endl;
//	int elmsize = SafeArrayGetElemsize(psa);
//	gerr << "element size = " << elmsize << std::endl;
    switch(vt)
	{
//		case VT_UI1:
//		case VT_I2:
//		case VT_UI2:
//		case VT_BOOL:

		case VT_I4:
		case VT_UI4:
			gia = new GenieIntegerArray((long*)pdata, dims_array, ndims);
			gv.newType(GenieIntegerArrayType, gia->value());
			break;


		case VT_INT:
		case VT_UINT:
			gia = new GenieIntegerArray((int*)pdata, dims_array, ndims);
			gv.newType(GenieIntegerArrayType, gia->value());
			break;

		case VT_R4:
			gra = new GenieRealArray((float*)pdata, dims_array, ndims);
			gv.newType(GenieRealArrayType, gra->value());
			break;

		case VT_R8:
			gra = new GenieRealArray((double*)pdata, dims_array, ndims);
			gv.newType(GenieRealArrayType, gra->value());
			break;

		case VT_BSTR:
			gsa = new GenieStringArray(dims_array, ndims);
			BSTR* pBstr;
			for(i=0; i<len; i++)
			{
				pBstr = (BSTR*)pdata + i;
				l = SysStringLen(*pBstr);
				j = WideCharToMultiByte(CP_ACP, 0, *pBstr, l, 
					str_buffer, MAX_OLE_SIZE, NULL, NULL);
				str_buffer[j] = '\0';
				(*gsa)(i, str_buffer);
			}
			gv.newType(GenieStringArrayType, gsa->value());
			break;
				
		default:
			gerr << "VariantToGenieVariableArray(unknown type) " << vt << std::endl;
			break;
	}	
    }
	SafeArrayUnaccessData(psa);
	delete []dims_array;
	delete []ubounds;
	delete []lbounds;
}

static void VariantToGenieVariable(VARIANT* v, GenieVariable& gv)
{
	int i, l;
	VARIANT temp_v;
	iunk_item iunk;
	idisp_item idsp;
	VariantInit(&temp_v);
	GenieIntegerArray gia(2);
    if (V_VT(v) & VT_ARRAY)
	{
//		ginf << "DCOM: got array" << std::endl;
		VariantToGenieVariableArray(v, gv);
		return;
	}
    switch(V_VT(v))
	{
		case VT_EMPTY:
		    break;

		case VT_I1:
		case VT_UI1:
		case VT_I2:
		case VT_UI2:
	    case VT_I4:
		case VT_UI4:
		case VT_INT:
		case VT_UINT:
		{
			GenieInteger gi;
			VariantChangeType(&temp_v, v, 0, VT_I4);
			gi = V_UNION(&temp_v,lVal);
		    gv.newType(GenieIntegerType, gi);
			break;
		}

		case VT_BOOL:
		{
			GenieInteger gi;
			gi = (V_UNION(v,boolVal) != 0 ? 1 : 0);
			gv.newType(GenieIntegerType, gi);
			break;
		}

		case VT_R4:
		case VT_R8:
			{
				GenieReal gr;
				VariantChangeType(&temp_v, v, 0, VT_R8);
				gr = V_UNION(&temp_v,dblVal);
				gv.newType(GenieRealType, gr);
				break;
			}
		case VT_BSTR:
			{
				GenieString gs;
				l = SysStringLen(V_UNION(v,bstrVal));
				i = WideCharToMultiByte(CP_ACP, 0, V_UNION(v,bstrVal), l, 
					str_buffer, MAX_OLE_SIZE, NULL, NULL);
				str_buffer[i] = '\0';
				gs = str_buffer;
				gv.newType(GenieStringType, gs);
				break;
			}

		case VT_UNKNOWN:
			{
				GenieWorkspace gw("GXCominterface");
				gia((g_integer*)&(V_UNION(v,punkVal)), sizeof(IUnknown*) / sizeof(g_integer));
				gw("name", "IUnknown");
				gw("type", VT_UNKNOWN);
				gw("value", gia);
				gv.newType(GenieWorkspaceType, gw);
				iunk_list.push(iunk_item(V_UNION(v,punkVal)));
				break;
			}

		case VT_DISPATCH:
			{
				GenieWorkspace gw("GXCominterface");
				gia((g_integer*)&(V_UNION(v,pdispVal)), sizeof(IDispatch*) / sizeof(g_integer));
				gw("name", "IDispatch");
				gw("type", VT_DISPATCH);
				gw("value", gia);
				gv.newType(GenieWorkspaceType, gw);
				idisp_list.push(idisp_item(V_UNION(v,pdispVal), 0));
				break;
			}

				
		default:
			gerr << "DCOM: VariantToGenieVariable(unknown type) " << V_VT(v) << std::endl;
			break;
	}	
}



static void DCOMCleanupVariant(VARIANT* v)
{
	VARTYPE vt, vvt;
	HRESULT hr;
	VARIANT* pv = NULL;
	int i, len;
	vvt = V_VT(v);
	vt = (vvt & ~VT_ARRAY);
	if ( (vvt & VT_ARRAY) && (vt == VT_VARIANT) )
	{
		SAFEARRAY* psa = V_UNION(v,parray);
		len = getVariantArrayLength(v);
		hr = SafeArrayAccessData(psa, (void**)&pv);
		if ( SUCCEEDED(hr) && (pv != NULL) )
		{
			for(i=0; i<len; i++)
			{
				DCOMCleanupVariant(pv + i);
			}
			SafeArrayUnaccessData(psa);
		}
		VariantClear(v);	// now free up array itself
		return;
	}
	switch(vt)
	{
		case VT_EMPTY:
		case VT_R4:
		case VT_R8:
		case VT_I1:
		case VT_UI1:
		case VT_I2:
		case VT_UI2:
	    case VT_I4:
		case VT_UI4:
		case VT_INT:
		case VT_UINT:
		case VT_BOOL:
		case VT_BSTR:
			VariantClear(v);	// free contents and mark as empty
			break;

		case VT_DISPATCH:
		case VT_UNKNOWN:
			VariantInit(v);		// mark it as empty so it doesn't get freed
			break;

		default:
			gerr << "DCOM: CleanupVariant(unknown type) " << V_VT(v)<< "/" << vt << std::endl;
			break;
	}
}

#endif