#include "MessageBuffer.h"
#include <iostream>
#include <sstream>
#include <iomanip>

#define FILL_BUFFER

namespace mpi1s
{   
 //------------------------------------------------------------------------------------------------
 // Implementation of class MessageBuffer       
 //------------------------------------------------------------------------------------------------
    MessageBuffer::
    MessageBuffer()
      : pBuffer_(nullptr)
      , bufferSize_(0)
      , bufferOwned_(false)
      , headersOnly_(false)
      , maxmsgs_(0)
    {}

    MessageBuffer::
    ~MessageBuffer()
    {
        if constexpr(verbose) {
            std::cout<<info<<"~MessageBuffer() "<<pBuffer_<<'['<<bufferSize_<<"] owned="<<bufferOwned_;
        }
        if( bufferOwned_ ) {
            delete[] pBuffer_;
            if constexpr(verbose) std::cout<<" deleted";
        }   if constexpr(verbose) std::cout<<std::endl;
    }

    void 
    MessageBuffer::
    initialize
      ( size_t size     // amount to be allocated for the messages, not counting the memory for the header section
      , size_t max_msgs // maximum number of messages that can be stored.
      )
    {
        headersOnly_ = (size == 0);
        bufferSize_ = 1 + max_msgs * HEADER_SIZE + size;
        pBuffer_ = new Index_t[bufferSize_];
        bufferOwned_ = true;
        maxmsgs_ = max_msgs;
        initialize_();
    }

    void
    MessageBuffer::
    initialize
      ( Index_t * pBuffer // pointer to pre-allocated memory
      , size_t size       // amount of pre-allocated memory 
      , size_t max_msgs   // maximum number of messages that can be stored.
      )
    {
        pBuffer_ = pBuffer;
        bufferSize_ = size;
        bufferOwned_ = false; // buffer is owned by whoever allocated it (typeically MPI_Win_alloc)
        maxmsgs_ = max_msgs;
        initialize_();
    }

    void 
    MessageBuffer::
    initialize_()
    {
        pBuffer_[0] = 0; // initially, there are no messages.
        setMessageBegin( 0, 1 + HEADER_SIZE * maxmsgs_ ); // Begin of the first message, and the size of the message header section.
      #ifdef FILL_BUFFER   
        for( Index_t i = 2; i < bufferSize_; ++i ) {
            pBuffer_[i] = -1;
        }
      #endif
        // std::cout<<"MessageBuffer()::initialize_()"<<pBuffer_<<'/'<<bufferSize_<<'/'<<bufferOwned_<<std::endl;
    }

    void MessageBuffer::clear()
    {// To clear the MessageBuffer, it suffices to set the number of messages to 0
        pBuffer_[0] = 0;
    }

    void*                         // returns pointer to the reserved memory in the MessageBuffer
    MessageBuffer::
    allocateMessage
      ( Index_t  sz             // the size of the message, in bytes
      , int      from_rank      // the source of the message
      , int      to_rank        // the destination of the message
      , MessageHandlerKey_t key // the key of the object responsible for reading the message
      , Index_t* the_msgid      // on return contains the id of the allocated message, if provided
      )
    {
        // std::cout<<headersToStr(true)<<std::endl;

        Index_t msgid = nMessages();
        if( the_msgid ) {
            *the_msgid = msgid;
        }
        incrementNMessages();
        
        setMessageSource     (msgid, from_rank);
        setMessageDestination(msgid, to_rank);
        setMessageHandlerKey (msgid, key);

     // The begin of the message is already set. Only the end must be set.
        Index_t szIndex_t = (sz + (sizeof(Index_t) - 1))/sizeof(Index_t);
        Index_t end = messageBegin(msgid) + szIndex_t;
        setMessageEnd( msgid, end );
     // Set the begin of the next message (so that the comment above is remains true).
     // The begin of the next message is end of this message.
        setMessageBegin( msgid + 1, end ); 

        // std::cout<<headersToStr(true)<<std::endl;

        return ( headersOnly_ ? nullptr : messagePtr(msgid) );
    }

    std::string 
    MessageBuffer::
    headersToStr(bool verbose) const
    {
        std::stringstream ss;
        Index_t n = verbose ? maxMessages() : nMessages();
        // std::cout<<nMessages()<<'/'<<maxMessages()<<std::endl;
        ss<<std::setw(5)<<"id"<<std::setw(20)<<"from"<<std::setw(20)<<"to"<<std::setw(20)<<"key"<<std::setw(20)<<"begin"<<std::setw(20)<<"end"<<'\n';
        for( Index_t i = 0; i < n; ++i ) {
            ss<< ( i < maxMessages() ? ' ' : 'x')
              <<std::setw(4)<<i
              <<std::setw(20)<<messageSource     (i)
              <<std::setw(20)<<messageDestination(i)
              <<std::setw(20)<<messageHandlerKey (i)
              <<std::setw(20)<<messageBegin      (i)
              <<std::setw(20)<<messageEnd        (i)
              <<'\n';
        }        
        return ss.str();
    }

    std::string
    MessageBuffer::
    messageToStr
      ( Index_t msg_id // message index
      )
    {// incomplete implementation. Must make sure that strings have a lengh
        Index_t msg_szb = messageSize(msg_id); // in bytes
        void*   msg_ptr = messagePtr (msg_id);
        std::stringstream ss;
        ss<<"message["<<msg_id<<"]\n"
          <<std::setw( 6)<<"offset"
          <<std::setw(20)<<"ptr"
          <<std::setw(20)<<"size_t"
          <<std::setw(20)<<"float"
          <<std::setw(20)<<"double"
          <<'\n';

        for( Index_t offset=0; offset < msg_szb; offset += sizeof(float) ) {
            ss<<std::setw( 6)<<offset
              <<std::setw(20)<<static_cast<float*>(msg_ptr) + offset/sizeof(float)
              <<std::setw(20)<<interpretAs<size_t>(msg_ptr, offset)
              <<std::setw(20)<<interpretAs<float  >(msg_ptr, offset)
              <<std::setw(20)<<interpretAs<double >(msg_ptr, offset)
              <<'\n';
        }
        return ss.str();
    }
 //------------------------------------------------------------------------------------------------
}