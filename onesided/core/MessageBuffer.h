#ifndef MESSAGEBUFFER_H
#define MESSAGEBUFFER_H

#include <vector>

 // copied from Primitives/Types/Index.h
    typedef int64_t Index_t;

namespace mpi 
{

 //------------------------------------------------------------------------------------------------
    class MessageBuffer
 // This class encapsulates the buffer for reading and writing messages.  
 // A message consists of a header section and a message section. The header section describes 
 // the messages and their location in the message section.
 // Used for both the window buffer, and the receiving buffer
 //------------------------------------------------------------------------------------------------
    {
        enum { MSG_BGN = 0
             , MSG_END
             , MSG_SRC
             , MSG_DST
             , MSG_KEY
             };
    public:
        enum { HEADER_SIZE = 5 }; // number of Index_t elements in the header of a message. 
        MessageBuffer();
        ~MessageBuffer();
     // allocate memory for the buffer:
        void 
        initialize
          ( size_t size     // amount to be allocated (on top of that for the header)
          , size_t max_msgs // maximum number of messages that can be stored.
          );
     // Assign pre-allocated memory for the buffer
        void 
        initialize
          ( Index_t * pBuffer // pointer to pre-allocated memory
          , size_t size       // amount of pre-allocated memory 
          , size_t max_msgs   // maximum number of messages that can be stored.
          );

     // Member functions for reading message headers from a buffer (getters).
     // This can be the buffer in the MPI window of this MessageBox, or a buffer 
     // read from other processes' MPI window. (see member functions getHeaderFromRank
     // and getHeaderFromAllRanks below).
     // Getters:
        inline Index_t   nMessages() const { return  pBuffer_[0]; }
        inline Index_t maxMessages() const { return maxmsgs_; }
        inline Index_t headerSize () const { return (pBuffer_[1]); }

        inline Index_t messageBegin       (Index_t msgid) const { return pBuffer_[1 + HEADER_SIZE * msgid + MSG_BGN]; }
        inline Index_t messageEnd         (Index_t msgid) const { return pBuffer_[1 + HEADER_SIZE * msgid + MSG_END]; }
        inline int     messageDestination (Index_t msgid) const { return pBuffer_[1 + HEADER_SIZE * msgid + MSG_DST]; }
        inline int     messageSource      (Index_t msgid) const { return pBuffer_[1 + HEADER_SIZE * msgid + MSG_SRC]; }
        inline Index_t messageHandlerKey  (Index_t msgid) const { return pBuffer_[1 + HEADER_SIZE * msgid + MSG_KEY]; }

        inline Index_t messageSize(Index_t msgid) const { return messageEnd(msgid) - messageBegin(msgid); }
        inline void*   messagePtr (Index_t msgid) const { return &pBuffer_[messageBegin(msgid)]; }

     // The setters work only on the buffer in the MPI window
        inline void
        setMessageBegin(Index_t msgid, Index_t messageBegin) { 
            // std::cout<<1 + HEADER_SIZE * msgid + MSG_BGN<<std::endl;
            pBuffer_[1 + HEADER_SIZE * msgid + MSG_BGN] = messageBegin; 
            // std::cout<<pBuffer_[1 + HEADER_SIZE * msgid + MSG_BGN]<<std::endl;
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
         
     // Intelligible string representation of the header section 
        std::string headers(bool verbose = false) const;

      private:
        void initialize_();

     private:
        Index_t *pBuffer_;
        size_t bufferSize_;
        bool bufferOwned_;
        size_t maxmsgs_;
     };
 //------------------------------------------------------------------------------------------------
}// namespace mpi

#endif // MESSAGEBUFFER_H
