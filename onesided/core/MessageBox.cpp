#include <stdexcept>
#include <sstream>

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
      ( size_t max_msgs
      , size_t size // in sizeof(Index_t)
      ) : comm_(MPI_COMM_WORLD)
    {
        Index_t * pWindowBuffer = nullptr;

        int success =
        MPI_Win_allocate
          ( static_cast<MPI_Aint>(size * sizeof(Index_t)) // The size of the memory area exposed through the window, in bytes.
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
        windowBuffer_.initialize(    pWindowBuffer, size);
        readBuffer_  .initialize(new Index_t[size], size);
    }

 
 // Dtor
    MessageBox::
    ~MessageBox()
    {// free resources.     
        MPI_Win_free(&window_);
        delete[] readBuffer_.ptr();
    }


    Index_t
    MessageBox::
    postMessage(Index_t for_rank, MessageHandlerBase& messageHandler)
    {
        Index_t msgid = windowBuffer_.nMessages();
        // Index_t begin = messageBegin(msgid);
        // setMessageEnd        (msgid, begin + convertSizeInBytes<8,8>(nBytes));
        // setMessageDestination(msgid, for_rank);
        // setMessageKey        (msgid, messageHandler.key());

        // memcpy( windowBuffer_.messagePtr(msgid), bytes, nBytes );

     // Increment number of messages.
        windowBuffer_.incrementNMessages();


        return msgid;
    }


    Index_t
    MessageBox::
    getHeaderFromRank(int rank)
    {// header is stored in this->messageHeaders[rank], a std::vector<Index_t> which was
     // appropriately sized in the MessageBox ctor.
        int success =
                      0;
        // MPI_Get
        //   ( readBuffer_.ptr() // buffer to store the elements to get
        //   , headerSectionSize_()                // size of that buffer (number of elements)
        //   , MPI_LONG_LONG_INT            // type of that buffer
        //   , rank                         // process rank to get from (target)
        //   , 0                            // offset in targets window
        //   , headerSectionSize_()                // number of elements to get
        //   , MPI_LONG_LONG_INT            // data type of that buffer
        //   , window_                      // window
        //   );
        if( success != MPI_SUCCESS ) {
            std::string errmsg = comm_.str() + "MPI_get failed (getting header).";
            throw std::runtime_error(errmsg);
        }

        return -1;
    }

    void
    MessageBox::
    getHeaderFromAllRanks()
    {
        for( int r=0; r<comm_.size(); ++r) 
            if( r != comm_.rank() ) // skip own rank
                getHeaderFromRank(r);
    }

    std::string
    MessageBox::
    headerSectionToStr() const
    {
        std::stringstream ss;
        // ss<<comm_.str()<<"\nheaders:";
        // for( int r=0; r<comm_.size(); ++r) {
        //     if( r != comm_.rank() ) {// skip own rank
        //         Index_t nMessages = receivedMessageHeaders_[r][0];
        //         ss<<"\n  from rank "<<r<<" : "<<nMessages<<" messages";
        //         Index_t const * buffer = receivedMessageHeaders_[r].data();
        //         for( Index_t m=0; m<nMessages; ++m ) {
        //             ss<<"\n    message "<<m<<" for rank "<<messageDestination(m,buffer)
        //               <<" ["<<messageBegin(m,buffer)<<','<<messageEnd(m,buffer)<<'[';
        //         }
        //     }
        // }
        // ss<<'\n';
        return ss.str();
    }

    void
    MessageBox::
    getMessages()
    {
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

