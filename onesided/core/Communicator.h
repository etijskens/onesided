#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include <string>
#include <mpi.h>
#include "memcpy_able.h"

namespace mpi1s
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
        
     // get the rank corresponding to rank_ + n
        inline int // result is always in [0,size_[ (as with periodic boundary conditions)
        next_rank
          ( int n = 1 // number of ranks to move, n>0 moves upward, n<0 moves down.
          ) const
        {
            int r = rank_ + n;
            while( r < 0 ) r += size_;
            return r % size_;
        }

      private:
        MPI_Comm comm_;
        int rank_;
        int size_;
    };
 //------------------------------------------------------------------------------------------------
}// namespace mpi1s

#endif // COMMUNICATOR_H
