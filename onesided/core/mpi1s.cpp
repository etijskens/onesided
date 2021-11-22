#include "mpi1s.h"

#include <iostream>
#include <sstream>

namespace mpi1s
{//---------------------------------------------------------------------------------------------------------------------
    int rank = -1;
    int size = -1;
    std::string info;

    void init()
    {// initialize MPI
        int argc = 0;
        char **argv = nullptr;
        MPI_Init(&argc, &argv);

     // initialize rank and size
        MPI_Comm_size(MPI_COMM_WORLD, &size);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);

      // initialize info
        std::stringstream ss;
        ss<<"MPI rank ["<<rank<<'/'<<size<<"] ";
        info = ss.str();

        if constexpr(verbose) std::cout<<info<<"::mpi1s::init() succeeded."<<std::endl;
    }

    void finalize()
    {
        if constexpr(verbose) std::cout<<info<<"MPI_Finalize() to be called ..."<<std::endl;
        MPI_Finalize();
        if constexpr(verbose) std::cout<<info<<"MPI_Finalize() succeeded."<<std::endl;
    }
 //---------------------------------------------------------------------------------------------------------------------
}
