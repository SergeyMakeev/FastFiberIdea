#include <stdio.h>
#include <atomic>

//
//
// https://github.com/SergeyMakeev/deboost.context/tree/master/asm
// https://docs.microsoft.com/en-us/cpp/build/x64-software-conventions?view=msvc-170
// https://docs.microsoft.com/en-us/cpp/build/x64-calling-convention?view=msvc-170
// 
// Goroutine is very lightweight
// The cost of context switching is low: Goroutine context switching involves only the modification of the value of three registers(PC / SP / DX)
// https://morioh.com/p/36af32e3f52c
//


#ifdef __cplusplus
extern "C" {
#endif

	struct context_t
	{
		uintptr_t rbp; // ??
		uintptr_t rsp;
		uintptr_t rsi; // are those really need?
		uintptr_t rdi; // ??
		uintptr_t rip;
	};

	extern void save_context(context_t& ctx);
	extern void restore_context(context_t& ctx);



#ifdef __cplusplus
}
#endif


void save(context_t& ctx)
{
	std::atomic_thread_fence(std::memory_order_seq_cst);
	save_context(ctx);
}

void restore(context_t& ctx)
{
	std::atomic_thread_fence(std::memory_order_seq_cst);
	restore_context(ctx);
}

int main()
{
	// compile and run using 'RelWithDebInfo'
	// how to avoid using volatile keyword?
	volatile int counter = 0;
	context_t ctx;

	printf("Step1\n");

	save(ctx);

	printf("Step2 [%d]\n", counter);
	counter++;

	if (counter < 100)
	{
		restore(ctx); // this will "jump" to the next line after save() function
	}

	printf("Step3\n");

	return 0;
}

