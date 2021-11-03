#ifndef MEMCPY_ABLE_H
#define MEMCPY_ABLE_H

namespace mpi
{//-------------------------------------------------------------------------------------------------
 // Compile-time detection of types T that are 
 // (1) contiguous and of fixed size: so its size is determined with sizeof(T) or sizeof(t) and 
 //     memcpy-ing a T copies its entire state.
 //     E.g. : int, double, float, .., but also float[3], double[4], ... 
 //     = fixed_length_memcpy_able
 // (2) a contiguous collection of type (1) objects, whose  size can be determined with a size()
 //     function. The collection type C must provide:
 //       - size_t size() member function
 //       - value_type typedef
 //       - value_type* data(), pointing to the beginning of the collection 
 //     std::vector<T> and std::string satisfy these requirements 
 //     = variable_length_memcpy_able
 // Check out:
 //   https://akrzemi1.wordpress.com/2017/12/02/your-own-type-predicate/
 //   https://stackoverflow.com/questions/33067983/type-traits-contiguous-memory
 //-------------------------------------------------------------------------------------------------

 
}// namespace mpi
#endif // MEMCPY_ABLE_H
