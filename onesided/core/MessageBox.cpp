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
      ( Index_t  max_msgs
      , Index_t size_in_bytes
      ) : comm_(MPI_COMM_WORLD)
        , pBuffer_(NULL)
    {
        if( size_in_bytes == -1 ) {
            size_in_bytes = 10000 *  max_msgs;
        }

        bufferSize_ = convertSizeInBytes<8>(size_in_bytes); // make sure the buffer is on 64bit boundaries

        int success =
        MPI_Win_allocate
          ( static_cast<MPI_Aint>(size_in_bytes) // The size of the memory area exposed through the window, in bytes.
          , sizeof(Index_t) // The displacement unit is used to provide an indexing feature during RMA operations. 
          , MPI_INFO_NULL   // MPI_Info object
          , comm_.comm()    // MPI_Comm communicator
          , &pBuffer_       // pointer to allocated memory
          , &window_        // pointer to the MPI_Win object
          );

        if( success != MPI_SUCCESS ) {
            std::string errmsg = comm_.str() + "MPI_Win_allocate failed.";
            throw std::runtime_error(errmsg);
        }
        pBuffer_[0] = 0; // number of messages is initially 0.
        headerSize_() = 2 + 2* max_msgs;  // also the begin index of the first message.

     // reserve space for getting the headers of the other ranks
        receivedMessageHeaders_.resize(comm_.size()); // one std::vector<Index_t> per rank
        for( int r=0; r<comm_.size(); ++r) {
            if( r != comm_.rank() ) {// skip own rank
                receivedMessageHeaders_[r].resize(headerSize_());
            }
        }
    }

 // Buffer encapsulation:
 // [ nMessages 2+2* maxMsgs_ dest_of_msg_1 end_of_msg_1 dest_of_msg_2 end_of_msg_2 ... dest_of_msg_maxmsgs_ end_of_msg_maxmsgs_
 //   contents_of_msg_1 contents_of_msg_2 ...]
 // the begin of a message is the end of the previous message

    Index_t MessageBox::nMessages         (               void const * buffer) const { return static_cast<Index_t const *>(buffer)[0]; }

    int     MessageBox::messageDestination(Index_t msgid, void const * buffer) const { return static_cast<Index_t const *>(buffer)[2 + 2*msgid]; }

    Index_t MessageBox::messageBegin      (Index_t msgid, void const * buffer) const { return static_cast<Index_t const *>(buffer)[2 + 2*msgid - 1]; }

    Index_t MessageBox::messageEnd        (Index_t msgid, void const * buffer) const { return static_cast<Index_t const *>(buffer)[2 + 2*msgid + 1]; }

    Index_t MessageBox::messageSize       (Index_t msgid, void const * buffer) const {
        return messageEnd(msgid,buffer) - messageBegin(msgid,buffer);
    }
//    Index_t MessageBox::messageSizeInBytes (Index_t msgid, void const * buffer=pBuffer_) const;

    void MessageBox::
    setNMessages(Index_t n) { static_cast<Index_t*>(pBuffer_)[0] = n; }

    void MessageBox::
    setMessageDestination(Index_t msgid, Index_t messageDestination) { static_cast<Index_t*>(pBuffer_)[2 + 2*msgid] = messageDestination; }

    void MessageBox::
    setMessageEnd        (Index_t msgid, Index_t messageEnd        ) { static_cast<Index_t*>(pBuffer_)[2 + 2*msgid + 1] = messageEnd; }

 // Dtor
    MessageBox::
    ~MessageBox()
    {
        MPI_Win_free(&window_);
    }


    Index_t
    MessageBox::
    addMessage(Index_t for_rank, void* bytes, size_t nBytes)
    {
        Index_t msgid = nMessages(pBuffer_);
        Index_t begin = messageBegin(msgid,pBuffer_);
        setMessageEnd        (msgid, begin + convertSizeInBytes<8,8>(nBytes));
        setMessageDestination(msgid, for_rank);

        memcpy( &pBuffer_[begin], bytes, nBytes );

     // Increment number of messages.
        setNMessages(nMessages(pBuffer_) + 1);

        return msgid;
    }


    std::string
    MessageBox::str() const
    {
        std::stringstream ss;
        ss<<"MessageBuffer, nMessages = "<< nMessages(pBuffer_)<<'\n';
        for( int i=0; i<nMessages(pBuffer_); ++i )
        {
            ss<<"\nmessage "<<i<<" for rank "<<messageDestination(i,pBuffer_);
            Index_t j0 = messageBegin(i,pBuffer_);
            for( Index_t j=j0; j<messageEnd(i,pBuffer_); ++j) {
                Index_t val_Index_t = pBuffer_[j];
                double  val_double  = (reinterpret_cast<double*>(pBuffer_))[j];
                ss<<"\n  ["<<j-j0<<"] int = "<<val_Index_t<<", dbl = "<<val_double;
            }
        }
        ss<<'\n';
        return ss.str();
    }


    Index_t*
    MessageBox::
    getHeaderFrom(int rank)
    {// header is stored in this->messageHeaders[rank], a std::vector<Index_t> which was
     // appropriately sized in the MessageBox ctor.
        int success =
        MPI_Get
          ( receivedMessageHeaders_[rank].data() // buffer to store the elements to get
          , headerSize_()                // size of that buffer (number of elements)
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

        return receivedMessageHeaders_[rank].data();
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
        ss<<comm_.str()<<"\nheaders:";
        for( int r=0; r<comm_.size(); ++r) {
            if( r != comm_.rank() ) {// skip own rank
                Index_t nMessages = receivedMessageHeaders_[r][0];
                ss<<"\n  from rank "<<r<<" : "<<nMessages<<" messages";
                Index_t const * buffer = receivedMessageHeaders_[r].data();
                for( Index_t m=0; m<nMessages; ++m ) {
                    ss<<"\n    message "<<m<<" for rank "<<messageDestination(m,buffer)
                      <<" ["<<messageBegin(m,buffer)<<','<<messageEnd(m,buffer)<<'[';
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
        messages.clear();
        {// open an epoch
            Epoch epoch(*this,0);
            getHeaders();

            for( int r=0; r<comm_.size(); ++r)
            {
                if( r != comm_.rank() )
                {// skip own rank
                    Index_t const * headers = receivedMessageHeaders_[r].data();
                    for( Index_t m=0; m<nMessages(headers); ++m )
                    {
                        if( messageDestination(m,headers) == comm_.rank() )
                        {// This is a message for me
                            Index_t const message_size = messageSize(m,headers);
                            msgbuf_type message_buffer( static_cast<std::string::size_type>(message_size*sizeof(Index_t)), '\0' );
                            int success =
                            MPI_Get
                              ( message_buffer.data()   // buffer to store the elements to get
                              , message_size            // size of that buffer (number of elements0
                              , MPI_LONG_LONG_INT       // type of that buffer
                              , r                       // process rank to get from (target)
                              , messageBegin(m,headers) // offset in targets window
                              , message_size            // number of elements to get
                              , MPI_LONG_LONG_INT       // data type of that buffer
                              , window_                 // window
                              );
                            if( success != MPI_SUCCESS ) {
                                std::string errmsg = comm_.str() + "MPI_get failed (getting message).";
                                throw std::runtime_error(errmsg);
                            }
                            messages.push_back(message_buffer);
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

 //---------------------------------------------------------------------------------------------------------------------
 // generic version for encoding and decoding messages with the same function calls
 //---------------------------------------------------------------------------------------------------------------------
    int const not_implemented =-1;
    int const built_in_type   = 0;
    int const array_like_type = 1;

    template<typename T> struct handler_kind {
        static int const value = not_implemented;
    };

    template<> struct handler_kind<int>                      { static int const value = built_in_type; };
    template<> struct handler_kind<unsigned int>             { static int const value = built_in_type; };
    template<> struct handler_kind<Index_t>                  { static int const value = built_in_type; };
    template<> struct handler_kind<float>                    { static int const value = built_in_type; };
    template<> struct handler_kind<double>                   { static int const value = built_in_type; };
    template<typename T> struct handler_kind<std::vector<T>> { static int const value = array_like_type; };
    template<> struct handler_kind<std::string>              { static int const value = array_like_type; };// C++17 required

 //---------------------------------------------------------------------------------------------------------------------
 // MessageStreamHelper class
 // This class does the work of the MessageStream class
 //---------------------------------------------------------------------------------------------------------------------
 // Only specialisations are functional.
    template<typename T, int> class MessageStreamHelper;

 //---------------------------------------------------------------------------------------------------------------------
 // MessageStreamHelper specialisation for selected built-in types
 //---------------------------------------------------------------------------------------------------------------------
    template<typename T>
    class MessageStreamHelper<T,built_in_type>
    {
        typedef MessageStreamHelper<T,built_in_type> This_type;
    public:
     // Write or read a T object, select using mode argument
     // Even when mode == 'w', t must be non-const.
        static void handle(T& t, std::stringstream& ss, char mode)
        {
            std::cout<<"handle not is_array_like, mode="<<mode<<std::endl;
            if (mode == 'w') {
                This_type::write(const_cast<T const &>(t),ss);
            } else if (mode == 'r') {
                This_type::read(t,ss);
            } else {
                throw std::invalid_argument("Mode should be 'r' (read) or 'w' (write).");
            }
        }
        
    private:
        static
        void write(T const & t, std::stringstream& ss)
        {
            std::cout<<"write not is_array_like"<<std::endl;
            ss.write( reinterpret_cast<std::ostringstream::char_type const *>(&t), sizeof(T) );
        }
        
        static
        void read(T & t, std::stringstream& ss)
        {
            std::cout<<"read not is_array_like"<<std::endl;
            ss.read( reinterpret_cast<std::ostringstream::char_type *>(&t), sizeof(T) );
        }
    };

 //---------------------------------------------------------------------------------------------------------------------
 // MessageStreamHelper specialisation for selected array-like types
 //---------------------------------------------------------------------------------------------------------------------
 // class T must have
 //   - T::value_type, the type of the array elements
 // T objects t must have
 //   - t.data() returning T::value_type* or T::value_type const*, pointer to begin of te data buffer
 //   - t.size() returning size_t, number of array elements
 // This is ok for std::vector<T> with T a built-in type , std::string
    template<typename T> 
    class MessageStreamHelper<T,array_like_type>
    {
        typedef MessageStreamHelper<T,array_like_type> This_type;
    public:
     // Write or read a T object, select using mode argument
     // Even when mode == 'w', t must be non-const.
        static void handle(T& t, std::stringstream& ss, char mode)
        {
            std::cout<<"handle is_array_like, mode="<<mode<<std::endl;
            if (mode == 'w') {
                This_type::write(const_cast<T const &>(t),ss);
            } else if (mode == 'r') {
                This_type::read(t,ss);
            }
        }
    
    private:
     // write a T object
        static
        void write(T const & t, std::stringstream& ss)
        {
            std::cout<<"write is_array_like"<<std::endl;
            size_t const sz = t.size();
            std::cout<<"write size: "<<sz<<std::endl;
            ss.write(reinterpret_cast<std::stringstream::char_type const *>(&sz), sizeof(size_t));
            ss.write(reinterpret_cast<std::stringstream::char_type const *>(t.data()), sz*sizeof(typename T::value_type));
            std::cout<<"write done "<<std::endl;
        }

     // read a T object
        static
        void read(T & t, std::stringstream& ss)
        {
            std::cout<<"read is_array_like"<<std::endl;
            size_t sz=0;
            ss.read(reinterpret_cast<std::ostringstream::char_type*>(&sz), sizeof(size_t));
            std::cout<<"read.size "<<sz<<std::endl;
            t.resize(sz);
            std::cout<<"read size: "<<t.size()<<std::endl;
            ss.read(reinterpret_cast<std::ostringstream::char_type*>(t.data()), sz*sizeof(typename T::value_type));
            std::cout<<"read done"<<std::endl;
        }
        
    };

 //---------------------------------------------------------------------------------------------------------------------
 // class MessageStream
 //---------------------------------------------------------------------------------------------------------------------
   class MessageStream
    {
      public:
        MessageStream() {}

//     // write T object
//        template<typename T>
//        void
//        write(T const & t) {
//            MessageStreamHelper<T,handler_kind<T>::value>::write(t, ss_);
//        }
//     // read T object
//        template<typename T>
//        void
//        read(T & t) {
//            MessageStreamHelper<T,handler_kind<T>::value>::read(t, ss_);
//        }
     // write or read T object, select with mode argument
        template<typename T>
        void
        handle(T & t, char mode) {
            MessageStreamHelper<T,handler_kind<T>::value>::handle(t, ss_, mode);
        }
     // get the message as e string to put it to the window
        std::string str() const { return ss_.str(); }

      private: // data members
        std::stringstream ss_;
    };
 //---------------------------------------------------------------------------------------------------------------------

