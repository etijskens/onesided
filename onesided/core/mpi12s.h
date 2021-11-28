#ifndef MPI12S_H
#define MPI12S_H

#include <mpi.h>

#include <strstream>
#include <vector>
#include <string>

#define  INFO ::mpi12s::info
#define CINFO ::mpi12s::info.c_str()

/*
    https://stackoverflow.com/questions/35324597/writing-to-text-file-in-mpi
    https://www.geeksforgeeks.org/variadic-function-templates-c/
    https://en.cppreference.com/w/cpp/language/parameter_pack
    https://eli.thegreenplace.net/2014/variadic-templates-in-c/
 */

namespace mpi12s // this code is both for the one-sided approach and for the two-sided approach.
{//---------------------------------------------------------------------------------------------------------------------
 // compile time constants restricted to this namespace. Set to true for debugging.
//    bool const _verbose_ = false;
    bool const _verbose_ = true;
    bool const _debug_ = true;

 // These global variables are set by mpi1s::init()
    extern int rank;
    extern int size;
    extern std::string info; // "MPI rank [rank/size]", useful for debugging messages

 // typedefs
    typedef std::vector<std::string> Lines_t;


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
 // Machinery for producing debugging output
    extern std::string dbg_fname;\
    extern const Lines_t nolines;
    extern int64_t timestamp0;
//    void prdbg(std::string const& s);
    void prdbg(std::string const& s, Lines_t const& lines=nolines);

    template<typename Last>
    void tostrhelper(std::stringstream & ss, Last const& last)
    {
        ss<<last;
    }

    template<typename First, typename... Args>
    void tostrhelper(std::stringstream & ss, First const& first, Args... args)
    {
        ss<<first;
        tostrhelper(ss, args...);
    }

    template<typename... Args>
    std::string tostr(Args... args)
    {
        std::stringstream ss;
        tostrhelper(ss, args...);
        return ss.str();
    }
 //---------------------------------------------------------------------------------------------------------------------
}
#endif // MPI12S_H
