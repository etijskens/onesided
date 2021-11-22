#ifndef MPI1S_H
#define MPI1S_H

#include <mpi.h>

namespace mpi1s // shorthand for mpi-onesided
{//---------------------------------------------------------------------------------------------------------------------
 // compile time constants restricted to this namespace. Set to true for debugging.
    bool const verbose = false;
//    bool const verbose = true;
    bool const debug = true;

 // These global variables are set by mpi1s::init()
    extern int rank;
    extern int size;
    extern std::string info; // "[rank/size] ", useful for debugging messages

 // Initialize MPI
    void init();

 // finalize MPI
    void finalize();

 // get the rank corresponding to rank_ + n
    inline int // result is always in [0,size_[ (as with periodic boundary conditions)
    next_rank
      ( int n = 1 // number of ranks to move, n>0 moves upward, n<0 moves down.
      )
    {
        int r = rank + n;
        while( r < 0 ) r += size;
        return r % size;
    }
 //---------------------------------------------------------------------------------------------------------------------
}
#endif // MPI1S_H
