#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include <vector>
#include <iostream>
#include <iomanip>
#include "Communicator.h"
#include "MessageBuffer.h"
// #include "MessageHandler.h"


namespace mpi 
{
 //------------------------------------------------------------------------------------------------
    class MessageBox
 // Encapsulation of
 //   - the MPI_Win object
 //   - the MPI communicator (MPI_COMM_WORLD)
 //   - posting messages to the MPI window
 //   - retrieve header and message from the MPI window 
 // This class 
 //------------------------------------------------------------------------------------------------
    {
        // friend class MessageHandlerBase;
        enum { DEFAULT_MAX_MESSAGES = 10 };
    public:
        MessageBox
          ( size_t bufferSize // the size of the message section of the buffer, in Index_t words.
          , size_t max_msgs   // maximum number of messages that can be stored. 
                              // This defines the size of the header section.
          );
         ~MessageBox();

    //  // Allocate resources in the windowBuffer_ for a message: 
    //  //   - reserve space for a message of size sz to be posted
    //  //   - make a header for that message
    //     void*                          // pointer to the reserved memory in the windowBuffer_
    //     allocateMessage
    //       ( Index_t sz                 // the size of the message, in bites
    //       , int     to_rank            // the destination of the message
    //       , MessageHandlerBase::
    //         key_type messageHandlerKey // the key of the object responsible for reading the message
    //       , Index_t* msgid = nullptr   //  on return contains the id of the allocated message, if provided
    //       );

     // Get all messages for this rank from all other ranks 
        void getMessages();

    private:
     // copy the header section from from_rank into the readHeaders_   
        void getHeaders_
          ( int from_rank // rank to get the header section from
          );

     // MPI_Get the message with id msgid in readHeaders_ (which is a copy of some other
     // rank's header section) and store the message in readBuffer_.
        Index_t           // id of the message received, which was stored in readBuffer_
        getMessage_
          ( Index_t msgid // id of the message to get
          );

     // Fetch the message handler for the message and read the message.
        void readMessage_
          ( Index_t to_msgid // message id in readBuffer_, from which the message is to 
                             // be read with the appropriate message handler
          );

    public: // data member accessors
        inline MessageBuffer& windowBuffer() {return windowBuffer_;}
        inline MPI_Win window() { return window_; }
        inline Communicator const & comm() const { return comm_; }
        
    private:
        Communicator comm_;
        MPI_Win  window_;
        MessageBuffer windowBuffer_; // its memory is allocated by MPI_Win_allocate
        MessageBuffer readHeaders_;   // its memory is allocated by new Index_t[]
        MessageBuffer readBuffer_;   // its memory is allocated by new Index_t[]
    };


 //------------------------------------------------------------------------------------------------
    class Epoch
 // a RAII class, typical use:
 //     Messagebox mb;
 //     /* Each rank can add some messages to its mb */
 //     {
 //         Epoch epoch(mb); // RAII object, created at the beginning of a scope
 //
 //         int myrank = mb.rank();
 //         for( int rank=0; rank<mb.nranks(); ++rank ) {
 //             if( rank != myrank ) {
 //                 /* retrieve messages from rank for myrank */
 //                 /* first retrieve the header section from rank's MessageBox */
 //                 /* loop over all the messages in the header, skip messages not for my_rank */
 //                 /* retrieve the messages which are for my_rank */
 //             }
 //         }
 //     }// the epoch object goes out of scope, and is destroyed, which closes the MPI_Win_fence.
 //------------------------------------------------------------------------------------------------
    {
      public:
        Epoch(MessageBox& mb, int assert); // open an epoch for MessageBox mb
        ~Epoch();                          // close the epoch
      private:
        Epoch(Epoch const &); // prevent object copy
        MessageBox& mb_;
    };
 //------------------------------------------------------------------------------------------------
}// namespace mpi

#endif // MESSAGEBOX_H
