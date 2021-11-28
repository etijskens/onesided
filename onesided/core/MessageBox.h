#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include <vector>
#include <iostream>
#include <iomanip>
#include "mpi12s.h"
#include "MessageBuffer.h"
// #include "MessageHandler.h"


namespace mpi1s
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
        inline ::mpi12s::MessageBuffer& windowBuffer() { return windowBuffer_; }
        inline mpi12s::MessageBuffer&   readBuffer() { return   readBuffer_; }
        inline MPI_Win window() { return window_; }

    private:
        MPI_Win  window_;
        ::mpi12s::MessageBuffer windowBuffer_; // its memory is allocated by MPI_Win_allocate
        ;;mpi12s::MessageBuffer readHeaders_;  // its memory is allocated by new Index_t[]
        ::mpi12s::MessageBuffer readBuffer_;   // its memory is allocated by new Index_t[]
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
     // Open an epoch for the MPI window of MessageBox mb
        Epoch
          ( MessageBox& mb          // the MessageBox whose MPI window is going to be opened.
          , int assert              // argument for MPI_Win_fence
          , char const* msg = ""
          );

     // close the epoch
        ~Epoch();

      private:
        Epoch(Epoch const &); // prevent object copy
        MessageBox& mb_;
        char const* msg_;
    };
 //------------------------------------------------------------------------------------------------
}// namespace mpi1s

#endif // MESSAGEBOX_H
