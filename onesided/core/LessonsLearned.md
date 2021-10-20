## MPI one-sided communication - Lessons learned

### MPI one-sided communication is non-blocking

The result of a put or a get is only available after the epoch is closed. 

### Window creation

the window may be created before or after writing the message.  Typically,
messages for MPI_Get are written outside a window epoch.   