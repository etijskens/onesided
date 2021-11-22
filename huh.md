### reflection

#### why did wee use MPI_Get? rather than MPI_Put?

MPI_Get requires peeking into every other rank to see if it has something to tell 
to me...

Every rank knows for whom a message is, so, it could directly put it in the 
destination's window. The problem is that when two processes are putting into the 
same window, there is now way of knowing where to put the two messages in a way that 
they do not interfere. 

#### Contiguous data

The data item (or contiguous data items) is memcpy-ed into the processor's window.
A data item is a memory location of a memcpy-able block of data. 
From the window it is read (MPI_Get) into a read buffer by another proces. 
The MessageHandler then memcpy-s it back into the memory location on MPI_Get-ing 
side. The data put will replace ***exactly*** the same data item (=variable) on the 
get side. As such, the MessageHandler is ***symmetrical***. Sofar, a message is composed 
of a list (std::vector) of contiguous data items. 

#### Non-contiguous data

However, what we will mostly need is moving or copying a set of particles in a 
particle container to another proces (the receiving end). At the receiving end, 
the particles must be created and initialized with the data in the message. On the
sending side, the particles will be kept, as for ghost particles, or removed, as 
for leaving particles. Thus, this type of MessageHandler is ***asymmetrical***. 
If we can manage to put particle after particle in the window, i.e. 

    x[i0], v[i0], a[i0], r[i0], m[i0], ...,
    x[i1], v[i1], a[i1], r[i1], m[i1], ...

rather than array by array

    x[:], v[:], a[:], r[:], m[:], ... 

the writer should iterate over the selected particles, and put them in the window,
while the reader should iterate over the same number of particles, but using each
time the index of a new particle. The message is more complicated, but the asymmetry 
more manageable. 

Another difficulty of ParticleContainers is that particles are non-contiguous data
structures themselves, as they are a SoA (structure of arrays) and not a Aos (array 
of structures). Obviously, on the receiving end, all arrays must use the same 
indices for new particles to initialize the new particles correctly. Thus a particle 
array message must have access to the index list message.

This could be an approach:

1. send the index list of particles that must be moved or copied.
2. send the parts of the particle arrays that correspond to that index list.
3. read the index list, but replace the values with indices for new particles
4. read the particle arrays, and use the replaced index list to put the particle
   array value at the new particle positions.

In principle, it should be possible to only send the length of the index array. 
The indices themselves are useless, as they must be replaced by indices of new 
particles. This makes the message a little shorter. 

