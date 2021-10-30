#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H

#include <map>
#include "MessageBox.h"

namespace mpi 
{
    class MessageHandlerBase; // forward declaration 

 //------------------------------------------------------------------------------------------------
   class MessageHandlerFactory
 //------------------------------------------------------------------------------------------------
    {
    public:
        MessageHandlerFactory();
        ~MessageHandlerFactory();
        
     // Create a MessageHander, store it in the registry_, and return a pointer to it.
        template<typename MessageHandlerT>
        MessageHandlerT&
        create()
        {
         // maybe assert that MessageHandlerT derives from MessageHandlerBase
            size_t key = counter_++;
            MessageHandlerT* mh = new MessageHandlerT(key);
         // maybe assert whether key does not already exist.
            registry_[key] = mh;
            
            return *mh;
        }

        MessageHandlerBase* operator[](size_t key);

    private:
        size_t counter_;
        std::map<size_t,MessageHandlerBase*> registry_;
    };

    static MessageHandlerFactory theMessageHandlerFactory;

  //------------------------------------------------------------------------------------------------
   class MessageHandlerBase
  //------------------------------------------------------------------------------------------------
   {
    public:
        MessageHandlerBase(size_t key);

     // Write/read a message to/from a buffer.
        virtual bool handlie(char mode) = 0;
         // Returns false if the message is empty, true otherwise.

     // Post the message (i.e. put it in the MPI window.)
        void post(int to_rank);

     // put a message in the MessageStream for deserialization
        void setMessage(std::string const & message);

     // 
        inline size_t key() const { return key_; }
    protected:
        size_t key_;
    };
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
 // MessageStreamHelper class
 // This class does the work of the MessageStream class
 //---------------------------------------------------------------------------------------------------------------------
 // Only specialisations are functional.
    template<typename T, int> class MessageStreamHelper;

 //---------------------------------------------------------------------------------------------------------------------
 // MessageStreamHelper specialisation for selected built-in types
 //---------------------------------------------------------------------------------------------------------------------
    template<typename T>
    class MessageStreamHelper<T,built_in_type>
    {
        typedef MessageStreamHelper<T,built_in_type> This_type;
    public:
     // Write or read a T object, select using mode argument
     // Even when mode == 'w', t must be non-const.
        static void handle(T& t, std::stringstream& ss, char mode)
        {
            std::cout<<"handle not is_array_like, mode="<<mode<<std::endl;
            if (mode == 'w') {
                This_type::write(const_cast<T const &>(t),ss);
            } else if (mode == 'r') {
                This_type::read(t,ss);
            } else {
                throw std::invalid_argument("Mode should be 'r' (read) or 'w' (write).");
            }
        }
        
    private:
        static
        void write(T const & t, std::stringstream& ss)
        {
            std::cout<<"write not is_array_like"<<std::endl;
            ss.write( reinterpret_cast<std::ostringstream::char_type const *>(&t), sizeof(T) );
        }
        
        static
        void read(T & t, std::stringstream& ss)
        {
            std::cout<<"read not is_array_like"<<std::endl;
            ss.read( reinterpret_cast<std::ostringstream::char_type *>(&t), sizeof(T) );
        }
    };

 //---------------------------------------------------------------------------------------------------------------------
 // MessageStreamHelper specialisation for selected array-like types
 //---------------------------------------------------------------------------------------------------------------------
 // class T must have
 //   - T::value_type, the type of the array elements
 // T objects t must have
 //   - t.data() returning T::value_type* or T::value_type const*, pointer to begin of te data buffer
 //   - t.size() returning size_t, number of array elements
 // This is ok for std::vector<T> with T a built-in type , std::string
    template<typename T> 
    class MessageStreamHelper<T,array_like_type>
    {
        typedef MessageStreamHelper<T,array_like_type> This_type;
    public:
     // Write or read a T object, select using mode argument
     // Even when mode == 'w', t must be non-const.
        static void handle(T& t, std::stringstream& ss, char mode)
        {
            std::cout<<"handle is_array_like, mode="<<mode<<std::endl;
            if (mode == 'w') {
                This_type::write(const_cast<T const &>(t),ss);
            } else if (mode == 'r') {
                This_type::read(t,ss);
            }
        }
    
    private:
     // write a T object
        static
        void write(T const & t, std::stringstream& ss)
        {
            std::cout<<"write is_array_like"<<std::endl;
            size_t const sz = t.size();
            std::cout<<"write size: "<<sz<<std::endl;
            ss.write(reinterpret_cast<std::stringstream::char_type const *>(&sz), sizeof(size_t));
            ss.write(reinterpret_cast<std::stringstream::char_type const *>(t.data()), sz*sizeof(typename T::value_type));
            std::cout<<"write done "<<std::endl;
        }

     // read a T object
        static
        void read(T & t, std::stringstream& ss)
        {
            std::cout<<"read is_array_like"<<std::endl;
            size_t sz=0;
            ss.read(reinterpret_cast<std::ostringstream::char_type*>(&sz), sizeof(size_t));
            std::cout<<"read.size "<<sz<<std::endl;
            t.resize(sz);
            std::cout<<"read size: "<<t.size()<<std::endl;
            ss.read(reinterpret_cast<std::ostringstream::char_type*>(t.data()), sz*sizeof(typename T::value_type));
            std::cout<<"read done"<<std::endl;
        }
        
    };
}// namespace mpi
#endif // MESSAGEHANDLER_H
