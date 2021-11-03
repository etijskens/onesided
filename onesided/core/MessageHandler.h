#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H

#include <map>
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
        MessageHandlerRegistry();
        // ~MessageHandlerRegistry() {}
        
     // Create a MessageHander, store it in the registry_, and return a pointer to it.
        void registerMessageHandler(MessageHandlerBase* messageHandler);

        inline MessageHandlerBase& operator[](size_t key) {
            return *registry_[key];
        }

    private:
        size_t counter_;
        std::map<size_t,MessageHandlerBase*> registry_;
    };


 
 //------------------------------------------------------------------------------------------------
   class MessageHandlerBase
 // Base class for message handlers
 //------------------------------------------------------------------------------------------------
   {
       friend class MessageHandlerRegistry;
    public:
        MessageHandlerBase(MessageBox& mb);

     // Post the message (i.e. put it in the MPI window.)
        void post(int to_rank);


     // data member access
        inline Message& message() { return message_; }
        inline size_t key() const { return key_; }

    protected:
        MessageBox& messageBox_;
        Message message_;
        size_t key_;
    };
 //------------------------------------------------------------------------------------------------
}// namespace mpi
#endif // MESSAGEHANDLER_H
