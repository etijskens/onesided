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
        if constexpr(::mpi12s::_verbose_) printf("%s ~MessageHandlerBase()\n", CINFO);
    }

    void
    MessageHandlerBase::
    putMessage(int to_rank)
    {// construct the message, and put the message in the mpi1s window
     // compute the length of the message:
        Index_t sz = ::mpi12s::convertSizeInBytes<sizeof(Index_t)>(message_.messageSize());
        int const from_rank = ::mpi12s::rank;
        Index_t msgid = -1;
        void* ptr = messageBox_.windowBuffer().allocateMessage( sz, from_rank, to_rank, key_, &msgid );
        message_.write(ptr);

        if constexpr(::mpi12s::_debug_) {
            printf("%s\nMessageHandlerBase::putMessage() : windowBuffer headers\n", CINFO);
//            print_lines(messageBox_.windowBuffer().headersToStr());
            printf("%s\nMessageHandlerBase::putMessage() : windowBuffer message : msg_id=%lld\n", CINFO, msgid);
//            print_lines(messageBox_.windowBuffer().messageToStr(msgid));
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

namespace mpi2s
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
    MessageHandlerBase()
    {
        theMessageHandlerRegistry.registerMessageHandler(this);
    }

    MessageHandlerBase::
    ~MessageHandlerBase()
    {
        if constexpr(::mpi12s::_debug_ && _debug_)
            ::mpi12s::prdbg( "~MessageHandlerBase()" );
    }

    void
    MessageHandlerBase::
    postMessage(int to_rank)
    {// construct the message, and put the message in the messageBuffer
     // compute the length of the message:
        Index_t sz = ::mpi12s::convertSizeInBytes<sizeof(Index_t)>(message_.messageSize());
     // allocate memory space for the message in the message buffer, and write the header
     // for the message in the message buffer
        int const from_rank = ::mpi12s::rank;
        Index_t msg_id = -1;
        void* ptr = ::mpi12s::theMessageBuffer.allocateMessage( sz, from_rank, to_rank, key_, &msg_id );
     // Write the message in the message buffer
        message_.write(ptr);

        if constexpr(::mpi12s::_debug_ && _debug_) {
            ::mpi12s::prdbg
              ( ::mpi12s::tostr("MessageHandlerBase::postMessage() : headers (current msg_id=", msg_id, ")")
              , ::mpi12s::theMessageBuffer.headersToStr()
              );
            ::mpi12s::prdbg
              ( ::mpi12s::tostr("MessageHandlerBase::postMessage() : message (current msg_id=", msg_id, ")")
              , ::mpi12s::theMessageBuffer.messageToStr(msg_id)
              );
        }
    }

    bool               // true if the MessageHandlerKey value in the message header
                       // with id msg_id corresponds to this->key_, false otherwise.
                       // in which case the message was not written by this
                       // MessageHandler, and therefore also cannot be read by it.
    MessageHandlerBase::
    readMessage
      ( Index_t msg_id // the message id identifies the message header
      )
    {// Verify that this is the correct MessageHandler for this message
        if( ::mpi12s::theMessageBuffer.messageHandlerKey(msg_id) != key_)
            return false;

     // Read
        void* ptr = ::mpi12s::theMessageBuffer.messagePtr(msg_id);
        message_.read(ptr);

        return true;
    }

 //------------------------------------------------------------------------------------------------
}// namespace mpi1s