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
    T *data = nullptr;
    mcpp_typeid dtype = MCPP_INT8;

    int ndim = 0;
    int ncontig = 0;
    ssize_t *shape = nullptr;
    ssize_t *strides = nullptr;  // Note: strides can be negative!
    ssize_t itemsize = 0;
    ssize_t size = 0;

    std::shared_ptr<mcpp_array_reaper> reaper;

    static constexpr int ndim_inline = 6;
    ssize_t _inline_sbuf[2*ndim_inline];

    // "Untyped" constructor: allocates new, contiguous array of type T.
    rs_array(int ndim, const ssize_t *shape, bool zero=true, const char *where=nullptr);
    
    // "Typed" constructor: allocates new, contiguous array of type 'dtype'.
    rs_array(int ndim, const ssize_t *shape, mcpp_typeid dtype, bool zero=true, const char *where=nullptr);
    
    ~rs_array();

    // Copy/assignment operators.
    template<typename U> rs_array(const rs_array<U> &a, const char *where=nullptr);
    template<typename U> rs_array<T> &operator=(const rs_array<U> &a);

    // "Incomplete" constructor, allocates shape/stride arrays but does not
    // initialize them!  Must be followed by call to _finalize_shape_and_strides().    
    rs_array(int ndim, T *data, mcpp_typeid dtype,
	     const std::shared_ptr<mcpp_array_reaper> &reaper,
	     const char *where=nullptr);

    // Helper functions used in constructors.
    void _finalize_shape_and_strides(const char *where=nullptr);
    void _allocate(const ssize_t *shape_, bool zero);
};


// -------------------------------------------------------------------------------------------------
//
// Implementation


template<typename T>
rs_array<T>::rs_array(int ndim_, const ssize_t *shape_, bool zero, const char *where) :
    data(nullptr),
    dtype(mcpp_type<T>::id),
    ndim(check_ndim(ndim_, where)),
    ncontig(ndim_),
    shape((ndim_ <= ndim_inline) ? _inline_sbuf : new ssize_t[2*ndim_]),
    strides(shape + ndim_),
    itemsize(sizeof(T)),
    size(1)
{
    this->_allocate(shape_, zero);
}


template<typename T>
rs_array<T>::rs_array(int ndim_, const ssize_t *shape_, mcpp_typeid dtype_, bool zero, const char *where) :
    data(nullptr),
    dtype(check_dtype<T>(dtype_, where)),
    ndim(check_ndim(ndim_, where)),
    ncontig(ndim_),
    shape((ndim_ <= ndim_inline) ? _inline_sbuf : new ssize_t[2*ndim_]),
    strides(shape + ndim_),
    itemsize(mcpp_sizeof<T> (dtype_)),
    size(1)
{
    this->_allocate(shape_, zero);
}
    

template<typename T>
rs_array<T>::~rs_array()
{
    if (shape != _inline_sbuf)
	delete[] shape;

    shape = strides = nullptr;
}


template<typename T> template<typename U> 
rs_array<T>::rs_array(const rs_array<U> &a, const char *where) :
    data(a.data),
    dtype(check_dtype<T>(a.dtype, where)),
    ndim(a.ndim),
    ncontig(a.ncontig),
    shape((ndim <= ndim_inline) ? _inline_sbuf : new ssize_t[2*ndim]),
    strides(shape + ndim),
    itemsize(a.itemsize),
    size(a.size),
    reaper(a.reaper)
{
    for (int i = 0; i < ndim; i++) {
	shape[i] = a.shape[i];
	strides[i] = a.strides[i];
    }
}


template<typename T> template<typename U>
rs_array<T> &rs_array<T>::operator=(const rs_array<U> &a)
{
    // Self-assignment.
    if ((void *)this == (void *)&a)
	return *this;
	
    dtype(check_dtype<T>(a.dtype, "rs_array::operator="));
	
    if ((a.ndim > ndim_inline) && (a.ndim > ndim)) {
	if (shape != _inline_sbuf)
	    delete[] shape;
	shape = new ssize_t[2*a.ndim];
    }
	
    this->data = a.data;
    this->dtype = a.dtype;
    this->ndim = a.ndim;
    this->ncontig = a.ncontig;
    this->strides = this->shape + a.ndim;
    this->itemsize = a.itemize;
    this->size = a.size;
    this->reaper = a.reaper;

    for (int i = 0; i < ndim; i++) {
	shape[i] = a.shape[i];
	strides[i] = a.strides[i];
    }
}


// "Incomplete" constructor, allocates shape/stride arrays but does not
// initialize them!  Must be followed by call to _finalize_shape_and_strides().

template<typename T>
rs_array<T>::rs_array(int ndim_, T *data_, mcpp_typeid dtype_,
		      const std::shared_ptr<mcpp_array_reaper> &reaper_,
		      const char *where) :
    data(data_),
    dtype(check_dtype<T>(dtype_, where)),
    ndim(check_ndim(ndim_, where)),
    ncontig(0),
    shape((ndim_ <= ndim_inline) ? _inline_sbuf : new ssize_t[2*ndim_]),
    strides(shape + ndim_),
    itemsize(mcpp_sizeof<T> (dtype_)),
    size(0),
    reaper(reaper_)
{
    for (int i = 0; i < ndim_; i++)
	shape[i] = strides[i] = 0;
}


template<typename T>
void rs_array<T>::_allocate(const ssize_t *shape_, bool zero)
{
    size = 1;
    
    for (int i = ndim-1; i >= 0; i--) {
	this->shape[i] = shape_[i];
	this->strides[i] = size;
	this->size *= shape_[i];
    }

    this->data = reinterpret_cast<T *> (aligned_malloc(size * itemsize, zero));
    this->reaper = std::make_shared<malloc_reaper> (data);
}


template<typename T>
void rs_array<T>::_finalize_shape_and_strides(const char *where)
{
    size = 1;
    ncontig = ndim;

    for (int i = 0; i < ndim; i++) {
	ssize_t cstride = (i < ndim-1) ? (shape[i+1] * strides[i+1]) : 1;
	
	if (shape[i] < 0)
	    throw std::runtime_error(std::string(where ? where : "mcpp_arrays") + ": negative array dimension specified");
	if (std::abs(strides[i]) < std::abs(cstride))
	    throw std::runtime_error(std::string(where ? where : "mcpp_arrays") + ": specified stride is too small");
	if (strides[i] != cstride)
	    ncontig = ndim - i - 1;
	
	size *= shape[i];
    }
}



}  // namespace mcpp_arrays

#endif  // _MCPP_ARRAYS_RS_ARRAY_HPP
