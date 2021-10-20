#include <stdexcept>
#include <sstream>

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
    Index_t toDWords(Index_t bytes) {
        return ((bytes + 7) / 8);
    }

 //------------------------------------------------------------------------------------------------
 // class Communicator implementation
 //------------------------------------------------------------------------------------------------
    MessageBox::
    MessageBox
      ( Index_t maxmsgs
      , Index_t size // in bytes
      ) : comm_(MPI_COMM_WORLD)
        , maxmsgs_(maxmsgs)
        , pBuffer_(NULL)
        , pHeaderSection_(NULL)
        , pMessageSection_(NULL)
    {
        if( size == -1 ) {
            size = 10000 * maxmsgs;
        }

        Index_t n = sizeof(Index_t);
        Index_t size_dw = toDWords(size); // make sure the buffer is on 64bit boundaries
        size *= n;

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
          , &pBuffer_
          , &window_
          );

        if( success != MPI_SUCCESS ) {
            Communicator comm(comm_);
            std::string errmsg = comm.str() + "MPI_Win_allocate failed.";
            throw std::runtime_error(errmsg);
        }
        nMessages_ = static_cast<Index_t*>(pBuffer_);
        nMessages_[0] = 0;
        nMessages_[1] = 0;  // always 0, begin index of first message.
        pHeaderSection_  = nMessages_ + 2;
        // the header of a message contains 2 numbers:
        //  - destination
        //  - the index of the element past the end of the message.
        // the index of the begin of the message is retrieved from the end
        // of the previous message, which is 0 if the message id is 0 (stored at nMessagesa[0])

        pMessageSection_ = pHeaderSection_ + 2*maxmsgs_;
        nMessageDWords_ = size_dw - 2*(maxmsgs_+1);
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
        return nMessages_[0];
    }

    Index_t
    MessageBox::
    addMessage(Index_t for_rank, void* bytes, size_t nBytes)
    {
        Index_t msgid = *nMessages_;
        Index_t ibegin = beginIndex(msgid);
        endIndex(msgid) = ibegin + toDWords(nBytes);
        rank(msgid) = for_rank;

        memcpy( &pMessageSection_[ibegin], bytes, nBytes );

        ++(*nMessages_);

        return msgid;
    }


    std::string
    MessageBox::str() const
    {
        std::stringstream ss;
        ss<<"MessageBuffer, nMessages = "<< nMessages()<<'\n';
        for( int i=0; i<nMessages(); ++i )
        {
            ss<<"\nmessage "<<i<<" for rank "<<rank(i);
            Index_t j0 = beginIndex(i);
            for( Index_t j=j0; j<endIndex(i); ++j) {
                Index_t val_int = pMessageSection_[j];
                double  val_dbl = (reinterpret_cast<double*>(pMessageSection_))[j];
                ss<<"\n  ["<<j-j0<<"] int = "<<val_int<<", dbl = "<<val_dbl;
            }
        }
        ss<<'\n';
        return ss.str();
    }

