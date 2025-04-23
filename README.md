# Computer-technology
## Pipe
This code simulates communication between clients and the server. Clients and the server are connected via pipes with a size not a multiple of the power of two. All buffers are of different sizes.

Synchronization of client access is carried out via select.
## Shared memory
This code simulates communication between two clients - the sender and the recipient of the message. 
Clients are synchronized via semaphores. Also they use shared memory. 

When one of the clients dies, the second one will be closed. 
One sender and one recipient can communicate at the same time. The others are waiting. 

Functions "clean" and "check" are needed to delete  segment of shared memory and semaphores and to check that segments  with id created by keys are free
## Signals
The code is designed to test the capabilities of POSIX signals in Linux. The program creates a child process and communicates with it. Information is transmitted via SIGUSR1 and SIGUSR1, as a sequence of 0 and 1. De facto synchronization is carried out on the signals

I express my gratitude to Morgen Matvey (@Melges) for the idea and description of the implementation
