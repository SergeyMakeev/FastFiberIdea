# FastFiberIdea

The idea was to make a very lightweight fiber library, similar to goroutines.

Unfortunately, there is no way to force a C++ compiler to spill currently used CPU registers to memory (optimization barrier or smth?)  
https://twitter.com/SergeyMakeev/status/1526363705947983872?s=20&t=qqSievVEK0RqKI4op6N9Lw  

Due to this the context is pretty "heavy" 104 bytes (not including 10 nonvolatile xmm registers)


# API

API is super simple

1 Creates a new context/fiber (you don't need to call ConvertThreadToFiber or anything like that)  
 `create(context_t* result, fiber_function, stack_top_ptr, stack_base_ptr);`

2 Automatically saves the current execution context to *from* and switches to the *to*  
`switch_to(context_t* from, context_t* to);`

3 Return the current context environment including a pointer to the current context and a pointer to the previous context to be able to switch back  
`context_env_t get_environment();`
  
  
P.S. sizeof(context_t) = 104 bytes (note! it does not include 10 nonvolatile xmm registers XMM6:XMM15)
