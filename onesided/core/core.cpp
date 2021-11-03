/*
 *  C++ source file for module onesided.core
 */


// See http://people.duke.edu/~ccc14/cspy/18G_C++_Python_pybind11.html for examples on how to use pybind11.
// The example below is modified after http://people.duke.edu/~ccc14/cspy/18G_C++_Python_pybind11.html#More-on-working-with-numpy-arrays
//#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

namespace py = pybind11;


#include "Communicator.cpp"
#include "MessageBuffer.cpp"
#include "MessageBox.cpp"
#include "Message.cpp"
#include "MessageHandler.cpp"

using namespace mpi;

bool test_hello()
{
    bool ok = true;
 // Initialize the MPI environment
//    MPI_Init(NULL, NULL);

 // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

 // Get the rank of the handle
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

 // Get the name of the processor
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);

 // Print off a hello world message
    printf("Hello world from processor %s, rank %d out of %d processors\n",
           processor_name, world_rank, world_size);

 // Finalize the MPI environment.
//    MPI_Finalize();
    return ok;
}

class MyMessageHandler : public MessageHandlerBase
{
public:
    MyMessageHandler(MessageBox& mb)
      : MessageHandlerBase(mb) 
    {// initialize some data to be sent
        i_ = i0;
        d_ = d0;
        vi_.push_back(vi0);
        vi_.push_back(vi0+1);
        vi_.push_back(vi0+2);
        if( mb.comm().rank() != 0 ) {
            this->clear();
        } else {
            this->verify();
        }        
     // add the data to the message
        message().push_back(i_);
        message().push_back(d_);
        message().push_back(vi_);
    }
    void clear()
    {// empty the data members, so we can detect whether we have received the message
        i_ = 0;
        d_ = 0;
        vi_.clear();
    }
    bool verify() 
    {// verify the contents of the data members
        bool ok = true;
        ok &= (i_ == i0);
        ok &= (d_ == d0);
        ok &= (vi_.size() == 3);
        for( int i = 0; i<3; ++i)
            ok &= (vi_[i] == vi0+i);
        return ok;
    }

private:
    int    const i0 = 1;
    double const d0 = 11;
    int    const vi0= 12;    
    int i_;
    double d_;
    std::vector<int> vi_;
  
};

bool test_mh()
{
    bool ok = true;
    MessageBox mb(1000,10);
    MyMessageHandler mh(mb);
    if( mb.comm().rank() == 0 ) {
        mh.post(1);
    } 
    
    return ok;
}

PYBIND11_MODULE(core, m)
{// optional module doc-string
    m.doc() = "pybind11 core plugin"; // optional module docstring
 // list the functions you want to expose:
 // m.def("exposed_name", function_pointer, "doc-string for the exposed function");
    m.def("test_hello", &test_hello, "");
    m.def("test_mh", &test_mh, "");
}
