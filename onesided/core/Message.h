#ifndef MESSAGE_H
#define MESSAGE_H

#include "memcpy_able.h"
#include <string>
#include <iostream>
#include <sstream>

typedef int64_t Index_t;

namespace mpi
{// Although nothing in this file uses MPI, it is necessary machinery for the MPI messageing 
 // system that we need
 //-------------------------------------------------------------------------------------------------
 // Values for specializations
 //-------------------------------------------------------------------------------------------------
    enum
      { not_implemented =-1
      , built_in_type
      , array_like_type
      };

 //-------------------------------------------------------------------------------------------------
 // Compile-time detection of types T that are 
 // (1) contiguous and of fixed size: so its size is determined with sizeof(T) or sizeof(t) and 
 //     memcpy-ing a T copies its entire state.
 //     E.g. : int, double, float, .., but also float[3], double[4], ... 
 //     = fixed_length_memcpy_able
 // (2) a contiguous collection of type (1) objects, whose  size can be determined with a size()
 //     function. The collection type C must provide:
 //       - size_t size() member function
 //       - value_type typedef
 //       - value_type* data(), pointing to the beginning of the collection 
 //     std::vector<T> and std::string satisfy these requirements 
 //     = variable_length_memcpy_able
 // https://akrzemi1.wordpress.com/2017/12/02/your-own-type-predicate/
 // https://stackoverflow.com/questions/33067983/type-traits-contiguous-memory


 //-------------------------------------------------------------------------------------------------
    template<typename T> struct handler_kind
 //-------------------------------------------------------------------------------------------------
    {
        static int const value = not_implemented;
    };

 // handler_kind specializetions. For the time being, we must be exhaustive
    template<> struct handler_kind<int>     { static int const value = built_in_type; };
    template<> struct handler_kind<Index_t> { static int const value = built_in_type; };
    template<> struct handler_kind<float>   { static int const value = built_in_type; };
    template<> struct handler_kind<double>  { static int const value = built_in_type; };

    template<> struct handler_kind<std::vector<int>>     { static int const value = array_like_type; };
    template<> struct handler_kind<std::vector<Index_t>> { static int const value = array_like_type; };
    template<> struct handler_kind<std::vector<float>>   { static int const value = array_like_type; };
    template<> struct handler_kind<std::vector<double>>  { static int const value = array_like_type; };
    
    template<> struct handler_kind<std::string>          { static int const value = array_like_type; };// C++17 required

 //------------------------------------------------------------------------------------------------
 // Convert a size in bytes, ensuring that the result is rounded up to Boundary. The result is
 // expressed in word of Unit bytes. E.g. 
 //     convertSizeInBytes<8>(4)   -> 8 : the smallest 8 byte boundary >= 4 bytes is 8 bytes
 //     convertSizeInBytes<8,8>(4) -> 8 : the smallest 8 byte boundary >= 4 bytes is 1 8-byte word
 //     convertSizeInBytes<8,2>(4) -> 8 : the smallest 8 byte boundary >= 4 bytes is 4 2-byte words
    template<size_t Boundary, size_t Unit=1>
    Index_t convertSizeInBytes(Index_t bytes) {
        return ((bytes + Boundary - 1) / Boundary) * (Boundary / Unit);
    }


 //------------------------------------------------------------------------------------------------
 // A Helper class for writing message items to a buffer (void*) and reading them back in.
 // Their behavior is governed by handler_kind<T>::value.
 // Only specializations are functional
 //------------------------------------------------------------------------------------------------
    template<typename T, int> struct MessageItemHelper {};
 //------------------------------------------------------------------------------------------------
 // specialization for primitive types
 //------------------------------------------------------------------------------------------------
    template<typename T> 
    struct MessageItemHelper<T, built_in_type>
    {// Write t to buffer ptr. After writing the ptr is modified to the next position to write to.
        static void write( T const& t, void * & ptr ) {
            size_t sz = sizeof(t);
            memcpy(ptr, &t, sz);
            ptr = static_cast<char*>(ptr) + sz;
        }

     // Read t from buffer ptr. After reading the ptr is modified to the next position to read from.
        static void read( T& t, void * & ptr ) {
            size_t sz = sizeof(t);
            memcpy(&t, ptr, sz);
            ptr = static_cast<char*>(ptr) + sz;
        }

     // Compute the size t will occupy in a message, in bytes.
        static size_t size(T const& t) { 
            return sizeof(t);
        }

     // Convert t to string
        static std::string toStr(T const& t) {
            return std::to_string(t);
        }
    };

 //------------------------------------------------------------------------------------------------
 // specialization for std::vectors, and the like
 //------------------------------------------------------------------------------------------------
    template<typename T> 
    struct MessageItemHelper<T, array_like_type>
    {// Write t's contents to buffer ptr. After writing the ptr is modified to the next position to write to.
        static void write( T const& t, void * & ptr )
        {// we must also write the size of the array, so that on reading we know what to expect
            size_t sz = t.size();    
            // std::cout<<"writing size "<<sz<<std::endl;
            memcpy(ptr, &sz, sizeof(size_t));
            ptr = static_cast<char*>(ptr) + sizeof(size_t);
            sz *= sizeof(typename T::value_type);
            memcpy(ptr, t.data(), sz);
            ptr = static_cast<char*>(ptr) + sz;
        }

     // Read t from buffer ptr. After reading the ptr is modified to the next position to read from.
        static void read( T& t, void * & ptr ) {
            size_t sz;
            memcpy(&sz, ptr, sizeof(size_t));
            std::cout<<"size read "<<sz<<std::endl;
            t.resize(sz);
            ptr = static_cast<char*>(ptr) + sizeof(size_t);
            sz *= sizeof(typename T::value_type);
            memcpy(t.data(), ptr, sz);
            ptr = static_cast<char*>(ptr) + sz;
        }

     // Compute the size t will occupy in a message, in bytes.
        static size_t size(T const& t) {
            return sizeof(size_t) + t.size() * sizeof(typename T::value_type);
        }
 
     // Convert t to string
        static std::string toStr(T const& t) {
            std::stringstream ss;
            ss<<"[ ";
            for( auto ti : t) {
                ss<<ti<<' ';
            }
            ss<<']';
            return ss.str();
        }
    };

 //-------------------------------------------------------------------------------------------------
    class MessageItemBase
 //-------------------------------------------------------------------------------------------------
        {
    public:
        virtual ~MessageItemBase() {}
    // write the message item to ptr
        virtual void write(void*& ptr) const = 0;
    // reade the message item from ptr
        virtual void read (void*& ptr)       = 0;
    // get the size of the message item (in bytes)
        virtual size_t size() const = 0;
    // convert the message item to an intelligible string (for debugging and testing mainly)
        virtual std::string toStr() const = 0;
    };

 //-------------------------------------------------------------------------------------------------
    template <typename T>
    class MessageItem : public MessageItemBase
 //-------------------------------------------------------------------------------------------------
    {
    public:
        MessageItem(T& t) : ptrT_(&t) {}

        virtual void write(void*& ptr) const {
            MessageItemHelper<T,handler_kind<T>::value>::write( *ptrT_, ptr );
        }
        virtual void read(void*& ptr) {
            MessageItemHelper<T,handler_kind<T>::value>::read( *ptrT_, ptr );
        }
        virtual size_t size() const {
            return MessageItemHelper<T,handler_kind<T>::value>::size(*ptrT_);
        }
        virtual std::string toStr() const {
            return MessageItemHelper<T,handler_kind<T>::value>::toStr( *ptrT_ );
        }
    private:
        T* ptrT_;
    };

 //-------------------------------------------------------------------------------------------------
    class Message
 //-------------------------------------------------------------------------------------------------
    {
    public:
        ~Message();

    // Add an item to the message (behaves as FIFO)
        template<typename T> 
        void push_back(T& t)
        {
            MessageItem<T>* p = new MessageItem<T>(t);
            coll_.push_back(p);
        }

    // Write the message to ptr (=buffer)
        void write(void*& ptr) const;
        
    // Read the message from ptr (=buffer)
        void read (void*& ptr);

    // Compute the size of the message, in bytes.
        size_t size() const;

    // Construct an intelligible string with the message items:
        std::string toStr() const;

    private:
        std::vector<MessageItemBase*> coll_;
    };
 //-------------------------------------------------------------------------------------------------
}// namespace mpi

#endif // MESSAGE_H
