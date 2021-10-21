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

        Index_t nMessages() const;

        MPI_Win window() { return window_; }

     // Add a message <bytes>, of length <nbytes> for rank <for_rank>
        Index_t addMessage(Index_t for_rank, void* bytes, size_t nBytes);

        Index_t       rank(Index_t msgid) const { return pHeaderSection_[msgid*2  ]; }
        Index_t   endIndex(Index_t msgid) const { return pHeaderSection_[msgid*2+1]; }
        Index_t beginIndex(Index_t msgid) const { return pHeaderSection_[msgid*2-1]; }

        Index_t&       rank(Index_t msgid) { return pHeaderSection_[msgid*2  ]; }
        Index_t&   endIndex(Index_t msgid) { return pHeaderSection_[msgid*2+1]; }
        Index_t& beginIndex(Index_t msgid) { return pHeaderSection_[msgid*2-1]; }

     // header and messages in a readable format, not generic for all message types though
        std::string str() const;

     // access the MPI communicator:
        int rank() const;
        int nranks() const;

     // Get header from some process (to be called inside an epoch)
        Index_t* getHeaderFrom(int rank);
        void getHeaders();
        std::string headersStr() const;
        void getMessages();

      private:
        Communicator comm_;
        MPI_Win  window_;
        Index_t  maxmsgs_;
        void*    pBuffer_;
        Index_t* nMessages_;
        Index_t* pHeaderSection_;
        Index_t* pMessageSection_;
        Index_t nMessageDWords_;

        std::vector<std::vector<Index_t>> messageHeaders_;

        size_t headerSize_() const { return 2 + 2*maxmsgs_; }
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
