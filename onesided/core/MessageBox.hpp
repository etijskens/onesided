#include <mpi.h>

typedef int64_t Index_t; // copied from Primitives/Types/Index.h

 //------------------------------------------------------------------------------------------------
    class Communicator
 //------------------------------------------------------------------------------------------------
 // A wrapper class for MPI_Comm handles
    {
      public:
        Communicator(MPI_Comm comm=MPI_COMM_WORLD);

        inline int rank() const { return rank_; } // the rank of this process
        inline int size() const { return size_; } // number of processes in the Communicator
        inline MPI_Comm comm() const {return comm_;} // the raw MPI_Comm handle

        std::string str() const; // returns std::string("[<rank>/<size>] ") for printing, logging ...

      private:
        MPI_Comm comm_;
        int rank_;
        int size_;
    };

 //------------------------------------------------------------------------------------------------
    class MessageBox
 // encapsulation of
 //   - the MPI_Win object
 //   - the MPI communicator (MPI_COMM_WORLD)
 //   - MPI_Get for header and messages
 //------------------------------------------------------------------------------------------------
    {
      public:
        MessageBox
          ( Index_t maxmsgs=100 // maximum number of messages that can be stored.
          , Index_t size   = -1 // size of the buffer in bytes, the default uses 100000 bytes per message
          );
        ~MessageBox();

        MPI_Win window() { return window_; }

     // Add a message <bytes>, of length <nbytes> for rank <for_rank>
        Index_t addMessage(Index_t for_rank, void* bytes, size_t nBytes);

     // read message headers
     // the default buffer is the buffer in the window. A different buffer can
     // be used for reading message headers obtained via MPI_Get (getHeaderFrom
        Index_t nMessages          (               void const * buffer) const;
        Index_t maxMessages        (               void const * buffer) const;
        int messageDestination     (Index_t msgid, void const * buffer) const;
        Index_t messageBegin       (Index_t msgid, void const * buffer) const;
        Index_t messageEnd         (Index_t msgid, void const * buffer) const;
        Index_t messageSize        (Index_t msgid, void const    * buffer) const;
//        Index_t messageSizeInBytes (Index_t msgid, void * buffer) const;

        void setNMessages          (               Index_t n);
        void setMessageDestination (Index_t msgid, Index_t messageDestination);
        void setMessageEnd         (Index_t msgid, Index_t messageEnd);

        inline Index_t getNMessages() const { return pBuffer_[0]; }

     // header and messages in a readable format, not generic for all message types though
        std::string str() const;

     //

     // Get header from some process (to be called inside an epoch)
        Index_t* getHeaderFrom(int rank);
        void getHeaders();
        std::string headersStr() const;
        void getMessages();

      private:
        Communicator comm_;
        MPI_Win  window_;
        Index_t* pBuffer_;
        Index_t bufferSize_; // in dwords
//        Index_t* nMessages_;
//        Index_t* pHeaderSection_;
//        Index_t* pMessageSection_;
        Index_t nMessageDWords_;

        std::vector<std::vector<Index_t>> receivedMessageHeaders_;

        inline Index_t  headerSize_() const { return pBuffer_[1]; }
        inline Index_t& headerSize_()       { return pBuffer_[1]; }
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
