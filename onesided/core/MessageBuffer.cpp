#include "MessageBuffer.h"
#include <iostream>
#include <sstream>
#include <iomanip>

#define FILL_BUFFER

namespace mpi
{   
 //------------------------------------------------------------------------------------------------
 // Implementation of class MessageBuffer       
 //------------------------------------------------------------------------------------------------
    MessageBuffer::
    MessageBuffer()
      : pBuffer_(nullptr)
      , bufferSize_(0)
      , bufferOwned_(false)
      , maxmsgs_(0)
    {}

    MessageBuffer::
    ~MessageBuffer()
    {
        std::cout<<"~MessageBuffer()"<<pBuffer_<<'/'<<bufferSize_<<'/'<<bufferOwned_<<std::endl;
        if( bufferOwned_ )
            delete[] pBuffer_;
    }

    void 
    MessageBuffer::
    initialize( size_t size, size_t max_msgs )
    {
        bufferSize_ = 1 + max_msgs * HEADER_SIZE + size;
        pBuffer_ = new Index_t[bufferSize_];
        bufferOwned_ = true;
        maxmsgs_ = max_msgs;
        initialize_();
    }

    void 
    MessageBuffer::
    initialize( Index_t * pBuffer, size_t size, size_t max_msgs )
    {
        pBuffer_ = pBuffer;
        bufferSize_ = size;
        bufferOwned_ = false;
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

    std::string 
    MessageBuffer::
    headers(bool verbose) const
    {
        std::stringstream ss;
        Index_t n = verbose ? maxMessages() : nMessages();
        // std::cout<<nMessages()<<'/'<<maxMessages()<<std::endl;
        ss<<std::setw(4)<<"id"<<std::setw(20)<<"from"<<std::setw(20)<<"to"<<std::setw(20)<<"key"<<std::setw(20)<<"begin"<<std::setw(20)<<"end"<<'\n';
        for( Index_t i = 0; i < n; ++i ) {
            ss<<std::setw(4)<<i
              <<std::setw(20)<<messageSource     (i)
              <<std::setw(20)<<messageDestination(i)
              <<std::setw(20)<<messageHandlerKey (i)
              <<std::setw(20)<<messageBegin      (i)
              <<std::setw(20)<<messageEnd        (i)
              <<'\n';
        }        
        return ss.str();
    }

 //------------------------------------------------------------------------------------------------
}