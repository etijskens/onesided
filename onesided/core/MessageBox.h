#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include <vector>
#include "Communicator.h"
#include "MessageBuffer.h"
#include "MessageHandler.h"


namespace mpi 
{

 //------------------------------------------------------------------------------------------------
    class MessageBox
 // Encapsulation of
 //   - the MPI_Win object
 //   - the MPI communicator (MPI_COMM_WORLD)
 //   - add messages to the MPI window
 //   - check someone else's header and messages (MPI_Get)
 // This class 
 //------------------------------------------------------------------------------------------------
    {
    public:
        MessageBox
          ( size_t maxmsgs=100 // maximum number of messages that can be stored.
          , size_t size=-1
          );
         ~MessageBox();

        inline MPI_Win window() { return window_; }

        inline Communicator const & comm() const { return comm_; }

     // Get header from some process (to be called inside an epoch)
        Index_t getHeaderFromRank(int rank); // return msgid in readBuffer_ ???
        void    getHeaderFromAllRanks();
        void getMessages();
        
     // string representation of header section
        std::string headerSectionToStr() const;

     // Post a message (binary) in the MPI window.
      Index_t postMessage(Index_t for_rank, MessageHandlerBase& messageHandler);
        
      private:
        Communicator comm_;
        MPI_Win  window_;
        MessageBuffer windowBuffer_; // its memory is allocated by MPI_Win_allocate
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
}// namespace mpi ends here 
#endif // MESSAGEBOX_H
