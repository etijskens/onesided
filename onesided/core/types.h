#ifndef TYPES_H
#define TYPES_H

typedef int64_t Index_t; // copied from Primitives/Types/Index.h

namespace mpi12s
{//---------------------------------------------------------------------------------------------------------------------
    typedef size_t MessageHandlerKey_t; 
     // This type may be modified, but there are some constraints:
     // Because we need to know the key before the message can be received, it is necessary
     // that it can be stored in the header section of MessageBuffers. Therefor, it must be
     // of fixed size, and, preferentially, a size that is a multiple of sizeof(Index_t).
 //---------------------------------------------------------------------------------------------------------------------
}// namespace mpi1s

#endif // TYPES_H