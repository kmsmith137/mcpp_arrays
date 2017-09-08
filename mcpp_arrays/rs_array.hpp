#ifndef _MCPP_ARRAYS_RS_ARRAY_HPP
#define _MCPP_ARRAYS_RS_ARRAY_HPP

#include "core.hpp"

namespace mcpp_arrays {
#if 0
}  // pacify emacs c-mode
#endif


// Note: rs_array<void> is allowed, and means that the 'dtype' field
// encodes the data type.

template<typename T>
struct rs_array {
    T *data;
    mcpp_typeid dtype;

    int ndim;
    int ncontig;
    ssize_t *shape = nullptr;
    ssize_t *strides = nullptr;  // Note: strides can be negative!
    ssize_t itemsize;
    ssize_t size;

    static constexpr int ndim_inline = 6;
    ssize_t _inline_sbuf[2*ndim_inline];

    // Holds reference to allocated memory.
    // Note that 'data' need not be equal to ref.get(), if this is a subarray.
    std::shared_ptr<void> ref;

    // Note: the list of constructors below is incomplete, I'll revisit systematically later!

    // Default constructor: creates a zero-dimensional array.
    rs_array();

    // Allocates new, contiguous array of type T (only works if T is non-void).
    rs_array(int ndim, const ssize_t *shape, bool zero=true, const char *where=nullptr);

    // Allocates new, contiguous array of type 'dtype' (works if T=void).
    // Throws an exception if 'dtype' is inconsistent with the template argument T.
    rs_array(int ndim, const ssize_t *shape, mcpp_typeid dtype, bool zero=true, const char *where=nullptr);

    // Construct rs_array from existing data (only works if U is non-void).
    // Throws an exception if T and U are inconsistent.
    template<typename U> rs_array(U *data, int ndim, const ssize_t *shape, const ssize_t *strides, const std::shared_ptr<void> &ref, const char *where=nullptr);

    // Construct rs_array from existing data (works if U is void).
    // This constructor assumes that U and 'dtype' are consistent!
    // Throws an exception if T and dtype are inconsistent.
    template<typename U> rs_array(U *data, int ndim, const ssize_t *shape, const ssize_t *strides, mcpp_typeid dtype, const std::shared_ptr<void> &ref, const char *where=nullptr);

    // Copy/assignment operators.
    template<typename U> rs_array(const rs_array<U> &a, const char *where=nullptr);
    template<typename U> rs_array<T> &operator=(const rs_array<U> &a);

    // Need to explicitly delegate copy constructor to templated version above.
    rs_array(rs_array<T> &a) : rs_array(a,nullptr) { }
    
    ~rs_array();

    // Helper functions used in constructors.

    inline void _allocate_shape(int ndim_, const char *where);
    inline void _allocate(int ndim_, const ssize_t *shape_, bool zero, const char *where);
    inline void _construct_from_data(T *data, int ndim_, const ssize_t *shape_, const ssize_t *strides_, const std::shared_ptr<void> &ref, const char *where);

    template<typename U> inline void _construct_from_array(const rs_array<U> &a, const char *where);
};


// -------------------------------------------------------------------------------------------------
//
// Implementation


// Default constructor: allocates zero-dimensional array, by delegating to the "N-dimensional" constructor.
template<typename T>
rs_array<T>::rs_array() :
    rs_array(0, nullptr, true)
{ }


// Allocates new, contiguous array of type T (only works if T is non-void).
template<typename T>
rs_array<T>::rs_array(int ndim_, const ssize_t *shape_, bool zero, const char *where)
{
    this->dtype = mcpp_type<T>::id;
    this->itemsize = sizeof(T);
    this->_allocate(ndim_, shape_, zero, where);  // initializes all fields except dtype, itemsize
}


// Allocates new, contiguous array of type 'dtype' (works if T=void).
// Throws an exception if 'dtype' is inconsistent with the template argument T.
template<typename T>
rs_array<T>::rs_array(int ndim_, const ssize_t *shape_, mcpp_typeid dtype_, bool zero, const char *where)
{
    this->dtype = _set_dtype<T> (dtype_, where);
    this->itemsize = _set_itemsize<T> (dtype_);
    this->_allocate(ndim_, shape_, zero, where);  // initializes all fields except dtype, itemsize
}


// Construct rs_array from existing data (only works if U is non-void).
// Throws an exception if T and U are inconsistent.
template<typename T> template<typename U>
rs_array<T>::rs_array(U *data_, int ndim_, const ssize_t *shape_, const ssize_t *strides_, const std::shared_ptr<void> &ref_, const char *where)
{
    // Note: could speed things up by a few cycles here, with more compile-time magic.
    this->dtype = _set_dtype<T> (mcpp_type<U>::id);
    this->itemsize = sizeof(U);
    this->_construct_from_data((T *) data_, ndim_, shape_, strides_, ref_, where);
}


// Construct rs_array from existing data.
// This constructor assumes that 'data' points to an array of type 'dtype'.
// Throws an exception if the specified dtype is inconsistent with the template argument T.
template<typename T> template<typename U>
rs_array<T>::rs_array(U *data_, int ndim_, const ssize_t *shape_, const ssize_t *strides_, mcpp_typeid dtype_, const std::shared_ptr<void> &ref_, const char *where)
{
    // Note: could speed things up by a few cycles here, with more compile-time magic.
    this->dtype = _set_dtype<T> (dtype_, where);
    this->itemsize = _set_itemsize<T> (dtype_);
    this->_construct_from_data((T *) data_, ndim_, shape_, strides_, ref_, where);
}


// Copy constructor
template<typename T> template<typename U> 
rs_array<T>::rs_array(const rs_array<U> &a, const char *where)
{
    // Initializes 'ndim'.  Allocates 'shape' and 'strides', but does not initialize them.
    this->_allocate_shape(a.ndim, where);

    // _construct_from_array() takes care of the remaining initializations.
    this->_construct_from_array(a, where);
}


template<typename T> template<typename U>
rs_array<T> &rs_array<T>::operator=(const rs_array<U> &a)
{
    // If self-assignment, do nothing.
    if ((void *)this == (void *)&a)
	return *this;

    // Make sure shape, strides are correctly allocated (assumed by _construct_from_array()).
    if ((a.ndim > ndim_inline) && (a.ndim > ndim)) {
	if (shape != _inline_sbuf)
	    delete[] shape;
	shape = new ssize_t[2*a.ndim];
	strides = shape + a.ndim;
    }

    // _construct_from_array() also assumes that 'ndim' has been initialized.
    this->ndim = a.ndim_;

    // _construct_from_array() takes care of the remaining initializations.
    this->_construct_from_array(a, nullptr);
    return *this;
}


template<typename T>
rs_array<T>::~rs_array()
{
    if (shape != _inline_sbuf)
	delete[] shape;
    
    shape = strides = nullptr;
    data = nullptr;
}


// Helper function called by rs_array constructors.
// Initializes 'ndim'.  Allocates 'shape' and 'strides', but does not initialize them.
template<typename T>
inline void rs_array<T>::_allocate_shape(int ndim_, const char *where)
{
    if (ndim_ < 0)
	throw std::runtime_error(std::string(where ? where : "mcpp_arrays") + ": attempt to create array with ndim < 0");
    if (ndim_ > max_allowed_ndim)
	throw std::runtime_error(std::string(where ? where : "mcpp_arrays") + ": attempt to create array with ndim > mcpp_arrays::max_allowed_ndim");

    this->ndim = ndim_;
    this->shape = (ndim_ <= ndim_inline) ? _inline_sbuf : new ssize_t[2*ndim_];
    this->strides = shape + ndim_;
}


// Helper function called by rs_array constructors.
// Assumes 'dtype', 'itemsize' have been initialized.  Initializes all remaining fields.
template<typename T>
inline void rs_array<T>::_allocate(int ndim_, const ssize_t *shape_, bool zero, const char *where)
{
    // Initializes 'ndim'.  Allocates 'shape' and 'strides', but does not initialize them.
    this->_allocate_shape(ndim_, where);

    this->ncontig = ndim;
    this->size = 1;
    
    for (int i = ndim-1; i >= 0; i--) {
	this->shape[i] = shape_[i];
	this->strides[i] = size;
	this->size *= shape_[i];
    }

    this->data = reinterpret_cast<T *> (aligned_malloc(size * itemsize, zero));
    this->ref = std::shared_ptr<void> (data, free);
}


// Helper function called by rs_array constructors.
// Assumes 'dtype', 'itemsize' have been initialized.  Initializes all remaining fields.
template<typename T>
inline void rs_array<T>::_construct_from_data(T *data_, int ndim_, const ssize_t *shape_, const ssize_t *strides_, const std::shared_ptr<void> &ref_, const char *where)
{
    // Initializes 'ndim'.  Allocates 'shape' and 'strides', but does not initialize them.
    this->_allocate_shape(ndim_, where);

    this->data= data_;
    this->ref = ref_;
    this->ncontig = ndim;
    this->size = 1;

    for (int i = 0; i < ndim; i++) {
	shape[i] = shape_[i];
	strides[i] = strides_[i];
	
	if (shape[i] < 0)
	    throw std::runtime_error(std::string(where ? where : "mcpp_arrays") + ": negative array dimension specified");

	ssize_t cstride = (i < ndim-1) ? (shape[i+1] * strides[i+1]) : 1;
	if (strides[i] != cstride)
	    ncontig = ndim - i - 1;
	
	size *= shape[i];
    }
}


// Helper function called by rs_array constructors.
// Assumes 'ndim' has been initialized, and 'shape' and 'strides' have been allocated (but not initialized).
// Remaining initializations are done here, including throwing an exception if there is a type mismatch.
template<typename T> template<typename U>
inline void rs_array<T>::_construct_from_array(const rs_array<U> &a, const char *where)
{
    this->dtype = _set_dtype<T> (a.dtype);   // throws exception on type mismatch
    this->data = (T *) a.data;
    this->ncontig = a.ncontig;
    this->itemsize = a.itemsize;
    this->size = a.size;
    this->ref = a.ref;

    for (int i = 0; i < ndim; i++) {
	shape[i] = a.shape[i];
	strides[i] = a.strides[i];
    }
}


}  // namespace mcpp_arrays

#endif  // _MCPP_ARRAYS_RS_ARRAY_HPP
