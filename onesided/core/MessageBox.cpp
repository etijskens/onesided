#include <stdexcept>
#include <sstream>
#include <iomanip>

#include "MessageBox.h"
#include "MessageHandler.h"

namespace mpi
{
 //------------------------------------------------------------------------------------------------
 // class MessageBox implementation
 //------------------------------------------------------------------------------------------------
    MessageBox::
    MessageBox
      ( size_t bufferSize // the size of the message section of the buffer, in Index_t words.
      , size_t max_msgs   // maximum number of messages that can be stored. 
                          // This defines the size of the header section.
      )
      : comm_(MPI_COMM_WORLD)      
    {
     // Create an MPI window and allocate memory for it
        size_t total_size = (1 + max_msgs * MessageBuffer::HEADER_SIZE) + bufferSize ;
                          //(headere section                          ) + message section
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

     // Allocate memory for reading remote headers and initialize it
        readHeaders_.initialize( 0, max_msgs );

     // Allocate memory for the read buffer and initialize it
        readBuffer_.initialize( bufferSize, max_msgs );
    }

 
 // Dtor
    MessageBox::
    ~MessageBox()
    {// free resources.     
        MPI_Win_free(&window_);
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
        {// Clear the readHeaders_ and readBuffer_ buffers:
            readHeaders_.clear();
            readBuffer_ .clear();

            int from_rank = (left + i) % nranks;
            if( from_rank != my_rank ) // skip my rank
            {
                std::cout<<"from_rank="<<from_rank<<std::endl;
                {   Epoch(*this,0);
                 // copy the header section from from_rank into the readHeaders_
                    getHeaders_(from_rank);
                }// close the epoch

             // The epoch is closed, so the header is available   
                {// get all messages in the header which are for me
                    Epoch(*this,0);
                    for( Index_t m = 0; m < readHeaders_.nMessages(); ++m ) {
                        if( readHeaders_.messageDestination(m) == my_rank )
                        {// get the message and store it in the read buffer
                            getMessage_(m);
                        }
                    }
                }// close the Epoch

             // The epoch is closed, so the raw messages are available   
             // Fetch the message handler for the message and read the message
                for( Index_t m = 0; m < readBuffer_.nMessages(); ++m ) {
                    readMessage_(m);
                }
            }
        }  
    }

    void
    MessageBox::
    getHeaders_
      (int from_rank // rank to get the header from
      )
    {// copy the header section from from_rank into the readHeaders_
        int success =
        MPI_Get
          ( readHeaders_.ptr()          // buffer to store the elements to get
          , readHeaders_.headerSize()   // size of that buffer (number of elements)
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

    Index_t                // id of the received message, which is now stored in readBuffers_
    MessageBox::
    getMessage_
      ( Index_t from_msgid // id of the message to get, the corresponding header is already in readHeaders_
      )
    {// The message is copied from the windowBuffer_ in the remote process to the readBuffer_ in
     // this process. obviously we cannot just copy the header. A new header must be created.

        Index_t to_msgid = -1;
        void* to_ptr = readBuffer_.allocateMessage
            ( readHeaders_.messageSize       (from_msgid)
            , readHeaders_.messageSource     (from_msgid)
            , readHeaders_.messageDestination(from_msgid)
            , readHeaders_.messageHandlerKey (from_msgid)
            , &to_msgid
            );

        int success =
        MPI_Get
            ( readBuffer_.messagePtr (to_msgid)      // buffer to store the elements to get
            , readBuffer_.messageSize(to_msgid)      // size of that buffer (number of elements0
            , MPI_LONG_LONG_INT                      // type of that buffer
            , readHeaders_.messageSource(from_msgid) // process rank to get from (target)
            , readHeaders_.messageBegin (from_msgid) // offset in targets window
            , readHeaders_.messageSize  (from_msgid) // number of elements to get
            , MPI_LONG_LONG_INT                      // data type of that buffer
            , window_                                // window
            );
        if( success != MPI_SUCCESS ) {
            std::string errmsg = comm_.str() + "MPI_get failed (getting message).";
            throw std::runtime_error(errmsg);
        }
        return to_msgid;        
    }

    void 
    MessageBox::
    readMessage_
      ( Index_t msgid // message id in readBuffer_, from which the message is to 
                      // be read with the appropriate message handler
      )
    {
        MessageHandlerKey_t key = readBuffer_.messageHandlerKey(msgid);
        void*               ptr = readBuffer_.messagePtr       (msgid);
     // fetch the message handler
        MessageHandlerBase& messageHandler = theMessageHandlerRegistry[key];
     // read the message
        messageHandler.message().read(ptr);
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

