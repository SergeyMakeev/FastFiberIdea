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

	// TODO: think about parent context!!!! how do I implement yield() function?

	typedef void (*pfn_function)();

	extern void create_context(context_t* ctx, pfn_function func, void* sp);
	extern void switch_context(context_t* from, context_t* to);

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


#define switch_to(from ,to) \
	_ReadWriteBarrier(); \
	switch_context((from), (to)); \


// note: __debugbreak() below - if we trying to return from "main fiber" function there is no return address!!!!
// todo: replace _ReadWriteBarrier() to _mm_mfence() / __sync_synchronize() ?
#define create(ctx, func, stack_top, stack_base) \
	auto fiberMainFunc = []() { \
		func(); \
		__debugbreak(); \
	}; \
	_ReadWriteBarrier(); \
	create_context((ctx), (fiberMainFunc), (stack_base)); \
	(ctx)->stack_base = (uintptr_t)(stack_base); \
	(ctx)->stack_limit = (uintptr_t)(stack_top); \


context_t global_ctx;
context_t ctx_func;



#define ReadTeb(offset) __readgsqword(offset);
#define WriteTeb(offset, v) __writegsqword(offset, v)


void offset_test()
{
	static const size_t stackBaseOffset = FIELD_OFFSET(NT_TIB, StackBase);
	static const size_t stackLimit = FIELD_OFFSET(NT_TIB, StackLimit);
	static const size_t stackUserPtr = FIELD_OFFSET(NT_TIB, ArbitraryUserPointer);

	static_assert(stackBaseOffset == 8, "Bad offset");
	static_assert(stackLimit == 16, "Bad offset");
	static_assert(stackUserPtr == 40, "Bad offset");
}





static void test_func()
{
	//TODO: get parent context from TLB!!!

	void* stackTop = (void*)ReadTeb(FIELD_OFFSET(NT_TIB, StackBase));
	void* stackBottom = (void*)ReadTeb(FIELD_OFFSET(NT_TIB, StackLimit));

	//int a = rand();
	int a = 3;
	printf("That's my func! [%p][%d]\n", &a, a);
	switch_to(&ctx_func, &global_ctx);
}




int main()
{

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	int pageSize = (int)systemInfo.dwPageSize;

	// see _XCPT_GUARD_PAGE_VIOLATION and _XCPT_UNABLE_TO_GROW_STACK
	int stackSize = pageSize * 2; // min 2 pages because of stack probing! might need additional guard page to handle "stack overflow"
	//stackSize = 8192 bytes
	void* stack_top = VirtualAlloc(NULL, stackSize, MEM_COMMIT, PAGE_READWRITE);
	assert(stack_top);

/*
	DWORD oldProtect = 0;
	BOOL res = VirtualProtect(stack_top, pageSize, PAGE_NOACCESS, &oldProtect);
	assert(res);
*/
	void* stack_base = reinterpret_cast<char*>(stack_top) + stackSize;

	//auto func = []() {exit(0); };

	// todo: default context to return into? that's probably wont work?
	create(&ctx_func, test_func, stack_top, stack_base);

	// TODO: check stack state here!
	printf("before\n");

	void* stackTop = (void*)ReadTeb(FIELD_OFFSET(NT_TIB, StackBase));
	void* stackBottom = (void*)ReadTeb(FIELD_OFFSET(NT_TIB, StackLimit));


	switch_to(&global_ctx, &ctx_func);

	// TODO: check stack state here! (should be the same as above)
	printf("after\n");

	return 0;
}

