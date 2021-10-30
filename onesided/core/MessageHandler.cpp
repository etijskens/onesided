#include "MessageHandler.h"

namespace mpi 
{
 //------------------------------------------------------------------------------------------------
 // MessageHandlerFactory implementation
 //------------------------------------------------------------------------------------------------

    MessageHandlerFactory::
    MessageHandlerFactory() : counter_(0) {}

    MessageHandlerFactory::
    ~MessageHandlerFactory()
    {
        // destroy all items in the map
    }
        
    MessageHandlerBase* 
    MessageHandlerFactory::
    operator[](size_t key)
    {
        return registry_[key];
    }


 //------------------------------------------------------------------------------------------------
 // MessageHandlerFactory implementation
 //------------------------------------------------------------------------------------------------
    MessageHandlerBase::
    MessageHandlerBase(size_t key)
      : key_(key)
    {}

    void
    MessageHandlerBase::
    post(int to_rank)
    {// construct the message, and put the message in the mpi window
    }
 //------------------------------------------------------------------------------------------------

}// namespace mpi