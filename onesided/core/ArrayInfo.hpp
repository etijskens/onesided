#include <stdexcept>
#include <string>


 //-----------------------------------------------------------------------------
    template<class T, size_t NDIM> 
    class ArrayInfo
 // wrapper for py::array_t<T> to facilitate array validation and data access
 //-----------------------------------------------------------------------------
    {
    // py::array_t<T> a_;
        size_t shape_[NDIM];
        T * ptrT_; 

    public:
    //-----------------------------------------------------------------------------
    // ctor
        ArrayInfo(py::array_t<T> a)
        // : a_(a)
        {
            auto buf = a.request();
            
            if( buf.ndim != NDIM ) {
                std::string msg = "Wrong number of dimensions: " + std::to_string(buf.ndim) + std::string(", expecting ") + std::to_string(NDIM);
                throw std::runtime_error(msg);
            }

            for( size_t i = 0; i<NDIM; ++i) {
                this->shape_[i] = buf.shape[i];
            }
            this->ptrT_ = static_cast<T *>(buf.ptr);
        }
        
    //-----------------------------------------------------------------------------
    // Return non-const pointer to the array's data
        T* data() {
            return ptrT_;
        }
        
    //-----------------------------------------------------------------------------
    // Return const pointer to the array's data
        T const* cdata() {
            return const_cast<T const *>(ptrT_);
        }

    //-----------------------------------------------------------------------------
    // Return the number of dimensions of the shape
        size_t ndim() const {
            return NDIM;
        }

    //-----------------------------------------------------------------------------
    // return the length of the i-th array dimension
        size_t shape(size_t i) const {
            return this->shape_[i];
        }

    //-----------------------------------------------------------------------------
    // Throw runtime_error if b does not have the same shape as *this   
        template<class B>
        void assert_identical_shape(B const& b)
        {
            size_t ndim = this->ndim();
            if( ndim != b.ndim() ) {
                std::string msg = "Unequal array dimension: " + std::to_string(ndim) + std::string(" <> ") + std::to_string(b.ndim());
                throw std::runtime_error(msg);
            }
            for( size_t i=0; i<ndim; ++i ) {
                if( shape_[i] != b.shape(i) ) {
                    std::string msg = "Unequal shape: [" + std::to_string(shape_[0]);
                    for( size_t j=1; j<ndim; ++j)
                        msg += std::string(", ") + std::to_string( shape_[j]);
                    msg += std::string("] <> [") + std::to_string(b.shape(0));
                    for( size_t j=1; j<ndim; ++j)
                        msg += std::string(", ") + std::to_string(b.shape(j));
                    msg += std::string("].");
                    throw std::runtime_error(msg);
                }
            }
        }
     //-----------------------------------------------------------------------------
     // subscripting
        inline
        T&
        operator[](std::size_t idx) {
            return ptrT_[idx];
        }
     //-----------------------------------------------------------------------------
     // subscripting
        inline
        T const&
        operator[](std::size_t idx) const {
            return ptrT_[idx];
        }

    };
 //-----------------------------------------------------------------------------
