/*
 *  C++ source file for module onesided.core
 */


// See http://people.duke.edu/~ccc14/cspy/18G_C++_Python_pybind11.html for examples on how to use pybind11.
// The example below is modified after http://people.duke.edu/~ccc14/cspy/18G_C++_Python_pybind11.html#More-on-working-with-numpy-arrays
//#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

namespace py = pybind11;

#include "mpi1s.cpp"
#include "MessageBuffer.cpp"
#include "MessageBox.cpp"
#include "Message.cpp"
#include "MessageHandler.cpp"

#include <stdexcept>


using namespace mpi1s;

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
    class MessageHandler : public MessageHandlerBase
    {
    public:
        MessageHandler(MessageBox& mb)
          : MessageHandlerBase(mb) 
        {// initialize some data to be sent
            i_ = i0;
            d_ = d0;
            vi_.push_back(vi0);
            vi_.push_back(vi0+1);
            vi_.push_back(vi0+2);
            if( ::mpi1s::rank == 1 ) {
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
                    std::string errmsg = ::mpi1s::info + "verified = false.";
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
        MessageBox mb(1000,10);
        int const rank = ::mpi1s::rank;
        MessageHandler mh(mb);
        if( rank == 0 ) {
            std::cout<<::mpi1s::info<<"posting to 1"<<std::endl;
            mh.putMessage(1);
        } else {
            std::cout<<::mpi1s::info<<std::endl;
        }
        
        mh.getMessages();
    
        bool msg_ok = mh.verify();
        std::cout<<::mpi1s::info
                 <<( rank == 0 ? "poster " : (rank == 1 ? "receiver " : "neutral "))
                 <<"ok = "<<ok<<std::endl;
        
        ok &= msg_ok;
        return ok;
    }
 //---------------------------------------------------------------------------------------------------------------------
}// namespace test1

namespace test2
{//---------------------------------------------------------------------------------------------------------------------
    class MessageHandler : public MessageHandlerBase
    {
        typedef Eigen::Matrix<float, 3, 1, Eigen::DontAlign> vec_t;

    private:
        int              i_;
        double           d_;
        vec_t            v_;
        std::vector<int> a_;
        int              rank_;

    public:
        MessageHandler(MessageBox& mb)
          : MessageHandlerBase(mb)
        {// initialize some data to be sent
         // we will put to the right rank and get from the left rank

            rank_ = ::mpi1s::rank;

            i_ = rank_;
            d_ = rank_;
            v_ = vec_t(rank_,rank_,rank_);
            a_ = {rank_,rank_};

            std::cout
                <<'\n'<<::mpi1s::info<<"setting i_ = "<<i_
                <<'\n'<<::mpi1s::info<<"setting d_ = "<<d_
                <<'\n'<<::mpi1s::info<<"verify v_ = ["<<v_<<']'
                <<'\n'<<::mpi1s::info<<"verify a_ = ["<<a_[0]<<' '<<a_[1]<<']'
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
                <<'\n'<<::mpi1s::info<<"verify i_ = "<<i_
                <<'\n'<<::mpi1s::info<<"verify d_ = "<<d_
                <<'\n'<<::mpi1s::info<<"verify v_ = ["<<v_<<']'
                <<'\n'<<::mpi1s::info<<"verify a_ = ["<<a_[0]<<' '<<a_[1]<<']'
                <<std::endl;

            int left = next_rank(-1);
            std::cout<<::mpi1s::info<<"received from "<<left<<std::endl;

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
        ::mpi1s::init();

        bool ok = true;
        {
            MessageBox mb(1000,10);
            int rank = ::mpi1s::rank;
            int right_rank = next_rank();
    
            MessageHandler mh(mb);
            std::cout<<::mpi1s::info<<"posting to "<<right_rank<<std::endl;
            mh.putMessage(right_rank);
            
            mh.getMessages();
    
            bool msg_ok = mh.verify();
    
            std::cout<<::mpi1s::info
                    <<( rank == 0 ? "poster " : (rank == 1 ? "receiver " : "neutral "))
                    <<"ok = "<<ok<<std::endl;
            
            ok &= msg_ok;
            std::cout<<::mpi1s::info<<"end of MessageBox scope"<<std::endl;
        }
        std::cout<<::mpi1s::info<<"done"<<std::endl;
        ::mpi1s::finalize();
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
//        std::vector<vec_t> x;
        std::vector<value_t> r;
        std::vector<value_t> m;

        ParticleContainer(int size)
        {
            std::stringstream ss;
            ss<<mpi1s::info<<'\n';
            alive_.resize(size);
//            x.resize(size);
            r.resize(size);
            m.resize(size);
            for( int i=0; i<size; ++i) {
                int ir = 100*rank + i;
                alive_[i] = true;
//                for( int k=0; k<3; ++k )
//                    x[i][k] = ir;
                r[i] = ir;
                m[i] = ir + size;
                ss<<i<<std::setw(4)<<r[i]<<std::setw(4)<<m[i]<<'\n';
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

    class MessageHandler : public MessageHandlerBase
    {
        ParticleContainer& pc_;
//        std::vector<vec_t> xi;
        std::vector<value_t> ri;
        std::vector<value_t> mi;
    public:
        MessageHandler(MessageBox& mb, ParticleContainer& pc)
          : MessageHandlerBase(mb)
          , pc_(pc)
        {
//            message().push_back(xi);
            message().push_back(ri);
            message().push_back(mi);
        }

        void putMessage(std::vector<int>& indices, int to_rank)
        {
//            xi.clear();
            ri.clear();
            mi.clear();
            {// assemble the non-contigous data into a contiguous array
                for( auto index: indices) {
//                    xi.push_back(pc_.x[index]);
                    ri.push_back(pc_.r[index]);
                    mi.push_back(pc_.m[index]);
                 // remove the particle from the pc:
                    pc_.remove(index);
                }
                MessageHandlerBase::putMessage(to_rank); // from rank 0 to rank 1
                int n = ri.size();
                std::cout<<mpi1s::info<<"sending "<<n<<" items to "<<to_rank<<'\n';
                for( int i=0; i<n; ++i) {
//                    std::cout<<mpi1s::info<<i<<" ["<<xi[i][0]<<' '<<xi[i][1]<<' '<<xi[i][2]<<"] "<<ri[i]<<'\n';
                    std::cout<<mpi1s::info<<i<<' '<<ri[i]<<' '<<mi[i]<<'\n';
                }   std::cout<<std::endl;
            }
         // clearing is not necessary. Here just to make sure that the receivers are correctly resized.
//            xi.clear();
            ri.clear();
            mi.clear();
        }

        void getAllMessages()
        {// this must be executed by ALL processes
            std::cout<<mpi1s::info<<"sizes before= "<<ri.size()<<','<<mi.size()<<std::endl;
            getMessages();
            std::cout<<mpi1s::info<<"sizes after = "<<ri.size()<<','<<mi.size()<<std::endl;
            int n = ri.size();
            std::cout<<mpi1s::info<<n<<" items received\n";
            for( int i=0; i<n; ++i) {
//                std::cout<<mpi1s::info<<i<<" ["<<xi[i][0]<<' '<<xi[i][1]<<' '<<xi[i][2]<<"] "<<ri[i]<<'\n';
                std::cout<<mpi1s::info<<i<<' '<<ri[i]<<' '<<mi[i]<<'\n';
            }   std::cout<<std::endl;
         // copy to the particle container:
            std::vector<int> indices(n);
            std::stringstream ss;
            ss<<info<<"adding elements ";
            for( size_t i = 0; i<n; ++i ) {
                indices[i] = pc_.add();
                ss<<indices[i]<<' ';
            }   std::cout<<ss.str()<<std::endl;
            for( size_t i = 0; i<n; ++i ) {
                pc_.m[indices[i]] = mi[i];
                pc_.r[indices[i]] = ri[i];
            }
        }
    };

    bool test()
    {
        mpi1s::init();

        ParticleContainer pc(8);

        bool ok = true;
        {
            MessageBox mb(1000,10);
            MessageHandler mh(mb, pc);
            if(mpi1s::size <= 3)
            {// move the odd particles to the other rank
             // there is one message forr each process
                std::vector<int> indices = {1,3,5,7};
                mh.putMessage(indices, mpi1s::next_rank());
                mh.getAllMessages();
            }

            std::cout<<::mpi1s::info<<"end of MessageBox scope"<<std::endl;
         // destroy MessageBox mb, must come before mpi1s::finalize()
        }
        std::stringstream ss;
        ss<<info<<"pc\n";
        for(size_t i=0; i<pc.size(); ++i ) {
            if( pc.is_alive(i) ) {
                ss<<'['<<i<<"] "<<std::setw(4)<<pc.r[i]<<std::setw(4)<<pc.m[i]<<' ';
            }
         // verify contents:
            int const   my_rank = mpi1s::rank;
            int const prev_rank = mpi1s::next_rank(-1);
            value_t expected_r = ( i%2 ? 100*prev_rank + i // odd i
                                       : 100*my_rank + i   //even i
                                 );
            value_t expected_m = expected_r + 8;
            ok &= pc.r[i] == expected_r;
            ss<<ok<<' ';
            ok &= pc.m[i] == expected_m;
            ss<<ok<<'\n';
        }  std::cout<<ss.str()<<std::endl;

        std::cout<<::mpi1s::info<<"done"<<std::endl;
        mpi1s::finalize();
        return ok;
    }
 //---------------------------------------------------------------------------------------------------------------------
}// namespace test3


PYBIND11_MODULE(core, m)
{// optional module doc-string
    m.doc() = "pybind11 core plugin"; // optional module docstring
 // list the functions you want to expose:
 // m.def("exposed_name", function_pointer, "doc-string for the exposed function");
//    m.def("test_hello", &test_hello, "");
    m.def("test1", &test1::test, "");
    m.def("test2", &test2::test, "");
    m.def("test3", &test3::test, "");
}
