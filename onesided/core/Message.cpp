#include "Message.h"

namespace mpi1s
{
 //-------------------------------------------------------------------------------------------------
 // Implementation of class Message
 //-------------------------------------------------------------------------------------------------
    Message::
    ~Message()
    {
        if constexpr(verbose) std::cout<<'\n'<<::mpi1s::info<<"~Message() deleting MessageItems: {"<<std::endl;
        for( auto p : coll_) {
            delete p;
        }
        if constexpr(verbose) std::cout<<info<<"~Message() }"<<std::endl;
    }

    void
    Message::
    write(void*& ptr) const
    {
        MessageItemBase * const * pBegin = &coll_[0];
        MessageItemBase * const * pEnd   = pBegin + coll_.size();
        for( MessageItemBase * const * p = pBegin; p < pEnd; ++p) {
            (*p)->write(ptr);
        }
      #ifdef VERBOSE
        std::cout<<"Message::write(ptr)"<<std::endl;
      #endif
    }

    void
    Message::
    read(void*& ptr)
    {
        MessageItemBase ** pBegin = &coll_[0];
        MessageItemBase ** pEnd   = pBegin + coll_.size();
        for( MessageItemBase ** p = pBegin; p < pEnd; ++p) {
            (*p)->read(ptr);
        }
      #ifdef VERBOSE
        std::cout<<"Message::read(ptr)"<<std::endl;
      #endif
    }

    size_t
    Message::
    messageSize() const
    {
        size_t sz = 0;
        MessageItemBase * const * pBegin = &coll_[0];
        MessageItemBase * const * pEnd   = pBegin + coll_.size();
        for( MessageItemBase * const * p = pBegin; p < pEnd; ++p) {
            sz += (*p)->messageSize();
        }
        return sz;
    }

    std::string
    Message::
    toStr() const 
    {
        std::stringstream ss;
        size_t i = 0;
        MessageItemBase * const * pBegin = &coll_[0];
        MessageItemBase * const * pEnd   = pBegin + coll_.size();
        for( MessageItemBase * const * p = pBegin; p < pEnd; ++p) {
            ss<<'\n'<<i++<<' '<<(*p)->toStr();
        }
        return ss.str();
    }
 //-------------------------------------------------------------------------------------------------
}// namespace mpi1s