# FastFiberIdea

The idea was to make a very lightweight fiber library, similar to goroutines.
Unfortunately, there is no way to say a compiler to save state stored in CPU registers to memory (compiler barrier on steroids?)
so all the variables between save/restore need to be marked as volatile, which contradicts the original idea.