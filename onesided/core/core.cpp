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

#include <stdexcept>


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
        if( mb.comm().rank() == 1 ) {
         // clear the receiver's data
            this->clear();
            bool verified = this->verify();
            if( verified ) {
                std::string errmsg = mb.comm().str() + "verified = true.";
                throw std::runtime_error(errmsg);
            }
        } else {
            bool verified = this->verify();
            if( !verified ) {
                std::string errmsg = mb.comm().str() + "verified = false.";
                throw std::runtime_error(errmsg);
            }
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
    int rank = mb.comm().rank();
    MyMessageHandler mh(mb);
    if( rank == 0 ) {
        std::cout<<mb.comm().str()<<"posting to 1"<<std::endl;
        mh.putMessage(1);
    } else {
        std::cout<<mb.comm().str()<<std::endl;
    }
    
    mb.getMessages();

    bool msg_ok = mh.verify();
    std::cout<<mb.comm().str()
             <<( rank == 0 ? "poster " : (rank == 1 ? "receiver " : "neutral "))
             <<"ok = "<<ok<<std::endl;
    
    ok &= msg_ok;
    return ok;
}

class MessageHandler2 : public MessageHandlerBase
{
    typedef Eigen::Matrix<float, 3, 1, Eigen::DontAlign> vec_t;

private:
    int              i_;
    double           d_;
    vec_t            v_;
    std::vector<int> a_;
    int              rank_;

public:
    MessageHandler2(MessageBox& mb)
      : MessageHandlerBase(mb) 
    {// initialize some data to be sent
     // we will put to the right rank and get from the left rank

        rank_ = comm().rank();
        
        i_ = rank_;
        d_ = rank_;
        v_ = vec_t(rank_,rank_,rank_);
        a_ = {rank_,rank_};

        std::cout
            <<'\n'<<comm().str()<<"setting i_ = "<<i_
            <<'\n'<<comm().str()<<"setting d_ = "<<d_
            <<'\n'<<comm().str()<<"verify v_ = ["<<v_<<']'
            <<'\n'<<comm().str()<<"verify a_ = ["<<a_[0]<<' '<<a_[1]<<']'
            <<std::endl;
                
     // add the data to the message
        message().push_back(i_);
        message().push_back(d_);
        message().push_back(v_);
        message().push_back(a_);
    }
 // verify the contents of the data members
    bool verify() 
    {
        std::cout
            <<'\n'<<comm().str()<<"verify i_ = "<<i_
            <<'\n'<<comm().str()<<"verify d_ = "<<d_
            <<'\n'<<comm().str()<<"verify v_ = ["<<v_<<']'
            <<'\n'<<comm().str()<<"verify a_ = ["<<a_[0]<<' '<<a_[1]<<']'
            <<std::endl;

        int left = comm().next_rank(-1);
        std::cout<<comm().str()<<"received from "<<left<<std::endl;

        bool ok = true;
        ok &= (i_ == left);                     if(!ok) std::cout<<"failing on i_"<<std::endl;
        ok &= (d_ == left);                     if(!ok) std::cout<<"failing on i_"<<std::endl;
        ok &= (v_ == vec_t(left,left,left));    if(!ok) std::cout<<"failing on v_"<<std::endl;
        ok &= (a_.size() == 2);                 if(!ok) std::cout<<"failing on a_.size()"<<std::endl;
        for( int i = 0; i<2; ++i ) {
            ok &= (a_[i] == left);              if(!ok) std::cout<<"failing on a_["<<i<<']'<<std::endl;
        }
        return ok;
    }
};

bool test_mh2()
{
    bool ok = true;
    {
        MessageBox mb(1000,10);
        int rank = mb.comm().rank();
        int right_rank = mb.comm().next_rank(); 

        MessageHandler2 mh(mb);
        std::cout<<mb.comm().str()<<"posting to "<<right_rank<<std::endl;
        mh.putMessage(right_rank);
        
        mb.getMessages();

        bool msg_ok = mh.verify();

        std::cout<<mb.comm().str()
                <<( rank == 0 ? "poster " : (rank == 1 ? "receiver " : "neutral "))
                <<"ok = "<<ok<<std::endl;
        
        ok &= msg_ok;
    }
 // apparently, not calling MPI_Finalizes results in a segfault in python's garbage collection.
 // and for that to succeed, MPI_Win_free (in ~MessageBox) must be called first. So, the MessageBox 
 // must be in its own scope. 
    // MPI_Finalize();
    return ok;
}

PYBIND11_MODULE(core, m)
{// optional module doc-string
    m.doc() = "pybind11 core plugin"; // optional module docstring
 // list the functions you want to expose:
 // m.def("exposed_name", function_pointer, "doc-string for the exposed function");
    m.def("test_hello", &test_hello, "");
    // m.def("test_mh", &test_mh, "");
    m.def("test_mh2", &test_mh2, "");
}
