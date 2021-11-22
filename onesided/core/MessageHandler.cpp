#include "MessageHandler.h"

namespace mpi1s
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

    MessageHandlerBase::
    ~MessageHandlerBase()
    {
        if constexpr(verbose) std::cout<<::mpi1s::info<<"~MessageHandlerBase()"<<std::endl;
    }

    void
    MessageHandlerBase::
    putMessage(int to_rank)
    {// construct the message, and put the message in the mpi1s window
     // compute the length of the message:
        Index_t sz = convertSizeInBytes<sizeof(Index_t)>(message_.messageSize());
        int const from_rank = ::mpi1s::rank;
        Index_t msgid = -1;
        void* ptr = messageBox_.windowBuffer().allocateMessage( sz, from_rank, to_rank, key_, &msgid );
        message_.write(ptr);

        if constexpr(debug)
        {
            std::string s = info + "MessageHandlerBase::putMessage()"
                                 + "\nwindowBuffer headers\n" + messageBox_.windowBuffer().headersToStr()
                                 + "\nwindowBuffer message\n" + messageBox_.windowBuffer().messageToStr(msgid);
            std::cout<<s<<std::endl;
        }
    }

    void
    MessageHandlerBase::
    getMessages()
    {
        messageBox_.getMessages();
    }
 //------------------------------------------------------------------------------------------------
}// namespace mpi1s