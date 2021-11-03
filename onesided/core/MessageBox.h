#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include <vector>
#include <iostream>
#include <iomanip>
#include "Communicator.h"
#include "MessageBuffer.h"


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
        friend class MessageHandlerBase;
        enum { DEFAULT_MAX_MESSAGES = 10 };
    public:
        MessageBox
          ( size_t bufferSize    
          , size_t max_msgs   // maximum number of messages that can be stored.
          );
         ~MessageBox();

        inline MPI_Win window() { return window_; }

        inline Communicator const & comm() const { return comm_; }

     // Get header from some process (to be called inside an epoch)
        void getMessages();
        void getHeader (int from_rank);
        void getMessage(int from_rank, Index_t msgid);

     // Reserve space for a message of size sz to be posted, and make a header for it.
     // Returns a pointer to the reserved part of the MPI window buffer.
     // The message id is returned in msgid. 
        void* allocateMessage(Index_t sz, int from_rank, int to_rank, size_t key, Index_t* msgid = nullptr);
        
      private:
        Communicator comm_;
        MPI_Win  window_;
        MessageBuffer windowBuffer_; // its memory is allocated by MPI_Win_allocate
        MessageBuffer readHeader_;   // its memory is allocated by new Index_t[]
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
