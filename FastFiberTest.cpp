#include <windows.h>
#include <stdio.h>
#include <atomic>

//
//
// https://github.com/SergeyMakeev/deboost.context/tree/master/asm
// https://docs.microsoft.com/en-us/cpp/build/x64-software-conventions?view=msvc-170
// https://docs.microsoft.com/en-us/cpp/build/x64-calling-convention?view=msvc-170
// https://docs.microsoft.com/en-us/cpp/build/stack-usage?view=msvc-170
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
		uintptr_t rbp; // ?? may be used as a frame pointer; must be preserved by callee
		uintptr_t rsp; // stack pointer
		uintptr_t rsi; // are those really need?
		uintptr_t rdi; // ??
		uintptr_t rip;
	};

	typedef void (*pfn_function)();

	extern void save_context(context_t& ctx);
	extern void restore_context(context_t& ctx);
	extern void make_context(context_t& ctx, pfn_function func, void* sp, size_t stack_size);

#ifdef __cplusplus
}
#endif


// rax    - volatile (return value)
// rcx    - volatile (first integer arg)
// rdx    - volatile (second integer arg)
// r8     - volatile (third integer arg)
// r9     - volatile (fourth integer arg)
// r10:11 - volatile


#define save(ctx) \
	std::atomic_thread_fence(std::memory_order_seq_cst); \
	save_context(ctx); \

#define restore(ctx) \
	std::atomic_thread_fence(std::memory_order_seq_cst); \
	restore_context(ctx); \

#define make(ctx, func, sp, stack_size) \
	std::atomic_thread_fence(std::memory_order_seq_cst); \
	make_context(ctx, func, sp, stack_size); \

static void test_func()
{
	int a = rand();
	printf("That's my func! [%p][%d]\n", &a, a);
}

context_t ctx;




static void __stdcall fiberMain(void* fiber)
{
	test_func();
}


void fiber_test_main()
{
	void* main_fiber = ::ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH);
	void* fiber_func = ::CreateFiber(16384, fiberMain, nullptr);

	pfn_function fn = test_func;
	fn();

	::SwitchToFiber(fiber_func);


}

int main()
{
#if 0
	fiber_test_main();
	printf("done\n");
	return 3;
#endif

	pfn_function fn = test_func;
	fn();

	void* stack = malloc(512);
	memset(stack, 0x33, 512);
	void* stack_top = reinterpret_cast<char*>(stack) + 512;

	context_t ctx_func;
	make(ctx_func, fn, stack_top, 512); // where do we need to jump when function `fn` finished?


	// how to avoid using volatile keyword?
	volatile int counter = 0;

	printf("Step1\n");

	save(ctx);

	printf("Step2 [%d]\n", counter);
	counter++;

	if (counter < 100)
	{
		restore(ctx); // this will "jump" to the next line after save() function
	}

	printf("Step3\n");

	restore(ctx_func); // when `fn` function finished we'll have a crash!!!! because there is no return address on stack and stack is diffrent!!!!

	return 0;
}

