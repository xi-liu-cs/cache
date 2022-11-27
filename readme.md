# cache
Xi Liu<br>
2021<br>
<br>
The first part of the project contains an implementation of main memory. In this simulated system,
data is written to and read from the main memory as entire 8-word cache lines, not individual
words (which are 32 bits).<br>
<br>
The second part of the project contains an implementation of a direct-mapped, write-back L2 cache. As with
the main memory part, in this simulated system data is written to and read from the L2 cache as entire 8-word cache lines, not individual words (which are 32
bits).<br>
<br>
The third part of the project contains an implementation of 4-way set associative, write-back L1 cache. Unlike the main memory and L2 cache, data is read from and written to the L1 cache (by the CPU) one 32-bit word at a time. Following an L1 cache miss, however, an entire 8-word cache line is inserted into the L1 cache.<br>
<br>
The fourth and last part of the memory subsystem phase of the project contains an implementation of a simulated controller for the memory subsystem. This controller manages the movement of data among the CPU, the L1 cache, the L2 cache, and main memory.
