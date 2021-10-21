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
 // Convert a size in bytes, ensuring that the result is rounded up to Boundary. The result is
 // expressed in word of Unit bytes.
 // E.g.  convertSizeInBytes<8>(4) -> 8 : the smallest 8 byte boundary >= 4 bytes is 8 bytes
 // E.g.  convertSizeInBytes<8,8>(4) -> 8 : the smallest 8 byte boundary >= 4 bytes is 1 8-byte word
 // E.g.  convertSizeInBytes<8,2>(4) -> 8 : the smallest 8 byte boundary >= 4 bytes is 4 2-byte words
    template<size_t Boundary, size_t Unit=1>
    Index_t convertSizeInBytes(Index_t bytes) {
        return ((bytes + Boundary - 1) / Boundary) * (Boundary / Unit);
    }


 //------------------------------------------------------------------------------------------------
 // class MessageBox implementation
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

        Index_t size_dw = convertSizeInBytes<8>(size); // make sure the buffer is on 64bit boundaries

        int success =
        MPI_Win_allocate
          ( static_cast<MPI_Aint>(size) // The size of the memory area exposed through the window, in bytes.
          , sizeof(Index_t)             // The displacement unit is used to provide an indexing feature during
                                        // RMA operations. Indeed, the target displacement specified during RMA
                                        // operations is multiplied by the displacement unit on that target.
                                        // The displacement unit is expressed in bytes, so that it remains
                                        // identical in an heterogeneous environment.
          , MPI_INFO_NULL
          , comm_.comm()
          , &pBuffer_
          , &window_
          );

        if( success != MPI_SUCCESS ) {
            std::string errmsg = comm_.str() + "MPI_Win_allocate failed.";
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
        // of the previous message, which is 0 if the message id is 0 (stored at nMessages[0])

        pMessageSection_ = pHeaderSection_ + 2*maxmsgs_;
        nMessageDWords_ = size_dw - headerSize_();

     // reserve space for the headers of the other ranks
        messageHeaders_.resize(comm_.size());
        for( int r=0; r<comm_.size(); ++r) {
            if( r != comm_.rank() ) {// skip own rank
                messageHeaders_[r].resize(headerSize_());
            }
        }
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
        endIndex(msgid) = ibegin + convertSizeInBytes<8,8>(nBytes);
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

    int MessageBox:: rank () const { return Communicator(comm_).rank(); }
    int MessageBox::nranks() const { return Communicator(comm_).size(); }


    Index_t*
    MessageBox::
    getHeaderFrom(int rank)
    {// header is stored in this->messageHeaders[rank], a std::vector<Index_t> which was
     // appropriately sized in the MessageBox ctor.
        int success =
        MPI_Get
          ( messageHeaders_[rank].data() // buffer to store the elements to get
          , headerSize_()                // size of that buffer (number of elements0
          , MPI_LONG_LONG_INT            // type of that buffer
          , rank                         // process rank to get from (target)
          , 0                            // offset in targets window
          , headerSize_()                // number of elements to get
          , MPI_LONG_LONG_INT            // data type of that buffer
          , window_                      // window
          );
        if( success != MPI_SUCCESS ) {
            std::string errmsg = comm_.str() + "MPI_get failed (getting header).";
            throw std::runtime_error(errmsg);
        }

        return messageHeaders_[rank].data();
    }

    void
    MessageBox::
    getHeaders()
    {
        for( int r=0; r<comm_.size(); ++r) {
            if( r != comm_.rank() ) {// skip own rank
                getHeaderFrom(r);
            }
        }
    }

    std::string
    MessageBox::
    headersStr() const
    {
        std::stringstream ss;
        Communicator comm = Communicator(comm_);
        ss<<comm_.str()<<"\nheaders:";
        for( int r=0; r<comm_.size(); ++r) {
            if( r != comm_.rank() ) {// skip own rank
                Index_t nMessages = messageHeaders_[r][0];
                ss<<"\n  from rank "<<r<<" : "<<nMessages<<" messages";
                Index_t const * pHeaderSection = &messageHeaders_[r][2];
                for( Index_t m=0; m<nMessages; ++m ) {
                    ss<<"\n    message "<<m<<" for rank "<<pHeaderSection[2*m]<<" ["<<pHeaderSection[2*m-1]<<','<<pHeaderSection[2*m+1]<<'[';
                }
            }
        }
        ss<<'\n';
        return ss.str();
    }


    void
    MessageBox::
    getMessages()
    {
        std::vector<std::vector<Index_t>> messages; // for now we provide for only 10 messages
        {// open an epoch
            Epoch epoch(*this,0);
            getHeaders();
            std::cout<<headersStr()<<std::endl;

            for( int r=0; r<comm_.size(); ++r) {
                if( r != comm_.rank() ) {// skip own rank
                    Index_t nMessages = messageHeaders_[r][0];
                    Index_t const * pHeaderSection = &messageHeaders_[r][2];
                    for( Index_t m=0; m<nMessages; ++m ) {
                        if( pHeaderSection[2*m] == comm_.rank() ) {// message is for me
                            Index_t const begin = pHeaderSection[2*m-1];
                            Index_t const end   = pHeaderSection[2*m+1];
                            Index_t const n = end - begin;
                            std::vector<Index_t> buffer( static_cast<size_t>(n) );
                            int success =
                            MPI_Get
                              ( buffer.data()           // buffer to store the elements to get
                              , n                       // size of that buffer (number of elements0
                              , MPI_LONG_LONG_INT       // type of that buffer
                              , r                       // process rank to get from (target)
                              , 2 + 2*maxmsgs_ + begin  // offset in targets window
                              , n                       // number of elements to get
                              , MPI_LONG_LONG_INT       // data type of that buffer
                              , window_                 // window
                              );
                            if( success != MPI_SUCCESS ) {
                                std::string errmsg = comm_.str() + "MPI_get failed (getting message).";
                                throw std::runtime_error(errmsg);
                            }
                            messages.push_back(buffer);
                        }
                    }
                }
            }
        }// epoch is closed
        for( const auto& message: messages ) {
            std::cout<<'\n'<<comm_.str();
            for(auto it = std::begin(message); it != std::end(message); ++it) {
                std::cout<<*it<<' ';
            }
            std::cout<<'\n'<<comm_.str();
            for(auto ptr=message.data(); ptr!=message.data()+message.size(); ++ptr) {
                std::cout<<*reinterpret_cast<double const *>(ptr)<<' ';
            }
            std::cout<<std::endl;
        }
    }
 //------------------------------------------------------------------------------------------------
 // class Epoch implementation
 //------------------------------------------------------------------------------------------------
    Epoch::
    Epoch(MessageBox& mb, int assert=0) // not sure whether assert is useful
      : mb_(mb)
    {
        MPI_Win_fence(assert, mb_.window());
    }

    Epoch::
    ~Epoch()
    {
        MPI_Win_fence(0, mb_.window());
    }