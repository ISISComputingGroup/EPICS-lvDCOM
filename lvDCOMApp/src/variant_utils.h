/*************************************************************************\ 
* Copyright (c) 2013 Science and Technology Facilities Council (STFC), GB. 
* All rights reverved. 
* This file is distributed subject to a Software License Agreement found 
* in the file LICENSE.txt that is included with this distribution. 
\*************************************************************************/ 

/// @file variant_utils.h 
/// @author Freddie Akeroyd, STFC ISIS Facility, GB

#ifndef VARIANT_UTILS_H
#define VARIANT_UTILS_H

/// create an C++ exception from a COM HRESULT
class COMexception : public std::runtime_error
{
public:
	explicit COMexception(const std::string& what_arg) : std::runtime_error(what_arg) { }
	explicit COMexception(const std::string& message, HRESULT hr) : std::runtime_error(com_message(message, hr)) { }
private:
	static std::string com_message(const std::string& message, HRESULT hr);
};

/// need to be compiled with /EHa if you want to use this via _set_se_translator()
/// note that _set_se_translator() needs to be called per thread
class Win32StructuredException : public std::runtime_error
{
public:
	explicit Win32StructuredException(const std::string& message) : std::runtime_error(message) { }
	explicit Win32StructuredException(unsigned int code, EXCEPTION_POINTERS *pExp) : std::runtime_error(win32_message(code, pExp)) { }
private:
	static std::string win32_message(unsigned int code, EXCEPTION_POINTERS * pExp);
};

int allocateArrayVariant(VARIANT* v, VARTYPE v_type, int* dims_array, int ndims);

int accessArrayVariant(VARIANT* v, float** values);
int accessArrayVariant(VARIANT* v, double** values);
int accessArrayVariant(VARIANT* v, long** values);
int accessArrayVariant(VARIANT* v, VARIANT** values);
int accessArrayVariant(VARIANT* v, BSTR** values);

int unaccessArrayVariant(VARIANT* v);

int arrayVariantLength(VARIANT* v);
int arrayVariantDimensions(VARIANT* v, int dims_array[], int& ndims);

template <typename T> 
int makeVariantFromArray(VARIANT* v, const T* the_array, int n);

template <typename T> 
int makeVariantFromArray(VARIANT* v, const std::vector<T>& the_array);

#endif /* VARIANT_UTILS_H */
