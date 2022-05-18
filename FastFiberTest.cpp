/*


API is super simple

Creates a new context/fiber (you don't need to call ConvertThreadToFiber or anything like that)
1. create(context_t* result, fiber_function, stack_top_ptr, stack_base_ptr);

Automatically saves the current execution context to *from* and switches to the *to*
2. switch_to(context_t* from, context_t* to);

Return the current context environment including a pointer to the current context and a pointer to the previous context to be able to switch back
3. context_env_t get_environment();


P.S. sizeof(context_t) = 96 (note! it does not include 10 nonvolatile xmm registers XMM6:XMM15)

*/

#include <windows.h>
#include <stdio.h>
#include <atomic>
#include <assert.h>

//
//
// https://github.com/SergeyMakeev/deboost.context/tree/master/asm
// https://docs.microsoft.com/en-us/cpp/build/x64-software-conventions?view=msvc-170
// https://docs.microsoft.com/en-us/cpp/build/x64-calling-convention?view=msvc-170
// https://docs.microsoft.com/en-us/cpp/build/stack-usage?view=msvc-170
// https://devblogs.microsoft.com/oldnewthing/20040114-00/?p=41053
// https://en.wikipedia.org/wiki/Win32_Thread_Information_Block
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
		// nonvolatile registers to save
		uintptr_t r12;
		uintptr_t r13;
		uintptr_t r14;
		uintptr_t r15;
		uintptr_t rdi;
		uintptr_t rsi;
		uintptr_t rbx;
		uintptr_t rbp;

		// stack
		uintptr_t rsp;

		// address
		uintptr_t rip;

		uintptr_t stack_base;
		uintptr_t stack_limit;

		//xmm6 - xmm15 todo? (these are nonvolatile too)
	};

	// context environment
	struct context_env_t
	{
		context_t* _from;
		context_t* _to;
	};


	typedef void (*pfn_function)();

	extern void create_context(context_t* ctx, pfn_function func, void* sp);
	extern void switch_context(context_t* from, context_t* to, context_env_t* env);

#ifdef __cplusplus
}
#endif

// parameter passing
// rax    - volatile (return value)
// rcx    - volatile (first integer arg)
// rdx    - volatile (second integer arg)
// r8     - volatile (third integer arg)
// r9     - volatile (fourth integer arg)
// r10:11 - volatile

// todo: use line to generate unique names
#define switch_to(from ,to) \
	do { \
		context_env_t env; \
		env._from = (from); \
		env._to = (to); \
		_ReadWriteBarrier(); \
		switch_context((from), (to), &env); \
	} while(0) \

// note: __debugbreak() below - if we trying to return from "main fiber" function there is no return address!!!!
// todo: replace _ReadWriteBarrier() to _mm_mfence() / __sync_synchronize() ?
// todo: use line to generate unique names
#define create(ctx, func, stack_top, stack_base) \
	do { \
		auto fiberMainFunc = []() { \
			func(); \
			__debugbreak(); \
		}; \
		_ReadWriteBarrier(); \
		create_context((ctx), (fiberMainFunc), (stack_base)); \
		(ctx)->stack_base = (uintptr_t)(stack_base); \
		(ctx)->stack_limit = (uintptr_t)(stack_top); \
	} while(0) \


#define ReadTeb(offset) __readgsqword(offset);
#define WriteTeb(offset, v) __writegsqword(offset, v)


inline context_env_t* get_environment()
{
	context_env_t* e = (context_env_t*)ReadTeb(FIELD_OFFSET(NT_TIB, ArbitraryUserPointer));
	return e;
}

void offset_test()
{
	static const size_t stackBaseOffset = FIELD_OFFSET(NT_TIB, StackBase);
	static const size_t stackLimit = FIELD_OFFSET(NT_TIB, StackLimit);
	static const size_t stackUserPtr = FIELD_OFFSET(NT_TIB, ArbitraryUserPointer);

	static_assert(stackBaseOffset == 8, "Bad offset");
	static_assert(stackLimit == 16, "Bad offset");
	static_assert(stackUserPtr == 40, "Bad offset");
}


// this function will be called from two contexts (global_ctx and ctx_func)
static void test_func()
{
	int a = rand();
	printf("That's my func! [stack:%p][%d]\n", &a, a);

	// get current context environment
	context_env_t* e = get_environment();

	if (e)
	{
		//switch back to calling fiber
		switch_to(e->_to, e->_from);
	}
}


int main()
{
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	int pageSize = (int)systemInfo.dwPageSize;

	// see _XCPT_GUARD_PAGE_VIOLATION and _XCPT_UNABLE_TO_GROW_STACK
	int stackSize = pageSize * 4; // min 2 pages because of stack probing! might need additional guard page to handle "stack overflow"
	//stackSize = 8192 bytes
	void* stack_top = VirtualAlloc(NULL, stackSize, MEM_COMMIT, PAGE_READWRITE);
	assert(stack_top);

	printf("custom stack [%p .. %p]\n", stack_top, reinterpret_cast<char*>(stack_top) + stackSize);


	DWORD oldProtect = 0;
	BOOL res = VirtualProtect(stack_top, pageSize, PAGE_NOACCESS, &oldProtect);
	assert(res);
	void* stack_base = reinterpret_cast<char*>(stack_top) + stackSize - pageSize;

	res = VirtualProtect(stack_base, pageSize, PAGE_NOACCESS, &oldProtect);
	assert(res);


	context_t ctx_func;
	create(&ctx_func, test_func, stack_top, stack_base);


	test_func();

	printf("before\n");

	void* stackTop = (void*)ReadTeb(FIELD_OFFSET(NT_TIB, StackBase));
	void* stackBottom = (void*)ReadTeb(FIELD_OFFSET(NT_TIB, StackLimit));
	printf("thread stack [%p .. %p]\n", stackTop, stackBottom);


	context_t global_ctx;
	switch_to(&global_ctx, &ctx_func);

	printf("after\n");

	return 0;
}




