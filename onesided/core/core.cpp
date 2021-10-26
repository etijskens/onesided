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
#include <map>

#include "ArrayInfo.hpp"
#include "MessageBox.cpp"

void hello()
{
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
    // Fetch the value from the MPI handle 1 window
    MPI_Get
      ( getbfr // buffer to store the elements to get
      , 2       // size of that buffer
      , MPI_INT // type of that buffer
      , 1       // handle rank to get from (target)
      , 1       // offset in targets window
      , 2      // number of elements to get
      , MPI_INT // data type of that buffer
      , win     // window
      );
    }

 // end access epoch
    MPI_Win_fence(0, win);

    if (comm.rank() == 0) {
        printf("\n[MPI handle 0] Value fetched from MPI handle 1 window:");
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
          , target  // handle rank to get from (target)
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
          , target  // handle rank to get from (target)
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
        std::cout<<"Expecting 1 handle."<<std::endl;
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
    {// Fetch the header from the MPI handle 1 window
        MPI_Get
          ( getbfr // buffer to store the elements to get
          , 10       // size of that buffer
          , MPI_INT // type of that buffer
          , 0       // handle rank to get from (target)
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

void reverse_stringstream()
{
    std::stringstream ss;
    std::string s("hello");
    int i=5;
    std::cout<<s<<std::endl;
    std::cout<<i<<std::endl;

    ss.write(s.data(), s.size());
    ss.write(reinterpret_cast<char*>(&i), sizeof(int));
    std::string written = ss.str();
    std::cout<<"serialized: |"<<written<<'|'<<std::endl;

    s = "01234";
    i = 0;
    ss.read(const_cast<char*>(s.data()), s.size());
    ss.read(reinterpret_cast<char*>(&i), sizeof(int));

    std::cout<<"deserialized:"<<std::endl;
    std::cout<<s<<std::endl;
    std::cout<<i<<std::endl;
}

/*
 //---------------------------------------------------------------------------------------------------------------------
 // generic version for encoding and decoding messages with the same function calls
 //---------------------------------------------------------------------------------------------------------------------
    int const not_implemented =-1;
    int const built_in_type   = 0;
    int const array_like_type = 1;

    template<typename T> struct handler_kind {
        static int const value = not_implemented;
    };

    template<> struct handler_kind<int>                      { static int const value = built_in_type; };
    template<> struct handler_kind<unsigned int>             { static int const value = built_in_type; };
    template<> struct handler_kind<Index_t>                  { static int const value = built_in_type; };
    template<> struct handler_kind<float>                    { static int const value = built_in_type; };
    template<> struct handler_kind<double>                   { static int const value = built_in_type; };
    template<typename T> struct handler_kind<std::vector<T>> { static int const value = array_like_type; };
    template<> struct handler_kind<std::string>              { static int const value = array_like_type; };// C++17 required

 //---------------------------------------------------------------------------------------------------------------------
 // MessageHandler class
 //---------------------------------------------------------------------------------------------------------------------
 // only specialisations are functional.
    template<typename T, int> struct MessagaHandler;

 //---------------------------------------------------------------------------------------------------------------------
 // MessageHandler specialisation for selected built-in types
 //---------------------------------------------------------------------------------------------------------------------
    template<typename T> struct MessagaHandler<T,built_in_type>
    {
        typedef MessagaHandler<T,built_in_type> This_type;

        static
        void write(T const & t, std::stringstream& ss) {
            std::cout<<"write not is_array_like"<<std::endl;
            ss.write( reinterpret_cast<std::ostringstream::char_type const *>(&t), sizeof(T) );
        }
        static
        void read(T & t, std::stringstream& ss) {
            std::cout<<"read not is_array_like"<<std::endl;
            ss.read( reinterpret_cast<std::ostringstream::char_type *>(&t), sizeof(T) );
        }
        static void handle(T const & t, std::stringstream& ss, char mode) {
            std::cout<<"handle not is_array_like, mode="<<mode<<std::endl;
            if (mode == 'w') {
                This_type::write(t,ss);
            } else if (mode == 'r') {
                This_type::read(const_cast<T&>(t),ss);
            }
        }
        static void handle(T& t, std::stringstream& ss, char mode) {
            std::cout<<"handle not is_array_like, mode="<<mode<<std::endl;
            if (mode == 'w') {
                This_type::write(const_cast<T const &>(t),ss);
            } else if (mode == 'r') {
                This_type::read(t,ss);
            } else {
                throw std::invalid_argument("Mode should be 'r' (read) or 'w' (write).");
            }
        }
    };

 //---------------------------------------------------------------------------------------------------------------------
 // MessageHandler specialisation for selected array-like types
 //---------------------------------------------------------------------------------------------------------------------
 // class T must have
 //   - T::value_type, the type of the array elements
 // T objects t must have
 //   - t.data() returning T::value_type* or T::value_type const*, pointer to begin of te data buffer
 //   - t.size() returning size_t, number of array elements
 // This is ok for std::vector<T> with T a built-in type , std::string
    template<typename T> struct MessagaHandler<T,array_like_type>
    {
        typedef MessagaHandler<T,array_like_type> This_type;
     // write a T object
        static
        void write(T const & t, std::stringstream& ss) {
            std::cout<<"write is_array_like"<<std::endl;
            size_t const sz = t.size();
            std::cout<<"write size: "<<sz<<std::endl;
            ss.write(reinterpret_cast<std::stringstream::char_type const *>(&sz), sizeof(size_t));
            ss.write(reinterpret_cast<std::stringstream::char_type const *>(t.data()), sz*sizeof(typename T::value_type));
            std::cout<<"write done "<<std::endl;
        }
        
     // read a T object
        static
        void read(T & t, std::stringstream& ss) {
            std::cout<<"read is_array_like"<<std::endl;
            size_t sz=0;
            ss.read(reinterpret_cast<std::ostringstream::char_type*>(&sz), sizeof(size_t));
            std::cout<<"read.size "<<sz<<std::endl;
            t.resize(sz);
            std::cout<<"read size: "<<t.size()<<std::endl;
            ss.read(reinterpret_cast<std::ostringstream::char_type*>(t.data()), sz*sizeof(typename T::value_type));
            std::cout<<"read done"<<std::endl;
        }
        
     // write or read a T object, select using mode argument
        static void handle(T const & t, std::stringstream& ss, char mode) {
            std::cout<<"handle is_array_like, mode="<<mode<<std::endl;
            if (mode == 'w') {
                This_type::write(t,ss);
            } else if (mode == 'r') {
                This_type::read(const_cast<T&>(t),ss);
            } else {
                throw std::invalid_argument ("Mode should be 'r' (read) or 'w' (write).");
            }
        }
        static void handle(T& t, std::stringstream& ss, char mode) {
            std::cout<<"handle is_array_like, mode="<<mode<<std::endl;
            if (mode == 'w') {
                This_type::write(const_cast<T const &>(t),ss);
            } else if (mode == 'r') {
                This_type::read(t,ss);
            }
        }
    };

 //---------------------------------------------------------------------------------------------------------------------
 // classMessageStream
 //---------------------------------------------------------------------------------------------------------------------
   class MessageStream
    {
      public:
        MessageStream() {}
    
     // write T object
        template<typename T>
        void
        write(T const & t) {
            MessagaHandler<T,handler_kind<T>::value>::write(t, ss_);
        }
     // read T object
        template<typename T>
        void
        read(T & t) {
            MessagaHandler<T,handler_kind<T>::value>::read(t, ss_);
        }
     // write or read T object, select with mode argument
        template<typename T>
        void
        handle(T & t, char mode) {
            MessagaHandler<T,handler_kind<T>::value>::handle(t, ss_, mode);
        }
     // get the message as e string to put it to the window
        std::string str() const { return ss_.str(); }
      private:
        std::stringstream ss_;
    };
*/

void reverse_stringstream_1()
{
    MessageStream ms;

    std::string s("helloworld");
    int i=5;
    std::vector<double> d = {.0,.1,.2,.3,.4};
    std::cout<<s<<std::endl;
    std::cout<<i<<std::endl;
    std::cout<<"[ ";
    for( auto ptr=d.data(); ptr<d.data()+d.size(); ++ptr) std::cout<<*ptr<<' ';
    std::cout<<"]\n";

    std::cout<<'s'<<std::endl;
    ms.handle(s,'w');
    std::cout<<'i'<<std::endl;
    ms.handle(i,'w');
    std::cout<<'d'<<std::endl;
    ms.handle(d,'w');

    std::string written = ms.str();
    std::cout<<"serialized: |"<<written<<'|'<<std::endl;

    s = "01234";
    i = 0;
    d.clear();
    ms.handle(s,'r');
    ms.handle(i,'r');
    ms.handle(d,'r');

    std::cout<<"deserialized:"<<std::endl;
    std::cout<<s<<std::endl;
    std::cout<<i<<std::endl;
    std::cout<<"[ ";
    for( auto ptr=d.data(); ptr<d.data()+d.size(); ++ptr) std::cout<<*ptr<<' ';
    std::cout<<"]\n";
}

class MessageHandler;

class MessageHandlerFactory
{
public:
    MessageHandlerFactory()
      : counter_(0)
    {}
    ~MessageHandlerFactory()
    {
     // destroy als items in the map
    }
    
    template<typename T>
    T&
    createMessageHandler(MessageBox& mb)
    {
     // mayb assert that T derives from MessageHandler
        size_t key = counter_++;
        T* mh = new T(mb, key);
     // maybe assert whether key does not already exist.
        registry_[key] = mh;
        
        return *mh;
    }

static MessageHandlerFactory theMessageHandlerFactory;

private:
    size_t counter_;
    std::map<size_t,MessageHandler*> registry_;
};

class MessageHandler
{
public:
    MessageHandler(MessageBox& messageBox, size_t key)
      : mb_(messageBox)
      , key_(key)
    {}
    virtual bool handle(char mode) = 0;
    //  Returns false if the message is empty, true otherwise.

    void post(int to_rank)
    {// construct the message, and put the message in the mpi window

     // Construct the message
        if( this->handle('w') ) // non-empty message
        {// Put the message in the window.
            std::string message = ms_.str();
            mb_.addMessage(to_rank, message.data(), message.size());
        }
    }
  protected:
    MessageBox & mb_;
    size_t key_;
    MessageStream ms_;
};

//    void readMyPost()
//    {
//        Epoch epoch(mb_); // open epoch on messageBox mb_
//
//        for( int rank = 0; rank < mb_.comm().size(); ++rank )
//        {
//            if( rank != mb_.comm().rank() )
//            {// only look for messages from other ranks
//
//            }
//        }
//
//     // epoch is closed.
//    }

class MessageHandlerTest : public MessageHandler
{
    friend class MessageHandlerFactory;

 // private ctor, only to be called by theMessageHandlerFactory
    MessageHandlerTest(MessageBox& messageBox, size_t key)
      : MessageHandler(messageBox,key)
    {}

  public:
    void init(int rank)
    {
        s_ = std::string("this is rank ")+std::to_string(rank);
        i_ = rank;
        for( size_t i = 0; i < 5; ++i)
            d_[i] = 10*rank + i*1.1;
        std::cout<<'['<<rank<<"] "
                 <<"\n  s = '"<<s_<<"'"
                 <<"\n  i = "<<i_
                 <<"\n  d = [ ";
        for( size_t i=0; i<5; ++i) {
            std::cout<<d_[i]<<' ';
        }
        std::cout<<std::endl;
    }

    void verify(int rank)
    {
        if( rank != 0) {
            assert( s_ == "helloworld" );
            assert( i_ = 5 );
            for( size_t i = 0; i<5; ++i) {
                assert( d_[i] == (static_cast<double>(i)/10.0));
            }
        }
    }
    virtual bool handle(char mode)
    {
        if( mode == 'w') {
            std::cout<<"\nwriting s = "<<s_
                     <<"\nwriting i = "<<i_
                     <<"\nwriting d = [ ";
            for( size_t i = 0; i<5; ++i) {
                std::cout<<d_[i]<<' ';
            }
            std::cout<<std::endl;
        }
        ms_.handle(s_,mode);
        ms_.handle(i_,mode);
        ms_.handle(d_,mode);

        if( mode == 'r') {
            std::cout<<"\nread s = "<<s_
                     <<"\nread i = "<<i_
                     <<"\nread d = [ ";
            for( size_t i = 0; i<5; ++i) {
                std::cout<<d_[i]<<' ';
            }
            std::cout<<std::endl;
        }
        return true;
    }
  private:// data
    std::string s_;
    int i_;
    std::vector<double> d_;
};

void reverse_stringstream_2()
{
    MessageBox mb;
//    MessageHandlerTest mit(mb,0);

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
        .def("nMessages", &MessageBox::getNMessages)
        .def("str",&MessageBox::str)
        .def("getMessages", &MessageBox::getMessages)
        ;
    m.def("add_int_array", &add_int_array);
    m.def("add_float_array", &add_float_array);
    m.def("reverse_stringstream",&reverse_stringstream);
    m.def("reverse_stringstream_1",&reverse_stringstream_1);
    m.def("reverse_stringstream_2",&reverse_stringstream_2);
}
