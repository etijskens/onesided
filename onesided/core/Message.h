#ifndef MESSAGE_H
#define MESSAGE_H

#include "memcpy_able.h"
#include <string>
#include <iostream>
#include <sstream>

namespace mpi
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

        ~MessageItem()
        {
            if constexpr(verbose) std::cout<<"\n  rank"<<::mpi::my_rank<<" destroying MessageItem<T> "<<typeid(T).name()<<" @ "<<this;
        }

        virtual void write(void*& ptr) const {
            ::mpi::write( *ptrT_, ptr );
        }
        virtual void read(void*& ptr) {
            ::mpi::read( *ptrT_, ptr );
        }
     // Size that *ptrT_ will occupy in a message, in bytes
        virtual size_t messageSize() const {
            return ::mpi::messageSize(*ptrT_);
        }
        virtual std::string toStr() const
        {
            std::stringstream ss;
            if constexpr(internal::fixed_size_memcpy_able<T>::value) {
                ss<<'['<<*ptrT_<<']';
            }
            else if constexpr(internal::variable_size_memcpy_able<T>::value) {
                size_t sz = ptrT_->size();
                ss<<'('<<sz<<')'<<"[ ";
                for( auto v : *ptrT_ ) {
                    ss<<v<<' ';
                }   ss<<']';
            }
            else 
                static_assert
                  ( internal::   fixed_size_memcpy_able<T>::value ||
                    internal::variable_size_memcpy_able<T>::value
                  , "T is not memcpy-able."
                  );
            return ss.str();
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
        size_t messageSize() const;

    // Construct an intelligible string with the message items:
        std::string toStr() const;

    private:
        std::vector<MessageItemBase*> coll_;
    };
 //-------------------------------------------------------------------------------------------------
}// namespace mpi

#endif // MESSAGE_H
