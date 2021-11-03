#include <stdexcept>
#include <sstream>
#include <iomanip>

#include "MessageBox.h"

namespace mpi
{
 //------------------------------------------------------------------------------------------------
 // Convert a size in bytes, ensuring that the result is rounded up to Boundary. The result is
 // expressed in word of Unit bytes.
 // E.g.  convertSizeInBytes<8>(4) -> 8 : the smallest 8 byte boundary >= 4 bytes is 8 bytes
 // E.g.  convertSizeInBytes<8,8>(4) -> 8 : the smallest 8 byte boundary >= 4 bytes is 1 8-byte word
 // E.g.  convertSizeInBytes<8,2>(4) -> 8 : the smallest 8 byte boundary >= 4 bytes is 4 2-byte words
    // template<size_t Boundary, size_t Unit=1>
    // Index_t convertSizeInBytes(Index_t bytes) {
    //     return ((bytes + Boundary - 1) / Boundary) * (Boundary / Unit);
    // }


 //------------------------------------------------------------------------------------------------
 // class MessageBox implementation
 //------------------------------------------------------------------------------------------------
    MessageBox::
    MessageBox
      ( size_t size // in sizeof(Index_t)
      , size_t max_msgs
      ) 
      : comm_(MPI_COMM_WORLD)      
    {
     // Create an MPI window and allocate memory for it
        size_t total_size = size + 1 + max_msgs * MessageBuffer::HEADER_SIZE;
        Index_t * pWindowBuffer = nullptr;
        
        int success =
            MPI_Win_allocate
                ( static_cast<MPI_Aint>( total_size * sizeof(Index_t) ) // The size of the memory area exposed through the window, in bytes.
                , sizeof(Index_t) // The displacement unit is used to provide an indexing feature during RMA operations. 
                , MPI_INFO_NULL   // MPI_Info object
                , comm_.comm()    // MPI_Comm communicator
                , &pWindowBuffer  // pointer to allocated memory
                , &window_        // pointer to the MPI_Win object
                );
        if( success != MPI_SUCCESS ) {
            std::string errmsg = comm_.str() + "MPI_Win_allocate failed.";
            throw std::runtime_error(errmsg);
        }

     // Initialize the window buffer with the memory allocated by MPI_Win_allocate
        windowBuffer_.initialize( pWindowBuffer, total_size, max_msgs );

     // Allocate memory for the read buffer and initialize it
        readHeader_.initialize( 0, max_msgs );

     // Allocate memory for the read buffer and initialize it
        readBuffer_.initialize( size, max_msgs );
    }

 
 // Dtor
    MessageBox::
    ~MessageBox()
    {// free resources.     
        MPI_Win_free(&window_);
    }

    void*
    MessageBox::
    allocateMessage(Index_t sz, int from_rank, int to_rank, size_t key, Index_t* the_msgid )
    {
        std::cout<<windowBuffer_.headers(true)<<std::endl;

        Index_t msgid = windowBuffer_.nMessages();
        if( the_msgid ) {
            *the_msgid = msgid;
        }
        windowBuffer_.incrementNMessages();
        
        windowBuffer_.setMessageSource     (msgid, from_rank);
        windowBuffer_.setMessageDestination(msgid, to_rank);
        windowBuffer_.setMessageHandlerKey (msgid, key);

     // The begin of the message is already set. Only the end must be set. 
        Index_t end = windowBuffer_.messageBegin(msgid) + sz;
        windowBuffer_.setMessageEnd( msgid, end );
     // Set the begin of the next message (so that the comment above is remains true).
     // The begin of the next message is end of this message.
        windowBuffer_.setMessageBegin( msgid + 1, end ); 

        std::cout<<windowBuffer_.headers(true)<<std::endl;

        return windowBuffer_.messagePtr(msgid);
    }

    void
    MessageBox::
    getMessages()
    {
        int const my_rank = comm().rank();
        int const nranks  = comm().size();
     // loop over all ranks, starting with the left neighbour:
        int left = (my_rank - 1 + nranks) % nranks;
        for( int i = 0; i < nranks; ++i)
        {
            int from_rank = (left + i) % nranks;
            if( from_rank != my_rank ) // skip my rank
            {
                std::cout<<"from_rank="<<from_rank<<std::endl;
                {   Epoch(*this,0);
                    getHeader(from_rank);
                    for( Index_t m = 0; m < readBuffer_.nMessages(); ++m ) {
                        if( readBuffer_.messageDestination(m) == my_rank ) {
                            getMessage(from_rank, m);
                        }
                    }
                }
            }
        }  

    }

    void
    MessageBox::
    getHeader(int from_rank)
    {
        int success =
        MPI_Get
          ( readBuffer_.ptr()           // buffer to store the elements to get
          , readBuffer_.headerSize()    // size of that buffer (number of elements)
          , MPI_LONG_LONG_INT           // type of that buffer
          , from_rank                   // process rank to get from (target)
          , 0                           // offset in targets window
          , windowBuffer_.headerSize()  // number of elements to get
          , MPI_LONG_LONG_INT           // data type of that buffer
          , window_                     // window
          );
        if( success != MPI_SUCCESS ) {
            std::string errmsg = comm_.str() + "MPI_get failed, while getting header from rank " + std::to_string(from_rank) + ".";
            throw std::runtime_error(errmsg);
        }
    }

    void
    MessageBox::
    getMessage(int from_rank, Index_t from_msgid)
    {
        Index_t to_msgid = readBuffer_.nMessages();
        int success =
        MPI_Get
            ( readBuffer_.messagePtr (to_msgid)    // buffer to store the elements to get
            , readBuffer_.messageSize(to_msgid)    // size of that buffer (number of elements0
            , MPI_LONG_LONG_INT                    // type of that buffer
            , from_rank                            // process rank to get from (target)
            , readHeader_.messageBegin(from_msgid) // offset in targets window
            , readHeader_.messageSize (from_msgid) // number of elements to get
            , MPI_LONG_LONG_INT                    // data type of that buffer
            , window_                              // window
            );
        if( success != MPI_SUCCESS ) {
            std::string errmsg = comm_.str() + "MPI_get failed (getting message).";
            throw std::runtime_error(errmsg);
        }
        readBuffer_.incrementNMessages();
    }

    // void
    // MessageBox::
    // getMessages()
    // {
        // messages.clear();
        // {// open an epoch
        //     Epoch epoch(*this,0);
        //     getHeaders();

        //     for( int r=0; r<comm_.size(); ++r)
        //     {
        //         if( r != comm_.rank() )
        //         {// skip own rank
        //             Index_t const * headers = receivedMessageHeaders_[r].data();
        //             for( Index_t m=0; m<nMessages(headers); ++m )
        //             {
        //                 if( messageDestination(m,headers) == comm_.rank() )
        //                 {// This is a message for me
        //                     Index_t const message_size = messageSize(m,headers);
        //                     msgbuf_type message_buffer( static_cast<std::string::size_type>(message_size*sizeof(Index_t)), '\0' );
        //                     int success =
        //                     MPI_Get
        //                       ( message_buffer.data()   // buffer to store the elements to get
        //                       , message_size            // size of that buffer (number of elements0
        //                       , MPI_LONG_LONG_INT       // type of that buffer
        //                       , r                       // process rank to get from (target)
        //                       , messageBegin(m,headers) // offset in targets window
        //                       , message_size            // number of elements to get
        //                       , MPI_LONG_LONG_INT       // data type of that buffer
        //                       , window_                 // window
        //                       );
        //                     if( success != MPI_SUCCESS ) {
        //                         std::string errmsg = comm_.str() + "MPI_get failed (getting message).";
        //                         throw std::runtime_error(errmsg);
        //                     }
        //                     messages.push_back(message_buffer);
        //                 }
        //             }
        //         }
        //     }
        // }// epoch is closed
//        for( const auto& message: messages ) {
//            std::cout<<'\n'<<comm_.str();
//            for(auto it = std::begin(message); it != std::end(message); ++it) {
//                std::cout<<*it<<' ';
//            }
//            std::cout<<'\n'<<comm_.str();
//            for(auto ptr=message.data(); ptr!=message.data()+message.size(); ++ptr) {
//                std::cout<<*reinterpret_cast<double const *>(ptr)<<' ';
//            }
//            std::cout<<std::endl;
//        }
    // }
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
}// namespace mpi


//  //---------------------------------------------------------------------------------------------------------------------
//  // class MessageStream
//  //---------------------------------------------------------------------------------------------------------------------
//    class MessageStream
//     {
//       public:
//         MessageStream() {}

//      // write or read T object, select with mode argument
//         template<typename T>
//         void
//         handle(T & t, char mode) {
//             MessageStreamHelper<T,handler_kind<T>::value>::handle(t, ss_, mode);
//         }
//      // get the message as e string to put it to the window
//         std::string str() const { return ss_.str(); }

//         void set(std::string const& s) {
//             ss_.clear();
//             ss_<<s;
//         }

//       private: // data members
//         std::stringstream ss_;
//     };
 //---------------------------------------------------------------------------------------------------------------------

