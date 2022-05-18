# FastFiberIdea

The idea was to make a very lightweight fiber library, similar to goroutines.

Unfortunately, there is no way to force a C++ compiler to spill currently used CPU registers to memory (optimization barrier or smth?)  
https://twitter.com/SergeyMakeev/status/1526363705947983872?s=20&t=qqSievVEK0RqKI4op6N9Lw  

Due to this the context is pretty "heavy" 104 bytes (not including 10 nonvolatile xmm registers)
