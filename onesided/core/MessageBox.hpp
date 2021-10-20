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

    class MessageStream;
 //------------------------------------------------------------------------------------------------
    class MessageBox
 //------------------------------------------------------------------------------------------------
    {
        friend class MessageStream;
      public:
        MessageBox
          ( Index_t maxmsgs=100 // maximum number of messages that can be stored.
          , Index_t size   = -1 // size of the buffer in bytes, the default uses 100000 bytes per message
          );
        ~MessageBox();

        Index_t nMessages() const;

     // Add a message <bytes>, of length <nbytes> for rank <for_rank>
        Index_t addMessage(Index_t for_rank, void* bytes, size_t nBytes);

        Index_t       rank(Index_t msgid) const { return pHeaderSection_[msgid*2  ]; }
        Index_t   endIndex(Index_t msgid) const { return pHeaderSection_[msgid*2+1]; }
        Index_t beginIndex(Index_t msgid) const { return pHeaderSection_[msgid*2-1]; }

        Index_t&       rank(Index_t msgid) { return pHeaderSection_[msgid*2  ]; }
        Index_t&   endIndex(Index_t msgid) { return pHeaderSection_[msgid*2+1]; }
        Index_t& beginIndex(Index_t msgid) { return pHeaderSection_[msgid*2-1]; }

        std::string str() const;

      private:
        MPI_Comm comm_;
        MPI_Win  window_;
        Index_t  maxmsgs_;
        void*    pBuffer_;
        Index_t* nMessages_;
        Index_t* pHeaderSection_;
        Index_t* pMessageSection_;
        Index_t nMessageDWords_;
    };

 //------------------------------------------------------------------------------------------------
    class Epoch
 //------------------------------------------------------------------------------------------------
    {
      public:
        Epoch(MessageBox& mb); // open an epoch for MessageBox mb
        ~Epoch();              // close the epoch
    };
 //------------------------------------------------------------------------------------------------
