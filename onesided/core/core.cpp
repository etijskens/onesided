/*
 *  C++ source file for module onesided.core
 */


// See http://people.duke.edu/~ccc14/cspy/18G_C++_Python_pybind11.html for examples on how to use pybind11.
// The example below is modified after http://people.duke.edu/~ccc14/cspy/18G_C++_Python_pybind11.html#More-on-working-with-numpy-arrays
//#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

namespace py = pybind11;

#include <mpi.h>

#include <string>
#include <iostream>
#include <sstream>
#include <exception>

#include "ArrayInfo.hpp"
#include "MessageBox.cpp"

void hello()
{
 // Initialize the MPI environment
//    MPI_Init(NULL, NULL);

 // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

 // Get the rank of the process
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

}

//class Communicator
//{
//  public:
//    Communicator(MPI_Comm comm=MPI_COMM_WORLD) : comm_(comm)
//    {
//        MPI_Comm_size(comm_, &size_);
//        MPI_Comm_rank(comm_, &rank_);
//    }
//
//    int rank() const {return rank_;}
//    int size() const {return size_;}
//    MPI_Comm comm() const {return comm_;}
//
//    std::string str() const {
//        return std::string("[") + std::to_string(rank_) + "/" + std::to_string(size_) + "] ";
//    }
//
//  private:
//    MPI_Comm comm_;
//    int rank_
//      , size_
//      ;
//};


void tryout1()
{

    Communicator comm;
    std::cout << comm.str() << "tryout1" << std::endl;
//    if( comm.size() != 2 )
//        throw stdException
 // Create the window
    int wbfr[4] = {0};

    MPI_Win win;
    MPI_Win_create(&wbfr, (MPI_Aint)4 * sizeof(int), sizeof(int), MPI_INFO_NULL, comm.comm()   , &win);

 // Fill the buffer on rank 1
    if (comm.rank() == 1) {
      wbfr[0] = 0;
      wbfr[1] = 1;
      wbfr[2] = 2;
      wbfr[3] = 3;
    }

 // start access epoch
    MPI_Win_fence(0, win);

    int * getbfr = new int[4];
    for( int i=0; i<4; ++i) getbfr[i] = -1;

    if (comm.rank() == 0) {
    // Fetch the value from the MPI process 1 window
    MPI_Get
      ( getbfr // buffer to store the elements to get
      , 2       // size of that buffer
      , MPI_INT // type of that buffer
      , 1       // process rank to get from (target)
      , 1       // offset in targets window
      , 2      // number of elements to get
      , MPI_INT // data type of that buffer
      , win     // window
      );
    }

 // end access epoch
    MPI_Win_fence(0, win);

    if (comm.rank() == 0) {
        printf("\n[MPI process 0] Value fetched from MPI process 1 window:");
        for( int i=0; i<4; ++i ) {
            std::cout << comm.str() << i << "->" << getbfr[i] << std::endl;
        }
    }

 // Destroy the window
    MPI_Win_free(&win);

}

#define VERBOSE
class MessageBuffer
{  public:
    MessageBuffer(int maxmsgs)
      : size_(1 + 3*maxmsgs + 10*maxmsgs)
      , data_(new int[size_])
      , maxmsgs_(maxmsgs)
    {
        reset();
    }

    ~MessageBuffer()
    {
        delete[] data_;
    }
    void reset() {
        data_[0] = 0; // = no messages
        begin_ = 1 + 3*maxmsgs_;
    }

    void createWindow(Communicator const& comm)
    {
        std::cout<<comm.str()<<", data_="<<data_<<", size_="<<size_;

        int success =
        MPI_Win_create
            ( data_
            , (MPI_Aint)(size_ * sizeof(int))
            , sizeof(int)
            , MPI_INFO_NULL
            , comm.comm()
            , &win_
            );
        std::cout<<" MPI_Win_create success="<<(success==MPI_SUCCESS)<<std::endl;
    }
    int  nMessages() const { return data_[0]; }
    int& nMessages()       { return data_[0]; }

    int  dst(int msgid) const { return data_[1 + msgid*3]; }
    int& dst(int msgid)       { return data_[1 + msgid*3]; }

    int  begin(int msgid) const { return data_[1 + msgid*3 + 1]; }
    int& begin(int msgid)       { return data_[1 + msgid*3 + 1]; }

    int  end(int msgid) const { return data_[1 + msgid*3 + 2]; }
    int& end(int msgid)       { return data_[1 + msgid*3 + 2]; }

 /* For the moment we assume that
      - all messages are of equal length, 10 ints
      - the message is a series of incrementing ints, starting with the destination of the message
  */
    int newMessage(int destination)
    {
        #ifdef VERBOSE
            std::cout<<"newMessage("<<destination<<"): # = "<<nMessages()<<'\n';
        #endif
        int msgid = nMessages()++;
        #ifdef VERBOSE
            std::cout<<"newMessage("<<destination<<"): # = "<<nMessages()<<", id = "<<msgid<<'\n';
        #endif

        dst(msgid) = destination;
        begin(msgid) = begin_;
        begin_ += 10;
        end(msgid) = begin_;
        #ifdef VERBOSE
            std::cout<<"newMessage("<<destination<<"): header = ["<<dst(msgid)
                                                             <<','<<begin(msgid)
                                                             <<','<<end(msgid)
                                                             <<"]\n";
        #endif
     // copy the message
        int counter = msgid*1000 + destination*100;
        for( int m=begin(msgid); m<end(msgid); ++m)
            data_[m] = counter++;

        #ifdef VERBOSE
            std::cout<<"newMessage("<<destination<<"): message = "<<'\n';
            for( int m=begin(msgid); m<end(msgid); ++m)
                std::cout<<"  "<<m<<" = "<<data_[m]<<'\n';
            std::cout<<std::endl;
        #endif

        return msgid;
    }
 // Human readable view of the MessageBuffer
    std::string str() const
    {
        std::stringstream ss;
        ss<<"MessageBuffer, nMessages = "<< nMessages()<<'\n';
        for( int i=0; i<nMessages(); ++i ){
            ss<<'('<<i<<") ["<<dst(i)<<','<<begin(i)<<','<<end(i)<<"] [ ";
            for(int j=begin(i); j<end(i); ++j) {
                ss<<data_[j];
            }
            ss<<']';
        }
        ss<<'\n';
        return ss.str();
    }

    int* data() {return data_;}

    MPI_Win win() { return win_; }

    void getHeader(int target, int* getbfr) const
     {
        MPI_Get
          ( getbfr // buffer to store the elements to get
          , 1 + 3*maxmsgs_
          , MPI_INT // type of that buffer
          , target  // process rank to get from (target)
          , 0       // offset in targets window
          , 1 + 13*maxmsgs_       // number of elements to get
          , MPI_INT // data type of that buffer
          , win_     // window
          );
    }
    void getMessage(int iMsg, int target, int* getbfr)
    {
        MPI_Get
          ( getbfr // buffer to store the elements to get
          , 10
          , MPI_INT // type of that buffer
          , target  // process rank to get from (target)
          , 10 + 10*iMsg       // offset in targets window
          , 10       // number of elements to get
          , MPI_INT // data type of that buffer
          , win_     // window
          );
    }

    int size() const { return size_; }

  private:
    int size_;
    int* data_;
    int maxmsgs_;
    int begin_;
    MPI_Win win_;
};

void tryout2()
{
    Communicator comm;
    std::cout << comm.str() << "tryout2" << std::endl;
    if( comm.size() != 1 ) {
        std::cout<<"Expecting 1 process."<<std::endl;
        return;
    }
 // Create the window
    MessageBuffer msgbfr(10);
    msgbfr.newMessage(1);
    msgbfr.newMessage(2);
    msgbfr.newMessage(1);
}

void tryout3()
{
    Communicator comm;
    std::cout << comm.str() << "tryout1" << std::endl;
    if( comm.size() != 2 ) {
        std::cout<<"Expecting 2 processes."<<std::endl;
        return;        
    }

 // Create and fill the buffer 
    int const wsize = 40;
    int wbfr[wsize] = {-1};
    if (comm.rank() == 0) {
        wbfr[0] = 2;
        wbfr[1] = 1;     // one message for rank 1 from 10:20
        wbfr[2] = 10;
        wbfr[3] = 20;
        for( int i=10; i<20; ++i) wbfr[i] = 1000 + 100 + i - 10;
    }

 // create mpi window:
    MPI_Win win;
    MPI_Win_create(&wbfr, (MPI_Aint)(wsize * sizeof(int)), sizeof(int), MPI_INFO_NULL, comm.comm(), &win);

 // start access epoch
    MPI_Win_fence(0, win);

    int getbfr[wsize] = {-2};

    if (comm.rank() == 1)
    {// Fetch the header from the MPI process 1 window
        MPI_Get
          ( getbfr // buffer to store the elements to get
          , 10       // size of that buffer
          , MPI_INT // type of that buffer
          , 0       // process rank to get from (target)
          , 0       // offset in targets window
          , 10      // number of elements to get
          , MPI_INT // data type of that buffer
          , win     // window
          );
    }

 // end access epoch
    MPI_Win_fence(0, win);

    std::cout<<comm.str();
    if (comm.rank() == 0) {
        for( int i=0; i<10; ++i) std::cout<<wbfr[i]<<' ';
    }
    else {
        for( int i=0; i<10; ++i) std::cout<<getbfr[i]<<' ';
    }
    std::cout<<std::endl;

 // Destroy the window
    MPI_Win_free(&win);

}


void tryout4()
{
    Communicator comm;
    std::cout << comm.str() << "tryout2" << std::endl;
    std::cout << comm.str() << "sizeof(int)=" <<sizeof(int)<< std::endl;
    if( comm.size() != 2 ) {
        std::cout<<"Expecting 2 processes."<<std::endl;
        return;
    }
 // Create the window
    MessageBuffer msgbfr(3);
    std::cout<<comm.str()<<", size_="<<msgbfr.size()<<std::endl;

    msgbfr.createWindow(comm);
 // rank 0 creates messages
    if (comm.rank() == 0) {
        msgbfr.newMessage(1);
        msgbfr.newMessage(1);

        std::cout<<comm.str()<<"header ; #="<<msgbfr.data()[0];
        for( int i=1; i<10; i+=3) {
             std::cout<<"\n[ "<<msgbfr.data()[i]<<' '<<msgbfr.data()[i+1]<<' '<<msgbfr.data()[i+2]<<" ]";
        }
        std::cout<<std::endl;
    }

 // create a getbfr for rank 1 to store the message from rank 0
    int * getbfr = NULL;
    if( comm.rank() == 1) {
        getbfr = new int[40];
        for( int i=0; i<40; ++i) getbfr[i]=-1;
    }
        
    int success = MPI_Win_fence(0, msgbfr.win());
    std::cout<<comm.str()<<"MPI_Win_fence success="<<(success==MPI_SUCCESS)<<std::endl;
        if (comm.rank() == 1) {
            msgbfr.getHeader(0, getbfr);
            msgbfr.getMessage(0,0,getbfr+10);
            msgbfr.getMessage(1,0,getbfr+20);
        }
    MPI_Win_fence(0, msgbfr.win());
    std::cout<<comm.str();
    if(comm.rank() == 0) {
        int * d = msgbfr.data();
        for( int i=0; i<10; ++i) std::cout<<d[i]<<' ';
    }
    else {
        for( int i=0; i<10; ++i) std::cout<<getbfr[i]<<' ';
    }
    std::cout<<std::endl; 

    std::cout<<comm.str();
    if(comm.rank() == 0) {
        int * d = msgbfr.data()+10;
        for( int i=0; i<10; ++i) std::cout<<d[i]<<' ';
    }
    else {
        for( int i=0; i<10; ++i) std::cout<<getbfr[i+10]<<' ';
    }
    std::cout<<std::endl;

    std::cout<<comm.str();
    if(comm.rank() == 0) {
        int * d = msgbfr.data()+20;
        for( int i=0; i<10; ++i) std::cout<<d[i]<<' ';
    }
    else {
        for( int i=0; i<10; ++i) std::cout<<getbfr[i+20]<<' ';
    }
    std::cout<<std::endl;
}


Index_t
add_int_array(MessageBox& mb, Index_t for_rank, py::array_t<Index_t> a)
{
    ArrayInfo<Index_t,1> a__(a);
    void * ptrData = a__.data();
    size_t nBytes = a.shape(0)*sizeof(Index_t);
    std::cout<<"nbytes="<<nBytes<<std::endl;
    Index_t msgid = mb.addMessage(for_rank, ptrData, nBytes);
    return msgid;
}

Index_t
add_float_array(MessageBox& mb, Index_t for_rank, py::array_t<double> a)
{
    ArrayInfo<double,1> a__(a);
    void * ptrData = a__.data();
    size_t nBytes = a.shape(0)*sizeof(double);
    std::cout<<"nbytes="<<nBytes<<std::endl;
    Index_t msgid = mb.addMessage(for_rank, ptrData, nBytes);
    return msgid;
}

PYBIND11_MODULE(core, m)
{// optional module doc-string
    m.doc() = "pybind11 core plugin"; // optional module docstring
 // list the functions you want to expose:
 // m.def("exposed_name", function_pointer, "doc-string for the exposed function");
    m.def("hello", &hello, "");
    m.def("tryout1", &tryout1, "");
    m.def("tryout2", &tryout2, "");
    m.def("tryout3", &tryout3, "");
    m.def("tryout4", &tryout4, "");
    py::class_<MessageBox>(m, "MessageBox")
        .def(py::init<Index_t, Index_t>(), py::arg("maxmsgs") = 100, py::arg("size") = -1)
        .def("nMessages", &MessageBox::nMessages)
        .def("str",&MessageBox::str)
        ;
    m.def("add_int_array", &add_int_array);
    m.def("add_float_array", &add_float_array);
}