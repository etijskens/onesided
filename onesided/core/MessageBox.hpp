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
 //------------------------------------------------------------------------------------------------
    {
      public:
        MessageBox
          ( Index_t maxmsgs=100 // maximum number of messages that can be stored.
          , Index_t size   = -1 // size of the buffer in bytes, the default uses 100000 bytes per message
          );
        ~MessageBox();

        Index_t nMessages() const;


      private:
        MPI_Comm comm_;
        MPI_Win  window_;
        void *   buffer_;
        Index_t* headers_;
        void *   messages_;
        void *   end_;
        Index_t  maxmsgs_;
        Index_t* nMessages_;
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
