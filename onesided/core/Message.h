#ifndef MESSAGE_H
#define MESSAGE_H

#include "memcpy_able.h"
#include <string>
#include <iostream>
#include <sstream>

namespace mpi12s
{// Although nothing in this file uses MPI, it is necessary machinery for the MPI messageing 
 // system that we need
 
 //-------------------------------------------------------------------------------------------------
 // Convert a size in bytes, ensuring that the result is rounded up to Boundary. The result is
 // expressed in word of Unit bytes. E.g. 
 //     convertSizeInBytes<8>(4)   -> 8 : the smallest 8 byte boundary >= 4 bytes is 8 bytes
 //     convertSizeInBytes<8,8>(4) -> 8 : the smallest 8 byte boundary >= 4 bytes is 1 8-byte word
 //     convertSizeInBytes<8,2>(4) -> 8 : the smallest 8 byte boundary >= 4 bytes is 4 2-byte words
    template<size_t Boundary, size_t Unit=1>
    Index_t convertSizeInBytes(Index_t bytes) {
        return ((bytes + Boundary - 1) / Boundary) * (Boundary / Unit);
    }

 
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
        virtual size_t messageSize() const = 0;
    // convert the message item to an intelligible list of strings (for debugging and testing mainly)
        virtual Lines_t debug_text() const = 0;
    };

 //-------------------------------------------------------------------------------------------------
    template <typename T>
    class MessageItem : public MessageItemBase
 //-------------------------------------------------------------------------------------------------
    {
        static const bool _debug_ = true; // write debug output or not

    public:
        MessageItem(T& t) : ptrT_(&t) {}

        ~MessageItem()
        {
            if constexpr(::mpi12s::_debug_ && _debug_) {
                prdbg(tostr("~MessageItem<T=", typeid(T).name(), ">() : this=", this));
            }
        }

        virtual void write(void*& ptr) const {
            if constexpr(::mpi12s::_debug_ && _debug_) {
                prdbg( tostr("MessageItem<T=", typeid(T).name(), ">::write()"), this->debug_text() );
            }
            ::mpi12s::write( *ptrT_, ptr );
        }

        virtual void read(void*& ptr) {
            ::mpi12s::read( *ptrT_, ptr );
            if constexpr(::mpi12s::_debug_ && _debug_) {
                prdbg( tostr("MessageItem<T=", typeid(T).name(), ">::read()"), this->debug_text() );
            }
        }
     // Size that *ptrT_ will occupy in a message, in bytes
        virtual size_t messageSize() const {
            return ::mpi12s::messageSize(*ptrT_);
        }

     // Content of the message
        virtual
        Lines_t // return a list of lines.
        debug_text() const
        {
            Lines_t lines;
            std::stringstream ss;
            if constexpr(internal::fixed_size_memcpy_able<T>::value) {
                ss<<'['<<*ptrT_<<']';
                lines.push_back(ss.str());
            }
            else if constexpr(internal::variable_size_memcpy_able<T>::value) {
                size_t sz = ptrT_->size();
                ss<<"(size="<<sz<<") [";
                lines.push_back(ss.str()); ss.str(std::string());
                for( size_t i = 0; i < sz ; ++i ) {
                    ss<<std::setw(10)<<i
                      <<std::setw(20)<<(*ptrT_)[i];
                    lines.push_back(ss.str()); ss.str(std::string());
                }   ss<<']';
                lines.push_back(ss.str()); ss.str(std::string());
            }
            else 
                static_assert
                  ( internal::   fixed_size_memcpy_able<T>::value ||
                    internal::variable_size_memcpy_able<T>::value
                  , "T is not memcpy-able."
                  );
            return lines;
        }
    private:
        T* ptrT_;
    };

 //-------------------------------------------------------------------------------------------------
    class Message
 //-------------------------------------------------------------------------------------------------
    {
        static bool const _debug_ = true;
    public:
        ~Message();

    // Add an item to the message (behaves as FIFO)
        template<typename T> 
        void push_back(T& t)
        {
            MessageItem<T>* p = new MessageItem<T>(t);
            coll_.push_back(p);
        }

    // Write the message to ptr (=buffer)
        void write(void*& ptr) const;
        
    // Read the message from ptr (=buffer)
        void read (void*& ptr);

    // Compute the size of the message, in bytes.
        size_t messageSize() const;

    // Construct an intelligible string with the message items:
        ::mpi12s::Lines_t debug_text() const;

    private:
        std::vector<MessageItemBase*> coll_;
    };
 //-------------------------------------------------------------------------------------------------
}// namespace mpi12s

#endif // MESSAGE_H
