#ifndef VARIANT_UTILS
#define VARIANT_UTILS

class COMexception : public std::runtime_error
{
public:
	explicit COMexception(const std::string& what_arg) : std::runtime_error(what_arg) { }
//	explicit COMexception(const char* what_arg) : std::runtime_error(what_arg) { }
//	explicit COMexception(const COMexception& c) : std::runtime_error(c) { }
//	explicit COMexception() : std::runtime_error() { }
//	explicit COMexception(const char* message, HRESULT hr) : std::runtime_error(message + lookupHRESULT(hr)) { }
	explicit COMexception(const std::string& message, HRESULT hr) : std::runtime_error(com_message(message, hr)) { }
private:
	static std::string com_message(const std::string& message, HRESULT hr);
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


#endif /* VARIANT_UTILS */
