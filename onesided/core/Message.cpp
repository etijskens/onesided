#include "Message.h"

namespace mpi
{
 //-------------------------------------------------------------------------------------------------
 // Implementation of class Message
 //-------------------------------------------------------------------------------------------------
    Message::
    ~Message()
    {
        MessageItemBase** const pBegin = &coll_[0];
        MessageItemBase** const pEnd   = pBegin + coll_.size();
        for( MessageItemBase** p = pBegin; p < pEnd; ++p) {
            delete *p;
        }
      #ifdef VERBOSE
        std::cout<<"~Message()"<<std::endl;
      #endif
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
    size() const
    {
        size_t sz = 0;
        MessageItemBase * const * pBegin = &coll_[0];
        MessageItemBase * const * pEnd   = pBegin + coll_.size();
        for( MessageItemBase * const * p = pBegin; p < pEnd; ++p) {
            sz += (*p)->size();
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
}// namespace mpi