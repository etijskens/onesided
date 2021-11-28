#include <stdexcept>
#include <sstream>
#include <iomanip>

#include "MessageBox.h"
#include "MessageHandler.h"

namespace mpi1s
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
    {
     // Create an MPI window and allocate memory for it
        size_t total_size = (1 + max_msgs * ::mpi12s::MessageBuffer::HEADER_SIZE) + bufferSize ;
                          //(headere section                                    ) + message section
        Index_t * pWindowBuffer = nullptr;
        
        int success =
            MPI_Win_allocate
                ( static_cast<MPI_Aint>( total_size * sizeof(Index_t) ) // The size of the memory area exposed through the window, in bytes.
                , sizeof(Index_t) // The displacement unit is used to provide an indexing feature during RMA operations. 
                , MPI_INFO_NULL   // MPI_Info object
                , MPI_COMM_WORLD  // MPI_Comm communicator
                , &pWindowBuffer  // pointer to allocated memory
                , &window_        // pointer to the MPI_Win object
                );
        if( success != MPI_SUCCESS ) {
            std::string errmsg = ::mpi12s::info + "MPI_Win_allocate failed.";
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
        if constexpr(::mpi12s::_verbose_) printf("\n%s~MessageBox()\n", CINFO);
        int success = MPI_Win_free(&window_);
        if constexpr(::mpi12s::_verbose_) {
            printf("%s~MessageBox(): MPI_Win_free(&window_) success = %d\n", CINFO, (success == MPI_SUCCESS));
        }
    }

    void
    MessageBox::
    getMessages()
    {
        int const my_rank = ::mpi12s::rank;
        int const nranks  = ::mpi12s::size;
     // loop over all ranks, starting with the left neighbour, and moving to the right:
        int left = ::mpi12s::next_rank(-1);
        for( int i = 0; i < nranks; ++i)
        {// Clear the readHeaders_ and readBuffer_ buffers:
            readHeaders_.clear();
            readBuffer_ .clear();

            int from_rank = (left + i) % nranks;
            if( from_rank != my_rank ) // skip my rank
            {
                if constexpr(::mpi12s::_debug_) printf("%sMessageBox::getMessages() : from_rank==%d\n", CINFO, from_rank);
                {   Epoch(*this, 0, "MessageBox::getHeaders_()");
                 // copy the header section from from_rank into the readHeaders_
                    getHeaders_(from_rank);
                }// close the epoch
                if constexpr(::mpi12s::_debug_) {
                    printf("%s MessageBox::getMessages() : readHeaders_:\n", CINFO);
//                    print_lines( readHeaders_.headersToStr() );
                }
             // The epoch is closed, so the header is available   
                {// get all messages in the header which are for me
                    Epoch(*this, 0, "for(m) { MessageBox::getMessage_(m); }");
                    for( Index_t m = 0; m < readHeaders_.nMessages(); ++m ) {
                        if( readHeaders_.messageDestination(m) == my_rank )
                        {// get the message and store it in the read buffer
                            getMessage_(m);
                        }
                    }
                }// close the Epoch
             // The epoch is closed, so the raw messages are available
             // read the messages (by fetching the appropriate MessageHandler)
                for( Index_t m = 0; m < readBuffer_.nMessages(); ++m )
                    readMessage_(m);
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
            std::string errmsg = ::mpi12s::info + "MPI_get failed, while getting header from rank " + std::to_string(from_rank) + ".";
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
            std::string errmsg = ::mpi12s::info + "MPI_get failed (getting message).";
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
        ::mpi12s::MessageHandlerKey_t key = readBuffer_.messageHandlerKey(msgid);
        void*                         ptr = readBuffer_.messagePtr       (msgid);
     // fetch the message handler
        MessageHandlerBase& messageHandler = theMessageHandlerRegistry[key];
     // read the message
        messageHandler.message().read(ptr);
    }

 //------------------------------------------------------------------------------------------------
 // class Epoch implementation
 //------------------------------------------------------------------------------------------------
    Epoch::
    Epoch
      ( MessageBox& mb
      , int assert=0    // not sure whether assert is useful
      , char const* msg // message, ignored if ::mpi12s::verbose == false.
      )
      : mb_(mb)
      , msg_(msg)
    {
        if constexpr(::mpi12s::_verbose_) printf("%sOpening MPI window (Epoch)  %s\n", CINFO, msg_ );
        MPI_Win_fence(assert, mb_.window());
    }

    Epoch::
    ~Epoch()
    {
        MPI_Win_fence(0, mb_.window());
        if constexpr(::mpi12s::_verbose_) printf("%sClosing MPI window (Epoch)  %s\n", CINFO, msg_ );
    }
 //---------------------------------------------------------------------------------------------------------------------
}// namespace mpi1s



