/*
 *  C++ source file for module onesided.core
 */


// See http://people.duke.edu/~ccc14/cspy/18G_C++_Python_pybind11.html for examples on how to use pybind11.
// The example below is modified after http://people.duke.edu/~ccc14/cspy/18G_C++_Python_pybind11.html#More-on-working-with-numpy-arrays
//#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

namespace py = pybind11;

#include "mpi12s.cpp"
#include "MessageBuffer.cpp"
#include "MessageBox.cpp"
#include "Message.cpp"
#include "MessageHandler.cpp"

#include <stdexcept>


using namespace mpi12s;

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


namespace test1
{//---------------------------------------------------------------------------------------------------------------------
    class MessageHandler : public ::mpi1s::MessageHandlerBase
    {
    public:
        MessageHandler(::mpi1s::MessageBox& mb)
          : MessageHandlerBase(mb) 
        {// initialize some data to be sent
            i_ = i0;
            d_ = d0;
            vi_.push_back(vi0);
            vi_.push_back(vi0+1);
            vi_.push_back(vi0+2);
            if( ::mpi12s::rank == 1 ) {
             // clear the receiver's data
                this->clear();
                bool verified = this->verify();
                if( verified ) {
                    std::string errmsg =  + "verified = true.";
                    throw std::runtime_error(errmsg);
                }
            } else {
                bool verified = this->verify();
                if( !verified ) {
                    std::string errmsg = ::mpi12s::info + "verified = false.";
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

    bool test()
    {
        bool ok = true;
        ::mpi1s::MessageBox mb(1000,10);
        int const rank = ::mpi12s::rank;
        MessageHandler mh(mb);
        if( rank == 0 ) {
            std::cout<<::mpi12s::info<<"posting to 1"<<std::endl;
            mh.putMessage(1);
        } else {
            std::cout<<::mpi12s::info<<std::endl;
        }
        
        mh.getMessages();
    
        bool msg_ok = mh.verify();
        std::cout<<::mpi12s::info
                 <<( rank == 0 ? "poster " : (rank == 1 ? "receiver " : "neutral "))
                 <<"ok = "<<ok<<std::endl;
        
        ok &= msg_ok;
        return ok;
    }
 //---------------------------------------------------------------------------------------------------------------------
}// namespace test1

namespace test2
{//---------------------------------------------------------------------------------------------------------------------
    class MessageHandler : public ::mpi1s::MessageHandlerBase
    {
        typedef Eigen::Matrix<float, 3, 1, Eigen::DontAlign> vec_t;

    private:
        int              i_;
        double           d_;
        vec_t            v_;
        std::vector<int> a_;
        int              rank_;

    public:
        MessageHandler(::mpi1s::MessageBox& mb)
          : MessageHandlerBase(mb)
        {// initialize some data to be sent
         // we will put to the right rank and get from the left rank

            rank_ = ::mpi12s::rank;

            i_ = rank_;
            d_ = rank_;
            v_ = vec_t(rank_,rank_,rank_);
            a_ = {rank_,rank_};

            std::cout
                <<'\n'<<::mpi12s::info<<"setting i_ = "<<i_
                <<'\n'<<::mpi12s::info<<"setting d_ = "<<d_
                <<'\n'<<::mpi12s::info<<"verify v_ = ["<<v_<<']'
                <<'\n'<<::mpi12s::info<<"verify a_ = ["<<a_[0]<<' '<<a_[1]<<']'
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
                <<'\n'<<::mpi12s::info<<"verify i_ = "<<i_
                <<'\n'<<::mpi12s::info<<"verify d_ = "<<d_
                <<'\n'<<::mpi12s::info<<"verify v_ = ["<<v_<<']'
                <<'\n'<<::mpi12s::info<<"verify a_ = ["<<a_[0]<<' '<<a_[1]<<']'
                <<std::endl;

            int left = next_rank(-1);
            std::cout<<::mpi12s::info<<"received from "<<left<<std::endl;

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

    bool test()
    {
        ::mpi12s::init();

        bool ok = true;
        {
            ::mpi1s::MessageBox mb(1000,10);
            int rank = ::mpi12s::rank;
            int right_rank = next_rank();
    
            MessageHandler mh(mb);
            std::cout<<::mpi12s::info<<"posting to "<<right_rank<<std::endl;
            mh.putMessage(right_rank);
            
            mh.getMessages();
    
            bool msg_ok = mh.verify();
    
            std::cout<<::mpi12s::info
                    <<( rank == 0 ? "poster " : (rank == 1 ? "receiver " : "neutral "))
                    <<"ok = "<<ok<<std::endl;
            
            ok &= msg_ok;
            std::cout<<::mpi12s::info<<"end of ::mpi1s::MessageBox scope"<<std::endl;
        }
        std::cout<<::mpi12s::info<<" done"<<std::endl;
        ::mpi12s::finalize();
        return ok;
    }
 //---------------------------------------------------------------------------------------------------------------------
}// namespace test2


namespace test3
{//---------------------------------------------------------------------------------------------------------------------
    typedef Eigen::Matrix<float, 3, 1, Eigen::DontAlign> vec_t;
    typedef float value_t;
    class ParticleContainer
    {
        std::vector<bool> alive_;
    public:
        std::vector<value_t> r;
        std::vector<value_t> m;
        std::vector<vec_t>   x;

        ParticleContainer(int size)
        {
            std::stringstream ss;
            ss<<info<<'\n';
            alive_.resize(size);
            r.resize(size);
            m.resize(size);
            x.resize(size);
            for( int i=0; i<size; ++i) {
                int ir = 100*rank + i;
                alive_[i] = true;
                r[i] = ir;
                m[i] = ir + size;
                for( int k=0; k<3; ++k )
                    x[i][k] = ir+k*size;
                ss<<i<<std::setw(4)<<r[i]
                     <<std::setw(4)<<m[i]
                     <<'['<<std::setw(4)<<x[i][0]<<std::setw(4)<<x[i][1]<<std::setw(4)<<x[i][2]<<"]\n";
            }   std::cout<<ss.str()<<std::endl;
        }

        size_t // the size of a particle container.
        size() const {
            return alive_.size();
        }

     // Grow the arrays by a factor 1.5, and return the position of the first new element
     // (which is not alive, obviously).
        int grow()
        {
            int old_size = alive_.size();
            int new_size = (int)(old_size*1.5);
            if (new_size == old_size) ++new_size;
            alive_.resize(new_size);
//            x.resize(new_size);
            r.resize(new_size);
            m.resize(new_size);
            for (int i=old_size; i<new_size; ++i) {
                alive_[i] = false;
            }
            return old_size;
        }

     // Find an index for a new particle
        int add()
        {// look for a dead particle
            for( int i=0; i<alive_.size(); ++i) {
                if (not alive_[i]) {
                    alive_[i] = true;
                    return i;
                }
            }// none found
         // grow the array.
            return grow();
        }

     // remove an element
        void remove(int i)
        {
            alive_[i] = false;
        }

     // test if alive
        bool is_alive(int i) const {
            return alive_[i];
        }
    };

    class MessageHandler : public ::mpi1s::MessageHandlerBase
    {
        ParticleContainer& pc_;
        std::vector<value_t> ri;
        std::vector<value_t> mi;
        std::vector<vec_t  > xi;
    public:
        MessageHandler(::mpi1s::MessageBox& mb, ParticleContainer& pc)
          : MessageHandlerBase(mb)
          , pc_(pc)
        {
            message().push_back(ri);
            message().push_back(mi);
            message().push_back(xi);
        }

        void putMessage(std::vector<int>& indices, int to_rank)
        {
            ri.clear();
            mi.clear();
            xi.clear();

            {// assemble the non-contigous data into a contiguous array
                for( auto index: indices) {
                    ri.push_back(pc_.r[index]);
                    mi.push_back(pc_.m[index]);
                    xi.push_back(pc_.x[index]);
                 // remove the particle from the pc:
                    pc_.remove(index);
                }
                MessageHandlerBase::putMessage(to_rank); // from rank 0 to rank 1
                int n = ri.size();
                std::stringstream ss;
                ss<<info<<"sending "<<n<<" items to "<<to_rank<<'\n';
                for( int i=0; i<n; ++i) {
                    ss<<i<<std::setw(4)<<ri[i]<<std::setw(4)<<mi[i]<<" ["<<std::setw(4)<<xi[i][0]<<std::setw(4)<<xi[i][1]<<std::setw(4)<<xi[i][2]<<"]\n";
                }   std::cout<<ss.str()<<std::endl;
            }
         // clearing is not necessary. Here just to make sure that the receivers are correctly resized.
//            xi.clear();
            ri.clear();
            mi.clear();
        }

        void getAllMessages()
        {// this must be executed by ALL processes
            std::cout<<info<<"sizes before= "<<ri.size()<<','<<mi.size()<<std::endl;
            getMessages();
            std::cout<<info<<"sizes after = "<<ri.size()<<','<<mi.size()<<std::endl;
            int n = ri.size();
            std::cout<<info<<n<<" items received\n";
            for( int i=0; i<n; ++i) {
                std::cout<<info<<i<<' '<<ri[i]<<' '<<mi[i]
                          <<" ["<<xi[i][0]<<' '<<xi[i][1]<<' '<<xi[i][2]<<"] "<<ri[i]<<'\n';
            }   std::cout<<std::endl;
         // copy to the particle container:
            std::vector<int> indices(n);
            std::stringstream ss;
            ss<<::mpi12s::info<<"adding elements ";
            for( size_t i = 0; i<n; ++i ) {
                indices[i] = pc_.add();
                ss<<indices[i]<<' ';
            }   std::cout<<ss.str()<<std::endl;
            for( size_t i = 0; i<n; ++i ) {
                pc_.m[indices[i]] = mi[i];
                pc_.r[indices[i]] = ri[i];
                pc_.x[indices[i]] = xi[i];
            }
        }
    };

    bool test()
    {
        init();

        ParticleContainer pc(8);

        bool ok = true;
        {
            ::mpi1s::MessageBox mb(1000,10);
            MessageHandler mh(mb, pc);
         // move the odd particles to the next rank
         // there is one message for each process
            std::vector<int> indices = {1,3,5,7};
            mh.putMessage(indices, next_rank());
            mh.getAllMessages();

            std::cout<<::mpi12s::info<<"end of ::mpi1s::MessageBox scope"<<std::endl;
         // destroy ::mpi1s::MessageBox mb, must come before finalize()
        }
        std::stringstream ss;
        ss<<::mpi12s::info<<"pc\n";
        for(size_t i=0; i<pc.size(); ++i ) {
            if( pc.is_alive(i) ) {
                ss<<'['<<i<<"] "
                  <<std::setw(4)<<pc.r[i]
                  <<std::setw(4)<<pc.m[i]
                  <<" ["<<std::setw(4)<<pc.x[i][0]<<std::setw(4)<<pc.x[i][1]<<std::setw(4)<<pc.x[i][2]<<"]";
            }
         // verify contents:
            int const   my_rank = rank;
            int const prev_rank = next_rank(-1);
            value_t expected_r = ( i%2 ? 100*prev_rank + i // odd i
                                       : 100*my_rank + i   //even i
                                 );
            value_t expected_m = expected_r + 8;
            vec_t   expected_x = vec_t(expected_r, expected_r+8, expected_r+16);
            ok &= pc.r[i] == expected_r;
            ss<<' '<<ok;
            ok &= pc.m[i] == expected_m;
            ss<<' '<<ok;
            ok &= pc.x[i] == expected_x;
            ss <<' '<<ok<<'\n';
        }  std::cout<<ss.str()<<std::endl;

        std::cout<<::mpi12s::info<<" done"<<std::endl;
        finalize();
        return ok;
    }
 //---------------------------------------------------------------------------------------------------------------------
}// namespace test3

namespace test4
{//---------------------------------------------------------------------------------------------------------------------
    bool test()
    {// This test broadcasts an array in which every rank has initialized its own section: a[rank*5:rank*5+5]
     // After broadcasting the array is complete on every rank
        init();

     // // Get number of processes and check that 3 processes are use
        if(::mpi12s::size != 3)
        {
            printf("This application is meant to be run with 3 MPI processes.\n");
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
        bool ok = true;
        int a[15];
        for(size_t i = 0; i < 15; ++i) {
            a[i] = 0;
        }
        for( int r=0; r<3; ++r ) {
            if (rank == r) {
                for(size_t i = r*5; i < r*5 + 5; ++i) {
                    a[i] = i;
                }
            }
        }
        std::stringstream ss;
        ss<<::mpi12s::info;
        for(size_t i = 0; i < 15; ++i) {
            ss<<a[i]<<' ';
        }   std::cout<<ss.str()<<std::endl;

        for( int rank=0; rank<3; ++rank) {
            MPI_Bcast(&a[rank*5], 5, MPI_INT, rank, MPI_COMM_WORLD);
            std::stringstream ss;
            ss<<::mpi12s::info<<rank<<':';
            for(size_t i = 0; i < 15; ++i) {
                ss<<a[i]<<' ';
            }   std::cout<<ss.str()<<std::endl;
        }
        for(size_t i = 0; i < 15; ++i) {
            ok &= a[i] == i;
            if( !ok ) std::cout<<::mpi12s::info<<"oops"<<std::endl;
        }
        finalize();

        std::cout<<::mpi12s::info<<"ok="<<ok<<std::endl;

        return ok;
    }
 //---------------------------------------------------------------------------------------------------------------------
}// namespace test4

namespace test5
{//---------------------------------------------------------------------------------------------------------------------
    void print_a(char const* msg, int const * const a)
    {
        std::stringstream ss;
        ss<<::mpi12s::info + msg;
        for(size_t i = 0; i < 15; ++i) {
            ss<<a[i]<<' ';
        }   ss<<'\n';
        printf( "%s", ss.str().data() );
    }

    bool test()
    {// This test broadcasts an array in which every rank has initialized its own section:
     // rank 0 : 0 1 2 3 0 0 0 0 0 0 0 0 0 0 0
     // rank 1 : 4 5 6 7 8 0 0 0 0 0 0 0 0 0 0
     // rank 2 : 9 10 11 12 13 14 0 0 0 0 0 0 0 0 0
     // After broadcasting the array is complete on every rank, the values are not in sorted order.
     // Note that each rank broadcasts a different number of items (resp. 4, 5 and 6) and the the buffer on the
     // receiving end does not use the same memory location. So, after broadcasting, every rank has all the values
     // but not in the sorted order. (But this is intentially so).
        init();

     // // Get number of processes and check that 3 processes are used
        if(::mpi12s::size != 3)
        {
            printf("This application is meant to be run with 3 MPI processes.\n");
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
        bool ok = true;
        int a[15];
        for(size_t i = 0; i < 15; ++i) {
            a[i] = 0;
        }
        switch( rank) {
            case 0:
                for(size_t i = 0; i < 4; ++i) {
                    a[i] = i;
                }   break;
            case 1:
                for(size_t i = 0; i < 5; ++i) {
                    a[i] = 4 + i;
                }   break;
            case 2:
                for(size_t i = 0; i < 6; ++i) {
                    a[i] = 9 + i;
                }   break;
        }
        print_a("initialized:", a);

     // This approach here works only because the receivers know how much data to receive (4 ints from rank 0,
     // 5 from rank 1 and 6 from rank 2.
        int source = 0;
        switch( rank) {
            case 0:
                MPI_Bcast(&a[0], 4, MPI_INT, source, MPI_COMM_WORLD);
                break;
            case 1:
                MPI_Bcast(&a[5], 4, MPI_INT, source, MPI_COMM_WORLD);
                break;
            case 2:
                MPI_Bcast(&a[6], 4, MPI_INT, source, MPI_COMM_WORLD);
                break;
        }
        print_a("rank 0 broadcasted:", a );

        ++source;
        switch( rank) {
            case 0:
                MPI_Bcast(&a[ 4], 5, MPI_INT, source, MPI_COMM_WORLD);
                break;
            case 1:
                MPI_Bcast(&a[ 0], 5, MPI_INT, source, MPI_COMM_WORLD);
                break;
            case 2:
                MPI_Bcast(&a[10], 5, MPI_INT, source, MPI_COMM_WORLD);
                break;
        }
        print_a("rank 1 broadcasted:", a );

        ++source;
        switch( rank) {
            case 0:
                MPI_Bcast(&a[9], 6, MPI_INT, source, MPI_COMM_WORLD);
                break;
            case 1:
                MPI_Bcast(&a[9], 6, MPI_INT, source, MPI_COMM_WORLD);
                break;
            case 2:
                MPI_Bcast(&a[0], 6, MPI_INT, source, MPI_COMM_WORLD);
                break;
        }
        print_a("rank 2 broadcasted:", a );

        switch( rank) {
            case 0: {
                    int expected[15] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
                    for(size_t i = 0; i < 15; ++i) {
                        ok &= a[i] == expected[i];
                        if( !ok ) std::cout<<::mpi12s::info<<"oops"<<std::endl;
                    }
                }   break;
            case 1: {
                    int expected[15] = {4, 5, 6, 7, 8, 0, 1, 2, 3, 9, 10, 11, 12, 13, 14};
                    for(size_t i = 0; i < 15; ++i) {
                        ok &= a[i] == expected[i];
                        if( !ok ) std::cout<<::mpi12s::info<<"oops"<<std::endl;
                    }
                }   break;
            case 2: {
                    int expected[15] = {9, 10, 11, 12, 13, 14, 0, 1, 2, 3, 4, 5, 6, 7, 8};
                    for(size_t i = 0; i < 15; ++i) {
                        ok &= a[i] == expected[i];
                        if( !ok ) std::cout<<::mpi12s::info<<"oops"<<std::endl;
                    }
                }   break;
        }
        finalize();

        std::cout<<::mpi12s::info<<"ok="<<ok<<std::endl;

        return ok;
    }
 //---------------------------------------------------------------------------------------------------------------------
}// namespace test5

namespace test6
{//---------------------------------------------------------------------------------------------------------------------
    typedef Eigen::Matrix<float, 3, 1, Eigen::DontAlign> vec_t;
    typedef float value_t;

    class ParticleContainer
    {
        std::vector<bool> alive_;
    public:
        std::vector<value_t> r;
        std::vector<value_t> m;
//        std::vector<vec_t>   x;

        ParticleContainer(int size)
        {
            alive_.resize(size);
            r.resize(size);
            m.resize(size);
//            x.resize(size);
            for( int i=0; i<size; ++i) {
                int ir = 100*rank + i;
                alive_[i] = true;
                r[i] = ir;
                m[i] = ir + size;
//                for( int k=0; k<3; ++k )
//                    x[i][k] = ir+k*size;
            }
        }

        void prdbg()
        {
            std::stringstream ss;
            Lines_t lines;

            ss<<std::setw(4)<<"i"
              <<std::setw(4)<<"r"
              <<std::setw(4)<<"m"
//            <<std::setw(4)<<"x0"<<std::setw(4)<<"x1"<<std::setw(4)<<"x2"
              ;
            lines.push_back(ss.str()); ss.str(std::string());

            int size = alive_.size();
            for( int i=0; i<size; ++i) {
                if( alive_[i] ) {
                    ss<<std::setw(4)<<i
                      <<std::setw(4)<<r[i]
                      <<std::setw(4)<<m[i]
    //                     <<'['<<std::setw(4)<<x[i][0]<<std::setw(4)<<x[i][1]<<std::setw(4)<<x[i][2]<<"]"
                      ;
                    lines.push_back(ss.str()); ss.str(std::string());
                }
            }
            ::mpi12s::prdbg( tostr("ParticleContainer(size=", size, ")"), lines );
        }

        size_t // the size of a particle container.
        size() const {
            return alive_.size();
        }

     // Grow the arrays by a factor 1.5, and return the position of the first new element
     // (which is not alive, obviously).
        int grow()
        {
            int old_size = alive_.size();
            int new_size = (int)(old_size*1.5);
            if (new_size == old_size) ++new_size;
            alive_.resize(new_size);
//            x.resize(new_size);
            r.resize(new_size);
            m.resize(new_size);
            for (int i=old_size; i<new_size; ++i) {
                alive_[i] = false;
            }
            return old_size;
        }

     // Find an index for a new particle
        int add()
        {// look for a dead particle
            for( int i=0; i<alive_.size(); ++i) {
                if (not alive_[i]) {
                    alive_[i] = true;
                    return i;
                }
            }// none found
         // grow the array.
            return grow();
        }

     // remove an element
        void remove(int i)
        {
            alive_[i] = false;
        }

     // test if alive
        bool is_alive(int i) const {
            return alive_[i];
        }
    };

    class MessageHandler : public ::mpi2s::MessageHandlerBase
    {
        ParticleContainer& pc_;
        std::vector<value_t> ri;
        std::vector<value_t> mi;
        std::vector<vec_t  > xi;
    public:
        MessageHandler(ParticleContainer& pc)
          : pc_(pc)
        {
            message().push_back(ri);
            message().push_back(mi);
//            message().push_back(xi);
        }

        virtual
        void postMessage(std::vector<int>& indices, int to_rank)
        {
            ri.clear();
            mi.clear();
            xi.clear();

            {// assemble the non-contigous data into a contiguous array
                for( auto index: indices) {
                    ri.push_back(pc_.r[index]);
                    mi.push_back(pc_.m[index]);
//                    xi.push_back(pc_.x[index]);
                 // remove the particle from the pc:
                    pc_.remove(index);
                }
                MessageHandlerBase::postMessage(to_rank);
                int n = ri.size();
            }
         // clearing is not necessary. Here just to make sure that the receivers are correctly resized.
            xi.clear();
            ri.clear();
            mi.clear();
        }

        virtual
        bool
        readMessage
          ( Index_t msg_id // the message id identifies the message header
          )
        {
            bool ok = MessageHandlerBase::readMessage(msg_id);
            if(ok)
            {// copy back to PC
                const size_t n = ri.size();
                for( size_t i = 0; i < n; ++i) {
                    Index_t j = pc_.add();
                    pc_.r[j] = ri[i];
                    pc_.m[j] = mi[i];
                }
            }
            return ok;
        }
    };

    bool test()
    {
        init();
     // Todo move this into the init call above
        ::mpi12s::theMessageBuffer.initialize(1000, 10);

        ParticleContainer pc(8);
        pc.prdbg();

        bool ok = true;
        MessageHandler mh(pc);
     // move the odd particles to the next rank
     // there is one message for each process
        std::vector<int> indices = {1,3,5,7};

        mh.postMessage(indices, next_rank());

        mpi12s::theMessageBuffer.broadcast();
        mpi12s::theMessageBuffer.readMessages();
        pc.prdbg();

//        mh.readMessage();

        Lines_t lines;
        for(size_t i=0; i<pc.size(); ++i )
        {
            if( pc.is_alive(i) )
            {
                std::stringstream ss;
                ss<<std::setw(4)<<i
                  <<std::setw(4)<<pc.r[i]
                  <<std::setw(4)<<pc.m[i]
//                  <<" ["<<std::setw(4)<<pc.x[i][0]<<std::setw(4)<<pc.x[i][1]<<std::setw(4)<<pc.x[i][2]<<"]"
                  <<" : ";
             // verify contents:
                int const   my_rank = rank;
                int const prev_rank = next_rank(-1);
                value_t expected_r = ( i%2 ? 100*prev_rank + i // odd i
                                           : 100*my_rank + i   //even i
                                     );
                value_t expected_m = expected_r + 8;
    //            vec_t   expected_x = vec_t(expected_r, expected_r+8, expected_r+16);

                ok &= pc.r[i] == expected_r;
                ss<<' '<<ok;

                ok &= pc.m[i] == expected_m;
                ss<<','<<ok;
    //            ok &= pc.x[i] == expected_x;
    //            ss <<' '<<ok<<;

                lines.push_back(ss.str());
            }
        }
        prdbg("pc verify", lines);

        std::cout<<::mpi12s::info<<" done"<<std::endl;
        finalize();
        return ok;
    }
 //---------------------------------------------------------------------------------------------------------------------
}// namespace test6

PYBIND11_MODULE(core, m)
{// optional module doc-string
    m.doc() = "pybind11 core plugin"; // optional module docstring
 // list the functions you want to expose:
 // m.def("exposed_name", function_pointer, "doc-string for the exposed function");
//    m.def("test_hello", &test_hello, "");
    m.def("test1", &test1::test, "");
    m.def("test2", &test2::test, "");
    m.def("test3", &test3::test, "");
    m.def("test4", &test4::test, "");
    m.def("test5", &test5::test, "");
    m.def("test6", &test6::test, "");
}
