#ifndef MESSAGEBUFFER_H
#define MESSAGEBUFFER_H

#include <vector>
#include <sstream>
#include <iomanip>
#include "types.h"


namespace mpi12s
{
    template<typename T>
    T&
    interpretAs
      ( void * ptr    // ptr to begin of a message in a buffer
      , size_t offset // offset relative to ptr, in bytes
      )
    {
        T* ptrT = (T*)((char*)(ptr) + offset );
        return *ptrT;
    }
 //------------------------------------------------------------------------------------------------
    class MessageBuffer
 // This class encapsulates the buffer for reading and writing messages.  
 // A message consists of a header section and a message section. The header section describes 
 // the messages and their location in the message section.
 // Used for both the window buffer, and the receiving buffer
 //------------------------------------------------------------------------------------------------
    {
        static bool const _debug_ = true;
    public:
     // This enum enumerates the consecutive items in a single message header, plus the HEADER_SIZE
     // entry which must be the last.
        enum { MSG_BGN = 0 // first entry must be 0
             , MSG_END
             , MSG_SRC
             , MSG_DST
             , MSG_KEY
             , HEADER_SIZE // must be last entry.
             };
         MessageBuffer();
        ~MessageBuffer();
     // allocate memory for the buffer:
        void 
        initialize
          ( size_t size     // amount to be allocated for the messages, not counting the memory for the header section
          , size_t max_msgs // maximum number of messages that can be stored.
          );
     // Assign pre-allocated memory for the buffer
        void 
        initialize
          ( Index_t * pBuffer // pointer to pre-allocated memory
          , size_t size       // amount of pre-allocated memory 
          , size_t max_msgs   // maximum number of messages that can be stored.
          );

     // clear the MessageBuffer
        void clear();

     // Allocate resources for a message in the MessageBuffer: 
     //   - reserve space for a message of size sz to be posted
     //   - write a header for that message in the buffer
        void*                                 // returns pointer to the reserved memory in the MessageBuffer, or
                                              // nullptr if this is a headers only buffer
        allocateMessage
          ( Index_t  sz                       // the size of the message, in bytes
          , int      from_rank                // the source of the message (=MPI rank)
          , int      to_rank                  // the destination of the message (=MPI rank)
          , ::mpi12s::MessageHandlerKey_t key   // the key of the object responsible for reading the message
          , Index_t* msgid = nullptr          // on return contains the id of the allocated message, if provided
          );

     // Broadcast my headers to all other processes, process the headers and
     // fetch the messages which are for me.
     // This function must be called on all processes.
        void broadcast();

     // Read all the messages (to be called after broadcast()).
        void readMessages();

     // Member functions for reading message headers from a buffer (getters).
     // This can be the buffer in the MPI window of this MessageBox, or a buffer 
     // read from other processes' MPI window. (see member functions getHeaderFromRank
     // and getHeaderFromAllRanks below).
     // Getters:
        inline Index_t   nMessages() const { return  pBuffer_[0]; }
        inline Index_t maxMessages() const { return maxmsgs_; }
        inline Index_t headerSize () const { return (pBuffer_[1]); }
         // pBuffer_[1] contains the location of the begin of the first message. For that reason it
         // is equal to the size of the header section. It is, however, NOT the size of the part of
         // the header section that is in used.
         inline Index_t headerSizeUsed() const { return 1 + nMessages()*HEADER_SIZE; }

        inline Index_t messageBegin       (Index_t msgid) const { return pBuffer_[1 + HEADER_SIZE * msgid + MSG_BGN]; }
        inline Index_t messageEnd         (Index_t msgid) const { return pBuffer_[1 + HEADER_SIZE * msgid + MSG_END]; }
        inline int     messageDestination (Index_t msgid) const { return pBuffer_[1 + HEADER_SIZE * msgid + MSG_DST]; }
        inline int     messageSource      (Index_t msgid) const { return pBuffer_[1 + HEADER_SIZE * msgid + MSG_SRC]; }
        inline Index_t messageHandlerKey  (Index_t msgid) const { return pBuffer_[1 + HEADER_SIZE * msgid + MSG_KEY]; }

        inline Index_t messageSize  (Index_t msgid) const { return (messageEnd(msgid) - messageBegin(msgid))*sizeof(Index_t); } // in bytes
        inline void*   messagePtr   (Index_t msgid) const { return &pBuffer_[messageBegin(msgid)]; }
        inline void*   messagePtrEnd(Index_t msgid) const { return &pBuffer_[messageEnd  (msgid)]; }

     // The setters work only on the buffer in the MPI window
        inline void
        setMessageBegin(Index_t msgid, Index_t messageBegin) { 
            pBuffer_[1 + HEADER_SIZE * msgid + MSG_BGN] = messageBegin;
        }
        inline void 
        setMessageEnd(Index_t msgid, Index_t messageEnd) {
            pBuffer_[1 + HEADER_SIZE *  msgid    + MSG_END] = messageEnd;
            pBuffer_[1 + HEADER_SIZE * (msgid+1) + MSG_BGN] = messageEnd; // end of message is begin of next message.
        }
        inline void 
        setMessageDestination(Index_t msgid, Index_t messageDest) {
            pBuffer_[1 + HEADER_SIZE * msgid + MSG_DST] = messageDest;
        }
        inline void 
        setMessageSource(Index_t msgid, Index_t messageDest) {
            pBuffer_[1 + HEADER_SIZE * msgid + MSG_SRC] = messageDest;
        }
        inline void 
        setMessageHandlerKey(Index_t msgid, Index_t key) {
            pBuffer_[1 + HEADER_SIZE * msgid + MSG_KEY] = key;
        }
        inline void
        incrementNMessages(Index_t inc = 1) {
            pBuffer_[0] += inc;
        }
     // pointer to the raw buffer 
        inline Index_t* 
        ptr() const {
            return pBuffer_;
        }
         
     // Intelligible string representation of the header section of the message buffer
        std::vector<std::string> // list of lines
        headersToStr
          ( bool verbose = false // if true also prints unused headers.
          ) const;

     // string representation of a message. This interprets every word and dword in the message as a
     // a number of common types (int, long long int, float, double, ...). This is the only way to
     // examine the contents of the buffer if the corresponding MessageHandler is not available.
     // Mainly for debugging purposes.
        std::vector<std::string> // list of lines
        messageToStr
          ( Index_t msg_id       // message index
          );

    private:
        void initialize_();

    public:
        Index_t *pBuffer_;
    private:
        size_t bufferSize_;
        bool bufferOwned_;
        bool headersOnly_;
        size_t maxmsgs_;
     };
 //------------------------------------------------------------------------------------------------
    extern MessageBuffer theMessageBuffer;
 //------------------------------------------------------------------------------------------------
}// namespace mpi1s

#endif // MESSAGEBUFFER_H
