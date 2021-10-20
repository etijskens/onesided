#include <stdexcept>

#include "MessageBox.hpp"

 //------------------------------------------------------------------------------------------------
 // class Communicator implementation
 //------------------------------------------------------------------------------------------------
    Communicator::
    Communicator(MPI_Comm comm)
      : comm_(comm)
      , rank_(-1)
      , size_(-1)
    {
        MPI_Comm_size(comm_, &size_);
        MPI_Comm_rank(comm_, &rank_);
    }

    std::string
    Communicator::
    str() const
    {
        return std::string("[") + std::to_string(rank_) + "/" + std::to_string(size_) + "] ";
    }

 //------------------------------------------------------------------------------------------------
 // class Communicator implementation
 //------------------------------------------------------------------------------------------------
    MessageBox::
    MessageBox
      ( Index_t maxmsgs
      , Index_t size
      ) : comm_(MPI_COMM_WORLD)
        , maxmsgs_(maxmsgs)
        , buffer_(NULL)
        , headers_(NULL)
        , messages_(NULL)
    {
        if( size == -1 ) {
            size = 10000 * maxmsgs;
        }

        int success =
        MPI_Win_allocate
          ( static_cast<MPI_Aint>(size) // The size of the memory area exposed through the window, in bytes.
          , sizeof(Index_t)             // The displacement unit is used to provide an indexing feature during
                                        // RMA operations. Indeed, the target displacement specified during RMA
                                        // operations is multiplied by the displacement unit on that target.
                                        // The displacement unit is expressed in bytes, so that it remains
                                        // identical in an heterogeneous environment.
          , MPI_INFO_NULL
          , comm_
          , &buffer_
          , &window_
          );

        if( success != MPI_SUCCESS ) {
            Communicator comm(comm_);
            std::string errmsg = comm.str() + "MPI_Win_allocate failed.";
            throw std::runtime_error(errmsg);
        }

        nMessages_ = static_cast<Index_t*>(buffer_);
        headers_ = nMessages_ + 1;
        messages_= headers_ + 3*maxmsgs;
        end_ = static_cast<void*>(static_cast<Index_t*>(buffer_) + (size/sizeof(Index_t)));

        *nMessages_ = 0;
    }

    MessageBox::
    ~MessageBox()
    {
        MPI_Win_free(&window_);
    }

    Index_t
    MessageBox::
    nMessages() const
    {
        return static_cast<Index_t*>(buffer_)[0];
    }

