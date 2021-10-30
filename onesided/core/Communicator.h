#include <string>
#include <mpi.h>

namespace mpi
{//------------------------------------------------------------------------------------------------
    class Communicator
 // A wrapper class for MPI_Comm handles
 //------------------------------------------------------------------------------------------------
    {
      public:
        Communicator(MPI_Comm comm=MPI_COMM_WORLD);

        inline int rank() const { return rank_; } // the rank of this process
        inline int size() const { return size_; } // number of processes in the Communicator
        inline MPI_Comm comm() const {return comm_;} // the raw MPI_Comm handle

        inline std::string str() const; // returns std::string("[<rank>/<size>] ") for printing, logging ...

      private:
        MPI_Comm comm_;
        int rank_;
        int size_;
    };
 //------------------------------------------------------------------------------------------------
}