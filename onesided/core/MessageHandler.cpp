#include "MessageHandler.h"

namespace mpi 
{
 //------------------------------------------------------------------------------------------------
 // MessageHandlerRegistry implementation
 //------------------------------------------------------------------------------------------------

    MessageHandlerRegistry::
    MessageHandlerRegistry() : counter_(0) {}

    void
    MessageHandlerRegistry::
    registerMessageHandler(MessageHandlerBase* messageHandler)
    {
        registry_[counter_] = messageHandler;
        messageHandler->key_ = counter_;
        ++counter_;
    }

    MessageHandlerRegistry theMessageHandlerRegistry;

 //------------------------------------------------------------------------------------------------
 // MessageHandlerRegistry implementation
 //------------------------------------------------------------------------------------------------
    MessageHandlerBase::
    MessageHandlerBase(MessageBox& mb)
      : messageBox_(mb)
    {
        theMessageHandlerRegistry.registerMessageHandler(this);
    }

    void
    MessageHandlerBase::
    post(int to_rank)
    {// construct the message, and put the message in the mpi window

     // compute the length of the message:
        Index_t sz = convertSizeInBytes<sizeof(Index_t)>(message_.size());
        int from_rank = messageBox_.comm().rank();
        Index_t msgid = -1;
        void* ptr = messageBox_.windowBuffer().allocateMessage( sz, from_rank, to_rank, key_, &msgid );
        message_.write(ptr);
    }

 //------------------------------------------------------------------------------------------------
}// namespace mpi