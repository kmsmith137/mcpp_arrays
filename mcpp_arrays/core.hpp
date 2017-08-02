#ifndef _MCPP_ARRAYS_CORE_HPP
#define _MCPP_ARRAYS_CORE_HPP

#include <cstring>
#include <cstdlib>

#include <memory>
#include <string>
#include <sstream>
#include <complex>
#include <stdexcept>

#if (__cplusplus < 201103) && !defined(__GXX_EXPERIMENTAL_CXX0X__)
#error "This source file needs to be compiled with -std=c++11"
#endif

namespace mcpp_arrays {
#if 0
}  // emacs pacifier
#endif


constexpr int max_allowed_ndim = 100;


enum mcpp_typeid {
    MCPP_INT8 = 0,    // must be first
    MCPP_INT16 = 1,
    MCPP_INT32 = 2,
    MCPP_INT64 = 3,
    MCPP_UINT8 = 4,
    MCPP_UINT16 = 5,
    MCPP_UINT32 = 6,
    MCPP_UINT64 = 7,
    MCPP_FLOAT32 = 8,
    MCPP_FLOAT64 = 9,
    MCPP_COMPLEX64 = 10,
    MCPP_COMPLEX128 = 11,
    MCPP_INVALID = 12   // must be last
};


// Virtual base class which "owns" allocated memory,
// and releases it (via its destructor) when the last
// reference goes away.

struct mcpp_reaper {
    virtual ~mcpp_reaper() { }
};


// malloc_reaper: most common case, where allocated
// memory is released with free().

struct malloc_reaper : public mcpp_reaper {
    void *p = nullptr;
    malloc_reaper(void *p_) : p(p_) { }
    virtual ~malloc_reaper() { free(p); p = nullptr; }
};


// -------------------------------------------------------------------------------------------------


inline bool mcpp_typeid_is_valid(mcpp_typeid id)
{
    return (id >= MCPP_INT8) && (id < MCPP_INVALID);
}

inline const char *mcpp_typestr(mcpp_typeid id)
{
    switch (id)  
	{
        case MCPP_INT8: return "MCPP_INT8";
	case MCPP_INT16: return "MCPP_INT16";
	case MCPP_INT32: return "MCPP_INT32";
	case MCPP_INT64: return "MCPP_INT64";
	case MCPP_UINT8: return "MCPP_UINT8";
	case MCPP_UINT16: return "MCPP_UINT16";
	case MCPP_UINT32: return "MCPP_UINT32";
	case MCPP_UINT64: return "MCPP_UINT64";
	case MCPP_FLOAT32: return "MCPP_FLOAT32";
	case MCPP_FLOAT64: return "MCPP_FLOAT64";
	case MCPP_COMPLEX64: return "MCPP_COMPLEX64";
	case MCPP_COMPLEX128: return "MCPP_COMPLEX128";
	case MCPP_INVALID: return "MCPP_INVALID";
    }

    return "invalid mcpp_arrays::mcpp_typeid";
}

template<typename T>
inline ssize_t mcpp_sizeof(mcpp_typeid id)
{
    return sizeof(T);
}

template<>
inline ssize_t mcpp_sizeof<void> (mcpp_typeid id)
{
    switch (id) {
	case MCPP_INT8: return 1;
	case MCPP_INT16: return 2;
	case MCPP_INT32: return 4;
	case MCPP_INT64: return 8;
	case MCPP_UINT8: return 1;
	case MCPP_UINT16: return 2;
	case MCPP_UINT32: return 4;
	case MCPP_UINT64: return 8;
	case MCPP_FLOAT32: return 4;
	case MCPP_FLOAT64: return 8;
	case MCPP_COMPLEX64: return 8;
	case MCPP_COMPLEX128: return 16;
	case MCPP_INVALID: break;  // compiler pacifier
    }
    
    throw std::runtime_error("mcpp_arrays::mcpp_sizeof(): invalid typeid");
}


// -------------------------------------------------------------------------------------------------
//
// Type_traits


// is_complex<T>::value is 'true' for T=complex<float> or T=complex<double>
template<typename T> struct is_complex { static constexpr bool value = false; };
template<> struct is_complex<std::complex<float>> { static constexpr bool value = true; };
template<> struct is_complex<std::complex<double>> { static constexpr bool value = true; };


// is_mcpp_type<T>::value is 'true' if an mcpp_typeid exists for the type T.
template<typename T, bool VoidAllowed = false>
struct is_mcpp_type
{
    static constexpr bool ivalue = std::is_integral<T>::value && (sizeof(T)==1 || sizeof(T)==2 || sizeof(T)==4 || sizeof(T)==8);
    static constexpr bool fvalue = std::is_floating_point<T>::value && (sizeof(T)==4 || sizeof(T)==8);
    static constexpr bool cvalue = is_complex<T>::value && (sizeof(T)==8 || sizeof(T)==16);
    static constexpr bool value = ivalue || fvalue || cvalue;
};


template<bool VoidAllowed>
struct is_mcpp_type<void,VoidAllowed> { static constexpr bool value = VoidAllowed; };


// -------------------------------------------------------------------------------------------------
//
// The boilerplate below defines mcpp_type<T>::id, the compile-time typeid corresponding to C++ type T.
//
// FIXME: is there a way (using static_assert?) to get a helpful compile-time error if
// mcpp_type<T> is instantiated for an invalid type T?


template<typename T, 
	 int S = sizeof(T),
	 bool SI = std::is_integral<T>::value && std::is_signed<T>::value,
	 bool UI = std::is_integral<T>::value && !std::is_signed<T>::value,
	 bool F = std::is_floating_point<T>::value,
	 bool C = is_complex<T>::value>
struct mcpp_type { 
    static_assert(is_mcpp_type<T>::value, "mcpp_arrays::mcpp_type<T> was invoked for invalid type T");
};


template<typename T> struct mcpp_type<T,1,true,false,false,false>  { static constexpr mcpp_typeid id = MCPP_INT8; };
template<typename T> struct mcpp_type<T,2,true,false,false,false>  { static constexpr mcpp_typeid id = MCPP_INT16; };
template<typename T> struct mcpp_type<T,4,true,false,false,false>  { static constexpr mcpp_typeid id = MCPP_INT32; };
template<typename T> struct mcpp_type<T,8,true,false,false,false>  { static constexpr mcpp_typeid id = MCPP_INT64; };
template<typename T> struct mcpp_type<T,1,false,true,false,false>  { static constexpr mcpp_typeid id = MCPP_UINT8; };
template<typename T> struct mcpp_type<T,2,false,true,false,false>  { static constexpr mcpp_typeid id = MCPP_UINT16; };
template<typename T> struct mcpp_type<T,4,false,true,false,false>  { static constexpr mcpp_typeid id = MCPP_UINT32; };
template<typename T> struct mcpp_type<T,8,false,true,false,false>  { static constexpr mcpp_typeid id = MCPP_UINT64; };
template<typename T> struct mcpp_type<T,4,false,false,true,false>  { static constexpr mcpp_typeid id = MCPP_FLOAT32; };
template<typename T> struct mcpp_type<T,8,false,false,true,false>  { static constexpr mcpp_typeid id = MCPP_FLOAT64; };
template<typename T> struct mcpp_type<T,8,false,false,false,true>  { static constexpr mcpp_typeid id = MCPP_COMPLEX64; };
template<typename T> struct mcpp_type<T,16,false,false,false,true> { static constexpr mcpp_typeid id = MCPP_COMPLEX128; };


// -------------------------------------------------------------------------------------------------
//
// Intended as helpers for constructors


inline int check_ndim(int ndim, const char *where = nullptr)
{
    if (ndim < 0)
	throw std::runtime_error(std::string(where ? where : "mcpp_arrays") + ": attempt to create array with ndim < 0");
    if (ndim > max_allowed_ndim)
	throw std::runtime_error(std::string(where ? where : "mcpp_arrays") + ": attempt to create array with ndim > mcpp_arrays::max_allowed_ndim");
    return ndim;
}


// check_dtype, (T != void) version
template<typename T>
inline mcpp_typeid check_dtype(mcpp_typeid dtype, const char *where = nullptr)
{
    if (dtype == mcpp_type<T>::id)
	return dtype;

    std::stringstream ss;
    ss << (where ? where : "mcpp_arrays")
       << ": expected type " << mcpp_type<T>::id << " (" << mcpp_typestr(mcpp_type<T>::id) << ")"
       << ", got type " << dtype << " (" << mcpp_typestr(dtype) << ")";
    throw std::runtime_error(ss.str());
}

// check_dtype, (T == void) version
template<>
inline mcpp_typeid check_dtype<void> (mcpp_typeid dtype, const char *where)
{
    if (mcpp_typeid_is_valid(dtype))
	return dtype;

    std::stringstream ss;
    ss << (where ? where : "mcpp_arrays")
       << ": invalid mcpp_arrays::mcpp_typeid " << dtype;
    throw std::runtime_error(ss.str());	
}

inline void *aligned_malloc(ssize_t nbytes, bool zero=true, size_t nalign=128)
{
    if (nbytes <= 0)
	return nullptr;

    void *p = NULL;
    if (posix_memalign(&p, nalign, nbytes) != 0)
	throw std::runtime_error("couldn't allocate memory");

    if (zero)
	memset(p, 0, nbytes);

    return p;
}


template<typename T>
inline T *aligned_alloc(ssize_t nelts, bool zero=true, size_t nalign=128)
{
    return reinterpret_cast<T *> (aligned_malloc(nelts * sizeof(T), zero, nalign));
}


}  // namespace mcpp_arrays

#endif  // _MCPP_ARRAYS_CORE_HPP
