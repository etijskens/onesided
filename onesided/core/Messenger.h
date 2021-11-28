#ifndef MESSENGER_H
#define MESSENGER_H

#include "MessageBuffer.h"

namespace mpi1s
{//---------------------------------------------------------------------------------------------------------------------
    class Messenger
    {
    public:
     // ctor
        Messenger
          ( size_t bufferSize // the size of the message section of the buffer, in Index_t words.
          , size_t max_msgs   // maximum number of messages that can be stored.
                              // This defines the size of the header section.
          );

     // Make sure that every process has the headers of all processes
        void broadcastHeaders();
    private:
        MessageBuffer messageBuffer_;
    };
 //---------------------------------------------------------------------------------------------------------------------
}
#endif // MESSENGER_H
