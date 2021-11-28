#include "Messenger.h"

#include "mpi1s.h"

namespace mpi1s
{//---------------------------------------------------------------------------------------------------------------------
    Messenger::
    Messenger
      ( size_t bufferSize // the size of the message section of the buffer, in Index_t words.
      , size_t max_msgs   // maximum number of messages that can be stored.
                          // This defines the size of the header section.
      )
    {// Allocate memory for the read buffer and initialize it
        messageBuffer_.initialize( bufferSize, max_msgs );
    }


    void
    Messenger::
    broadcastHeaders()
    {
        std::vector<Index_t> nmsgs(mpi1s::size);
        nmsgs[mpi1s::rank] = messageBuffer_.nMessages();
        for( int source = 0; source < mpi1s::size; ++source ) {
            MPI_Bcast(&nmsgs[source], 1, MPI_LONG_LONG_INT, source, MPI_COMM_WORLD);
        }
        if constexpr(::mpi12s::_debug_) {
            std::stringstream ss;
            ss<<::mpi12s::info<<"broadcastHeaders(): nmessages=";
            for( auto n; nmsgs ) ss<<n<<' ';
            printf("%s", ss.str());
        }
    }
 //---------------------------------------------------------------------------------------------------------------------
}
