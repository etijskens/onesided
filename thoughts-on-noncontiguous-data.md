### Thoughts on non-contiguous data and one-sided communications

#### why do we use MPI_Get instead of MPI_Put? Or, is onesided communication 
even the best solution possible?

MPI_Get requires peeking into **every** other rank to see if it has any messages 
for me... That means that the communication scales as the square of the number of 
processes, N. This is not so good, even if, initially at least, we expect N to be 
rather small, say ~10-20. Note that this is in principle *N x one-to-all* 
communication, which can be optimised by the MPI implementation. It is, however,
doubtfull that the MPI implementation can recognise this pattern, so it will 
most probably be executed as an *N x N x one-to-one* communication, which is not
optimised. This pattern does not exploit the fact that every rank knows to whom 
it must send a message. 

However, every rank knows for whom a message is, so, it could directly put it in the 
destination's window. The problem is that when two processes are putting into the 
same window, there is now way of knowing where to put the two messages in a way that 
they do not interfere.

As a solution for this, we could use two-sided communication MPI_Bcast to send the 
message headers to all other processors. This is almost surely more efficient than
the above *N x N MPI_Get* approach. Furthermore, After this, again two-sided 
point-to-point communication can be used to communicate the message as the receiving 
ends now know what to receive. (Note that you can use a non-blocking send, and pair 
it with a blocking receive). 

What needs to be modified to turn the one-sided approach into a two-sided approach?

- we do no longer need a MessageBox class but the buffers need to be stored 
  somewhere, as well as the functions for communiation 
- we need a header buffer for every other process, 
- we need a message buffer for every other process, or reuse the message buffers.
  Can we afford a message buffer for every process? I guess not, this is also a 
  scaling issue.
- header buffers and message buffers may be separated.
- the MessageHandlerBase class does not need a MessageBox class

The MessageBox object could be replaced with a Messenger object: it stores the 
buffers, broadcasts the headers, and acts upon the received headers.

##### Plan

1. Split header and message buffers because they are sent separately (?)

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
sending side, the particles will be kept, as for ghost particles of the domain, or
removed, as for particles leaving the domain. Thus, this type of MessageHandler is 
***asymmetrical*** in the sense that the memory location of the source and 
destination of the data differs, i.e. they are held in a different variable.
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

Another question that is to be solved, is wether the particle properties need to 
be sent as SoA or AoS. If we want to avoid copying the noncontiguous data into a 
contiguous data array (which is what we did so far), we need a memcopy for each 
particle property and each element: from the particle container to the window on 
the sending side, and from the readBuffer to the particle container on the reading 
side. As AoS means that the loop over the element indices (which are stored as a 
contiguous list) is made only once, we suspect that there are a little less cache 
misses than with SoA.

### Useful links

- https://stackoverflow.com/questions/26166879/does-mpi-send-need-to-be-called-before-the-corresponding-mpi-recv