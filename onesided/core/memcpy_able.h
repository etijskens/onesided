#ifndef MEMCPY_ABLE_H
#define MEMCPY_ABLE_H

#include <type_traits>
#include <vector>
#include <string>
#include <Eigen/Geometry>

typedef int64_t Index_t;

namespace mpi12s
{//-------------------------------------------------------------------------------------------------
   namespace internal
    {// This contains the machinery
     //-------------------------------------------------------------------------------------------------
     // move a void pointer ptr by nBytes bytes

        inline void
        advance_void_ptr
          ( void*& ptr      // pointer to be advanced
          , size_t nBytes   // in bytes
          ) {
            ptr = (char*)(ptr) + nBytes;
        }
     //-------------------------------------------------------------------------------------------------
        template<typename T>
        struct fixed_size_memcpy_able : std::is_trivially_copyable<T> {};
     //-------------------------------------------------------------------------------------------------
     // specializations:
        template<typename T>
        struct fixed_size_memcpy_able<T[]> : std::false_type {};

        template<typename T>
        struct fixed_size_memcpy_able<T*> : std::false_type {};

     // Specializations for vector types
        template<typename T, int N>
        struct fixed_size_memcpy_able<Eigen::Matrix<T,N,1,Eigen::DontAlign>> : std::true_type {};
     // TODO
     // Also provide specializations for Vector, Point, Quaternion, ...

     //-------------------------------------------------------------------------------------------------
        template<typename T>
        struct variable_size_memcpy_able : std::false_type {};
     //-------------------------------------------------------------------------------------------------
     // specializations:
        template<typename T>
        struct variable_size_memcpy_able<std::vector<T>> : fixed_size_memcpy_able<T> {};

        template<>
        struct variable_size_memcpy_able<std::string> : std::true_type {}; // C++17 required

     //-------------------------------------------------------------------------------------------------
        template<typename T>
        struct memcpy_traits
     //-------------------------------------------------------------------------------------------------
        {// C++17 required
            static bool const _debug_ = true;

         // Get a pointer to t's data.
            static 
            void*       // pointer to the beginning of the memory of the T object t 
            ptr
              ( T& t    // a T object t, either fixed_size_memcpy_able or variable_size_memcpy_able.
              )
            { 
                if constexpr(fixed_size_memcpy_able<T>::value)
                    return &t; 
                else if constexpr(variable_size_memcpy_able<T>::value)
                    return &t[0]; 
                else
                    static_assert(fixed_size_memcpy_able<T>::value || variable_size_memcpy_able<T>::value, "type T is not memcpy-able");
            }

         // Compute the size that t will occupy in the message. 
            static 
            size_t      // size that t will occupy in a message, in bytes.
            messageSize
              ( T& t    // a T object t, either fixed_size_memcpy_able or variable_size_memcpy_able.
              ) 
            {
                if constexpr(fixed_size_memcpy_able<T>::value) 
                {// the size of a single T
                    return sizeof(T);
                }
                else if constexpr(variable_size_memcpy_able<T>::value)
                {// the size of siz_t + the size of a single T::value_type times the number of items in the collection
                    return sizeof(size_t) + sizeof(typename T::value_type) * t.size();
                }
                else
                    static_assert(fixed_size_memcpy_able<T>::value || variable_size_memcpy_able<T>::value, "type T is not memcpy-able");
            }

         // write a T to a buffer
            static void write
              ( T& t        // a T object t to be written to a message buffer, either fixed_size_memcpy_able or variable_size_memcpy_able.
              , void*& dst  // pointer in a message buffer where t will be written. On return the pointer is advanced past
                            // the end of what was written, so the next object can be written to that position.
              ) 
            {

                if constexpr(fixed_size_memcpy_able<T>::value)
                {
                    if constexpr(::mpi12s::_debug_ && _debug_)
                        prdbg( tostr("fixed_size_memcpy_able<T=", typeid(T).name(), ">::write(t, dst=", dst, ")") );
                 // write the variable t
                    auto nBytes = sizeof(t);
                    memcpy( dst, ptr(t), nBytes );
                 // advance the pointer in the buffer
                    advance_void_ptr(dst, nBytes);
                    if constexpr(::mpi12s::_debug_ && _debug_)
                        prdbg( tostr("fixed_size_memcpy_able<T=", typeid(T).name(), ">::write(t, dst=", dst, "), bytes written=sizeof(size_t)", nBytes) );
                }
                else if constexpr(variable_size_memcpy_able<T>::value)
                {
                    if constexpr(::mpi12s::_debug_ && _debug_)
                        prdbg( tostr("variable_size_memcpy_able<T=", typeid(T).name(), ">::read(t, dst=", dst
                                    , ") value_type=", typeid(typename T::value_type).name()
                                    )
                             );

                 // write the size of the collection:
                    size_t size = t.size();
                    auto nBytes = sizeof(size_t);
                    memcpy( dst, &size, nBytes );
                 // advance the pointer in the buffer
                    advance_void_ptr(dst, nBytes);
                    if constexpr(::mpi12s::_debug_ && _debug_)
                        prdbg( tostr("variable_size_memcpy_able<T=", typeid(T).name(), ">::write(t, dst=", dst
                                    , "), bytes read=sizeof(size_t)=", nBytes
                                    )
                             );

                 // write the collection:
                    nBytes = size * sizeof(typename T::value_type);
                    memcpy( dst, &t[0], nBytes );
                 // advance the pointer in the buffer
                    advance_void_ptr(dst, nBytes);
                    if constexpr(::mpi12s::_debug_ && _debug_)
                        prdbg( tostr("variable_size_memcpy_able<T=", typeid(T).name(), ">::write(t, dst=", dst
                                    , "), bytes read=", size, "*", sizeof(typename T::value_type), "=", nBytes
                                    )
                             );
                }
                else
                    static_assert(fixed_size_memcpy_able<T>::value || variable_size_memcpy_able<T>::value, "type T is not memcpy-able");
            }

        // read a T from a buffer
            static void read
              ( T& t        // a T object t to be read from a message buffer, either fixed_size_memcpy_able or variable_size_memcpy_able.
              , void*& src  // pointer in a message buffer where t will be read from. On return the pointer is advanced past
                            // the end of what was read, so the next object can be read from that position.
              ) 
            {
                if constexpr(fixed_size_memcpy_able<T>::value)
                {
                    if constexpr(::mpi12s::_debug_ && _debug_)
                        prdbg( tostr("fixed_size_memcpy_able<T=", typeid(T).name(), ">::read(t, src=", src, ")") );
                 // read the variable t
                    auto nBytes = sizeof(t);
                    memcpy( ptr(t), src, nBytes );
                 // advance the pointer in the buffer
                    advance_void_ptr(src, nBytes);
                    if constexpr(::mpi12s::_debug_ && _debug_)
                        prdbg( tostr("fixed_size_memcpy_able<T=", typeid(T).name(), ">::read(t, src=", src, "), bytes read=sizeof(size_t)", nBytes) );
                }
                else if constexpr(variable_size_memcpy_able<T>::value)
                {
                    if constexpr(::mpi12s::_debug_ && _debug_)
                        prdbg( tostr("variable_size_memcpy_able<T=", typeid(T).name(), ">::read(t, src=", src
                                    , ") value_type=", typeid(typename T::value_type).name()
                                    )
                             );
                 // read the size of the collection:
                    auto nBytes = sizeof(size_t);
                    size_t size;
                    memcpy( &size, src, nBytes );
                 // advance the pointer in the buffer
                    advance_void_ptr(src, nBytes);
                    if constexpr(::mpi12s::_debug_ && _debug_)
                        prdbg( tostr("variable_size_memcpy_able<T=", typeid(T).name(), ">::read(t, src=", src
                                    , "), bytes read=sizeof(size_t)=", nBytes
                                    )
                             );

                 // resize the collection
                    t.resize(size);
                 // read the collection:
                    nBytes = size * sizeof(typename T::value_type);
                    memcpy( &t[0], src, nBytes );
                 // advance the pointer in the buffer
                    advance_void_ptr(src, nBytes);
                    if constexpr(::mpi12s::_debug_ && _debug_)
                        prdbg( tostr("variable_size_memcpy_able<T=", typeid(T).name(), ">::read(t, src=", src
                                    , "), bytes read=", size, "*", sizeof(typename T::value_type), "=", nBytes
                                    )
                             );
}
                else
                    static_assert(fixed_size_memcpy_able<T>::value || variable_size_memcpy_able<T>::value, "type T is not memcpy-able");
            }
        };
     //-------------------------------------------------------------------------------------------------
    }// namespace internal 

 //-------------------------------------------------------------------------------------------------
 // The three template functions that the outside world should use for reading and writing messages.
 //------------------------------------------------------------------------------------------------- 
    template <typename T>
    void write
      ( T& t       // the T object that will be read from a message buffer,
                   // T must be either fixed_size_memcpy_able or variable_size_memcpy_able
      , void*& dst // pointer in the message buffer where t will be written to.
                   // On return the pointer is advanced to the position where the next
                   // object will be written, i.e. just behind t in the message buffer.
      )
    {
            internal::memcpy_traits<T>::write(t,dst);
    }

    template <typename T>
    void read
      ( T& t       // the T object that will be read from a message buffer,
                   // T must be either fixed_size_memcpy_able or variable_size_memcpy_able
      , void*& src // pointer in the message buffer where t will be read from.
                   // On return the pointer is advanced to the begin of the next object in 
                   // the message buffer, i.e. just behind t in the message buffer.
      )
    {
        internal::memcpy_traits<T>::read(t,src);
    }

    template <typename T>
    size_t      // returns the size that the T object t will occupy in a message
    messageSize
      (  T& t   // a T object, either fixed_size_memcpy_able or variable_size_memcpy_able
      ) 
    {
        return internal::memcpy_traits<T>::messageSize(t);
    }
 //-------------------------------------------------------------------------------------------------
}// namespace mpi12s

#endif // MEMCPY_ABLE_H
