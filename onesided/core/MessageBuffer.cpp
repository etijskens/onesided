#include "MessageBuffer.h"

namespace mpi
{
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
    void
    MessageBuffer::
    initialize(Index_t* pBuffer, size_t size, size_t maxmsgs)
    {
        bufferSize_ = size;
        
        pBuffer_[0] = 0; // initially, there are no messages.
        pBuffer_[1] = 1 + HEADER_SIZE * maxmsgs_; // Begin of the first message, and the
                                                  // size of the message header section.
        setMessageBegin( 0, pBuffer_[1] );
    }
 //------------------------------------------------------------------------------------------------
       


 //------------------------------------------------------------------------------------------------
}