#include "MessageBuffer.h"
#include "MessageHandler.h"

#include <iostream>
#include <sstream>
#include <iomanip>

#define FILL_BUFFER

namespace mpi12s
{   
 //------------------------------------------------------------------------------------------------
 // Implementation of class MessageBuffer       
 //------------------------------------------------------------------------------------------------
    MessageBuffer::
    MessageBuffer()
      : pBuffer_(nullptr)
      , bufferSize_(0)
      , bufferOwned_(false)
      , headersOnly_(false)
      , maxmsgs_(0)
    {}

    MessageBuffer::
    ~MessageBuffer()
    {
        if constexpr(::mpi12s::_verbose_)
            prdbg( tostr("~MessageBuffer(), pBuffer_=", pBuffer_, ", bufferSize_=", bufferSize_, ", bufferOwned_"
                        , bufferOwned_, (bufferOwned_ ? " (to be deleted)." : "")
                        )
                 );

        if( bufferOwned_ )
            delete[] pBuffer_;
    }

    void 
    MessageBuffer::
    initialize
      ( size_t size     // amount to be allocated for the messages, not counting the memory for the header section
      , size_t max_msgs // maximum number of messages that can be stored.
      )
    {
        headersOnly_ = (size == 0);
        bufferSize_ = 1 + max_msgs * HEADER_SIZE + size;
        pBuffer_ = new Index_t[bufferSize_];
        bufferOwned_ = true;
        maxmsgs_ = max_msgs;
        initialize_();
    }

    void
    MessageBuffer::
    initialize
      ( Index_t * pBuffer // pointer to pre-allocated memory
      , size_t size       // amount of pre-allocated memory 
      , size_t max_msgs   // maximum number of messages that can be stored.
      )
    {
        pBuffer_ = pBuffer;
        bufferSize_ = size;
        bufferOwned_ = false; // buffer is owned by whoever allocated it (typeically MPI_Win_alloc)
        maxmsgs_ = max_msgs;
        initialize_();
    }

    void 
    MessageBuffer::
    initialize_()
    {
        pBuffer_[0] = 0; // initially, there are no messages.
        setMessageBegin( 0, 1 + HEADER_SIZE * maxmsgs_ ); // Begin of the first message, and the size of the message header section.
      #ifdef FILL_BUFFER   
        for( Index_t i = 2; i < bufferSize_; ++i ) {
            pBuffer_[i] = -1;
        }
      #endif
        // std::cout<<"MessageBuffer()::initialize_()"<<pBuffer_<<'/'<<bufferSize_<<'/'<<bufferOwned_<<std::endl;
    }

    void MessageBuffer::clear()
    {// To clear the MessageBuffer, it suffices to set the number of messages to 0
        pBuffer_[0] = 0;
    }

    void*                                 // returns pointer to the reserved memory in the MessageBuffer
    MessageBuffer::
    allocateMessage
      ( Index_t  sz                       // the size of the message, in bytes
      , int      from_rank                // the source of the message
      , int      to_rank                  // the destination of the message
      , ::mpi12s::MessageHandlerKey_t key // the key of the object responsible for reading the message
      , Index_t* the_msgid                // on return contains the id of the allocated message, if provided
      )
    {
        // std::cout<<headersToStr(true)<<std::endl; // debugging

        Index_t msgid = nMessages();
        if( the_msgid ) {
            *the_msgid = msgid;
        }
        incrementNMessages();
        
        setMessageSource     (msgid, from_rank);
        setMessageDestination(msgid, to_rank);
        setMessageHandlerKey (msgid, key);

     // The begin of the message is already set. Only the end must be set.
        Index_t szIndex_t = (sz + (sizeof(Index_t) - 1))/sizeof(Index_t);
        Index_t end = messageBegin(msgid) + szIndex_t;
        setMessageEnd( msgid, end );
     // Set the begin of the next message (so that the comment above is remains true).
     // The begin of the next message is end of this message.
        setMessageBegin( msgid + 1, end ); 

        // std::cout<<headersToStr(true)<<std::endl; // debugging

        return ( headersOnly_ ? nullptr : messagePtr(msgid) );
    }

    std::vector<std::string> // list of lines
    MessageBuffer::
    headersToStr(bool verbose) const
    {
        std::stringstream ss;
        std::vector<std::string> lines;
        lines.push_back("MessageBuffer::headersToStr() :");

        ss<<std::setw(5)<<"id"<<std::setw(20)<<"from"<<std::setw(20)<<"to"<<std::setw(20)<<"key"<<std::setw(20)<<"begin"<<std::setw(20)<<"end";
        lines.push_back(ss.str()); ss.str(std::string());

        Index_t n = (verbose ? maxMessages() : nMessages());
        for( Index_t i = 0; i < n; ++i ) {

            ss<<std::setw( 5)<<( i < maxMessages() ? i : -i)
              <<std::setw(20)<<messageSource     (i)
              <<std::setw(20)<<messageDestination(i)
              <<std::setw(20)<<messageHandlerKey (i)
              <<std::setw(20)<<messageBegin      (i)
              <<std::setw(20)<<messageEnd        (i)
              ;
        lines.push_back(ss.str()); ss.str(std::string());
        }
        return lines;
    }

    std::vector<std::string> // list of lines
    MessageBuffer::
    messageToStr
      ( Index_t msg_id // message index
      )
    {// incomplete implementation. Must make sure that strings are padded to four byte boundaries
        std::vector<std::string> lines;

        std::stringstream ss;
        ss<<"MessageBuffer::messageToStr(msg_id="<<msg_id<<") (raw buffer view)";
        lines.push_back(ss.str()); ss.str(std::string());

        ss<<std::setw( 7)<<"offset"
          <<std::setw(20)<<"ptr"
          <<std::setw(20)<<"size_t"
          <<std::setw(20)<<"float"
          <<std::setw(20)<<"double"
          ;
        lines.push_back(ss.str()); ss.str(std::string());

        Index_t msg_szb = messageSize(msg_id); // in bytes
        void*   msg_ptr = messagePtr (msg_id);
        for( Index_t offset=0; offset < msg_szb; offset += sizeof(float) ) {
            ss<<std::setw( 7)<<offset
              <<std::setw(20)<<static_cast<float*>(msg_ptr) + offset/sizeof(float)
              <<std::setw(24)<<interpretAs<size_t>(msg_ptr, offset)
              <<std::setw(20)<<interpretAs<float  >(msg_ptr, offset)
              <<std::setw(20)<<interpretAs<double >(msg_ptr, offset)
              ;
            lines.push_back(ss.str()); ss.str(std::string());
        }
        return lines;
    }

 // Broadcast my headers to all other processes, process the headers and
 // fetch the messages which are for me.
    void
    MessageBuffer::
    broadcast()
    {// broadcast the size of the header section of all processes
        std::vector<Index_t> nmessages_per_rank(mpi12s::size);
        nmessages_per_rank[mpi12s::rank] = nMessages();
        for( int source = 0; source < mpi12s::size; ++source ) {
            MPI_Bcast(&nmessages_per_rank[source], 1, MPI_LONG_LONG_INT, source, MPI_COMM_WORLD);
        }
     // print the number of messages per rank:
        if constexpr(::mpi12s::_debug_) {
            Lines_t lines;
            std::stringstream ss;
            for( size_t i = 0; i < nmessages_per_rank.size(); ++i ) {
                ss<<"rank"<<i<<std::setw(6)<<nmessages_per_rank[i];
                lines.push_back(ss.str()); ss.str(std::string());    
            }
            prdbg( tostr("broadcast(): numbe of essages in each rank:"), lines );
        }

     // Broadcast the header section of all processes
     // All the headers to appear after each other, therefore the buffer location depends on the proces
     // Also note that we do NOT want to send the first entry of the buffer as this contains the number
     // of messages in the header.
        for( int source = 0; source < mpi12s::size; ++source ) {
            if( source == ::mpi12s::rank)
            {// this process is the root of the broadcast operation (=sender)
                MPI_Bcast
                ( &pBuffer_[1]                           // the headers to be sent start here
                                                         // this is the source
                , HEADER_SIZE*nmessages_per_rank[source]              // number of Index_t items to be sent
                , MPI_LONG_LONG_INT                      // MPI equivalent of Index_t
                , source                                 // source rank, equals ::mpi12s::rank
                , MPI_COMM_WORLD
                );
            } else
            {// This process is a listener to the broadcast operation (=receiver)
                MPI_Bcast
                ( &pBuffer_[1 + nMessages()*HEADER_SIZE] // this is the destination
                , HEADER_SIZE*nmessages_per_rank[source]              // number of Index_t items to be received from source rank
                , MPI_LONG_LONG_INT                      // MPI equivalent of Index_t
                , source                                 // source rank
                , MPI_COMM_WORLD
                );
             // We have now received the headers from the source rank. Note that the messageBegin and messageEnd
             // entries in these refer to the begin and end of the message in the messageBuffer of the source rank
             // and not in the messageBuffer of this rank. However, at this point we cannot update these locations
             // as only the messages for this rank need to be transferred.
             // We must however update the the number of message in this messageBuffer's header section:
                incrementNMessages( nmessages_per_rank[source] );
            }
        }
     // print the headers:
        if constexpr(::mpi12s::_debug_ && _debug_) {
            prdbg("MessageBuffer::broadcast() : headers transferred:", headersToStr());
        }

     // Loop over the message headers in this messageBuffer.
     //   If its source is this rank, then send it to its destination: MPI_Isend (non-blocking)
     //   If its destination is this rank then receive it from this rank: MPI_Recv (blocking)
     //   update the header if needed to have the correct locations of the begin and end of the message.
     //
     // What do we use as a tag? We need it as there may be several messages between the same rank.
     // Since no process can make two message with the same messageHandlerKey, the latter can be used as a tag.
     // The triplet (source rank, destination rank, messageHandlerKey) is unique.
     //
     // We first want do all the sends, non-blocking, then all the receives, blocking.
     // This is automatically satisfied because all the sends are at the beginning of the
     // messageBuffer.
        int elements_added = 0;
        for( Index_t msg_id = 0; msg_id < nMessages(); ++msg_id)
        {
            if( messageSource(msg_id) == ::mpi12s::rank )
            {// send the message content
                if constexpr(::mpi12s::_debug_ && _debug_) {
                    prdbg( tostr("MessageBuffer::broadcast() : sending   message content "
                                , messageSource(msg_id), "->", messageDestination(msg_id)
                                , ", key=", messageHandlerKey(msg_id))
                         );
                }
                MPI_Request request;
                int success =
                MPI_Isend                                       // non-blocking
                  ( messagePtr(msg_id)                          // pointer to buffer to send
                  , messageEnd(msg_id) - messageBegin(msg_id)   // number of Index_t elements to send
                  , MPI_LONG_LONG_INT                           // MPI type equivalent of Index_t
                  , messageDestination(msg_id)                  // the destination
                  , messageHandlerKey(msg_id)                   // the tag
                  , MPI_COMM_WORLD
                  , &request
                  );
            } else {
                if( messageDestination(msg_id) == ::mpi12s::rank )
                {// recv the message content
                    if constexpr(::mpi12s::_debug_ && _debug_) {
                        prdbg( tostr("MessageBuffer::broadcast() : receiving message content "
                                    , messageSource(msg_id), "->", messageDestination(msg_id)
                                    , ", key=", messageHandlerKey(msg_id))
                             );
                    }

                    int elements_to_add =  messageEnd(msg_id) - messageBegin(msg_id); // this number does not change
                    Index_t begin = messageEnd(msg_id - 1); // first element of the buffer where the message content will be written
                     // even if this is the first message to be received, the end of the previous message in the buffer is
                     // correctly set, since that is a message that was constructed on this rank and therefore the end
                     // of the previous message exists and is correct.
                    Index_t end   = begin + elements_to_add; // past-the-end element of the buffer where the message content will be written

                    int succes =
                    MPI_Recv
                      ( &pBuffer_[begin]            // pointer to buffer where to store the message
                      , elements_to_add             // number of elements to receive
                      , MPI_LONG_LONG_INT
                      , messageSource(msg_id)       // source rank
                      , messageHandlerKey(msg_id)   // tag
                      , MPI_COMM_WORLD
                      , MPI_STATUS_IGNORE
                      );
                 // Update the header of the message, so that the message content can be read afterwards.
                    setMessageBegin(msg_id, begin);
                    setMessageEnd  (msg_id, end);

                 // print the messageBuffer:
                    if constexpr(::mpi12s::_debug_ && _debug_) {
                        prdbg( tostr( "MessageBuffer::broadcast() : received message content (msg_id=", msg_id, ", "
                                    , messageSource(msg_id), "->", messageDestination(msg_id), ")"
                                    )
                             , messageToStr(msg_id)
                             );
                    }
                }
                else {
                    if constexpr(::mpi12s::_debug_ && _debug_) {
                        prdbg( tostr("MessageBuffer::broadcast() : skipping message  "
                                    , messageSource(msg_id), "->", messageDestination(msg_id)
                                    , " because it it not for this rank (", ::mpi12s::rank, ")"
                                    )
                             );
                    }
                }
            }
        }
    }

    void
    MessageBuffer::
    readMessages()
    {
     // Loop over all the received messages.
        for(Index_t msg_id = 0; msg_id < nMessages(); ++msg_id)
        {
            if( messageSource     (msg_id) != ::mpi12s::rank
             && messageDestination(msg_id) == ::mpi12s::rank
              ) {
                if constexpr(::mpi12s::_debug_ && _debug_)
                    prdbg( tostr( "MessageBuffer::readMessages() : reading message ", msg_id, "/", nMessages(), ", "
                                , messageSource(msg_id), "->", messageDestination(msg_id)
                                )
                         );
             // Fetch the MessageHandler:
                ::mpi2s::MessageHandlerBase& mh = ::mpi2s::theMessageHandlerRegistry[messageHandlerKey(msg_id)];
             // read the message
                mh.readMessage(msg_id);

                if constexpr(::mpi12s::_debug_ && _debug_)
                    prdbg( tostr( "MessageBuffer::readMessages() : read message ", msg_id, "/", nMessages(), ", "
                                , messageSource(msg_id), "->", messageDestination(msg_id)
                                )
                         , mh.message().debug_text()
                         );
            }
            else {
                if constexpr(::mpi12s::_debug_ && _debug_)
                    prdbg( tostr( "MessageBuffer::readMessages() : skipping message ", msg_id, "/", nMessages(), ", "
                                , messageSource(msg_id), "->", messageDestination(msg_id)
                                )
                         );
            }
        }
    }

 //------------------------------------------------------------------------------------------------
    MessageBuffer theMessageBuffer;
     // needs to be initialized still.
 //------------------------------------------------------------------------------------------------
}