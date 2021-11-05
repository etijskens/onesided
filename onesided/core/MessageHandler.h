#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H

#include <map>
#include "types.h"
#include "MessageBox.h"
#include "Message.h"

namespace mpi 
{
    class MessageHandlerBase; // forward declaration 

 //------------------------------------------------------------------------------------------------
   class MessageHandlerRegistry
 //------------------------------------------------------------------------------------------------
    {
    public:
        typedef MessageHandlerKey_t key_type; // if this must be changed, do it in "types.h".

        MessageHandlerRegistry();
        // ~MessageHandlerRegistry() {}
        
     // Create a MessageHander, store it in the registry_, and return a pointer to it.
        void registerMessageHandler(MessageHandlerBase* messageHandler);

        inline MessageHandlerBase& operator[](key_type key) {
            return *registry_[key];
        }

    private:
        size_t counter_;
        std::map<key_type,MessageHandlerBase*> registry_;
    };

 // A global MessageHandlerRegistry 
 // Check out https://stackoverflow.com/questions/86582/singleton-how-should-it-be-used for when to
 // use singletons and how to implement them.
    extern MessageHandlerRegistry theMessageHandlerRegistry;
 
 //------------------------------------------------------------------------------------------------
   class MessageHandlerBase
 // Base class for message handlers
 //------------------------------------------------------------------------------------------------
   {
       friend class MessageHandlerRegistry;
    public:
        typedef MessageHandlerRegistry::key_type key_type;

        MessageHandlerBase(MessageBox& mb);

     // Post the message (i.e. put it in the MPI window.)
        void putMessage
          ( int to_rank // destination of the message.
          );
        
     // Read the message from a buffer (void*)
     //    someMessageHandler.message().read(ptr)

     // data member access
        inline MessageBox& messageBox() { return messageBox_; }
        inline Message& message() { return message_; }
        inline key_type key() const { return key_; }
        inline Communicator const & comm() const { return messageBox_.comm(); } 

    protected:
        MessageBox& messageBox_;
        Message message_;
        key_type key_;
    };
 //------------------------------------------------------------------------------------------------
}// namespace mpi
#endif // MESSAGEHANDLER_H
