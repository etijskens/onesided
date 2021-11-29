#include "mpi12s.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <cmath>


namespace mpi12s
{//---------------------------------------------------------------------------------------------------------------------
    int rank = -1;
    int size = -1;
    std::string info;
    std::string dbg_fname;
    int64_t timestamp0;


    void init()
    {// initialize MPI
        int argc = 0;
        char **argv = nullptr;
        int success =
        MPI_Init(&argc, &argv);

     // initialize rank and size
        MPI_Comm_size(MPI_COMM_WORLD, &size);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);

     // initialize info
        std::stringstream ss;
        ss<<"MPI rank ["<<rank<<'/'<<size<<']';
        info = ss.str();

     // initialize debug output file.
        if constexpr(_debug_)
        {
            unsigned n = (unsigned)log10((float)size) + 2;
            std::stringstream fname;
            fname<<std::setfill('_')<<std::setw(n)<<rank<<".dbg";
            dbg_fname = fname.str();

            using namespace std::chrono;
            if( rank == 0) timestamp0 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
            MPI_Bcast( &timestamp0, 1, MPI_LONG_LONG_INT, 0, MPI_COMM_WORLD );
            int64_t timestamp = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - timestamp0;

            FILE* fh = fopen(dbg_fname.c_str(), "w");
            if(fh==nullptr) {
                printf("%s failed to open %s file: permission issue?\n", CINFO, dbg_fname.c_str());
                exit(1);
            } else {
                printf("%s Opened %s file for debug output.\n", CINFO, dbg_fname.c_str());
            }
            fprintf(fh, "--------------------------------------------------------------------------------\n");
            fprintf(fh, "%s - debug output\n",CINFO);
            fprintf(fh, "--------------------------------------------------------------------------------\n\n");
            fprintf(fh, "[%lld]\nmpi12s::init()\n", timestamp);
            fprintf(fh, "  %s\n", (success==MPI_SUCCESS ? "MPI_Initialize succeeded." : "MPI_Initialize failed."));
            fprintf(fh, "--------------------------------------------------------------------------------\n\n");
            fclose(fh);
        }
    }

    void finalize()
    {
        int success = MPI_Finalize();
        if constexpr(::mpi12s::_verbose_) {
            std::string msg = (success==MPI_SUCCESS ? "mpi12s::finalize()\n  MPI_Finalize succeeded."
                                                    : "mpi12s::finalize()\n  MPI_Finalize failed.");
            prdbg(msg);
        }
    }

 //---------------------------------------------------------------------------------------------------------------------
 // Machinery for producing debugging output
 //---------------------------------------------------------------------------------------------------------------------
    const Lines_t nolines;
    void prdbg(std::string const& s, Lines_t const& lines)
    {
        FILE* fh = fopen(dbg_fname.c_str(), "a");
        if(fh==nullptr){printf("%s failed to open .dbg file: permission issue?\n", CINFO); exit(1);}

        using namespace std::chrono;
        int64_t timestamp = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - timestamp0;
        fprintf(fh, "[%lld]\n%s\n", timestamp, s.c_str());

        for( auto line: lines ) fprintf(fh, "%s\n", line.c_str());

        fprintf(fh, "--------------------------------------------------------------------------------\n\n");
        fclose(fh);
    }


 //---------------------------------------------------------------------------------------------------------------------
}// namespace mpi12s
