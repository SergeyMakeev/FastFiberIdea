/*


API is super simple

Creates a new context/fiber (you don't need to call ConvertThreadToFiber or anything like that)
1. create(context_t* result, fiber_function, stack_top_ptr, stack_base_ptr);

Automatically saves the current execution context to *from* and switches to the *to*
2. switch_to(context_t* from, context_t* to);

Return the current context environment including a pointer to the current context and a pointer to the previous context to be able to switch back
3. context_env_t get_environment();


P.S. sizeof(context_t) = 104 bytes (note! it does not include 10 nonvolatile xmm registers XMM6:XMM15)

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
// https://www.wikiwand.com/en/Win32_Thread_Information_Block
// https://devblogs.microsoft.com/oldnewthing/20040114-00/?p=41053
// https://graphitemaster.github.io/fibers/
/*
Stack information stored in the TIB
A process should be free to move the stack of its threads as long as it updates the information stored in the TIB accordingly.
A few fields are key to this matter: stack base, stack limit, deallocation stack, and guaranteed stack bytes, respectively
stored at offsets 0x8, 0x10, 0x1478 and 0x1748 in 64 bits.Different Windows kernel functions read and write these values,
specially to distinguish stack overflows from other read / write page faults(a read or write to a page guarded among the stack limits
in guaranteed stack bytes will generate a stack - overflow exception instead of an access violation).

The deallocation stack is important because Windows API allows to change the amount of guarded pages : the function SetThreadStackGuarantee allows
both read the current space and to grow it.In order to read it, it reads the GuaranteedStackBytes field, and to grow it,
it uses has to uncommit stack pages.Setting stack limits without setting DeallocationStack will probably cause odd
behavior in SetThreadStackGuarantee.For example, it will overwrite the stack limits to wrong values.

Different libraries call SetThreadStackGuarantee, for example the.NET CLR uses it for setting up the stack of their threads.
*/
// 
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
		uintptr_t deallocation_stack;

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

// x64 MS ABI function parameter passing
// rax    - volatile (return value)
// rcx    - volatile (first integer arg)
// rdx    - volatile (second integer arg)
// r8     - volatile (third integer arg)
// r9     - volatile (fourth integer arg)

// todo: use line to generate unique names?
// todo: replace _ReadWriteBarrier() to _mm_mfence() / __sync_synchronize() ?
#define switch_to(from ,to) \
	do { \
		context_env_t env; \
		env._from = (from); \
		env._to = (to); \
		_ReadWriteBarrier(); \
		switch_context((from), (to), &env); \
	} while(0) \

// note: 0xDEADC0DE if we trying to return from fiber function there is no reasonable address to return!!!!
// todo: use line to generate unique names?
// note: sb-=4; because (according to x64 MS ABI) we have to reserve stack space for first four arguments (even if there no arguments!)
#define create(ctx, fiber_func, stack_top, stack_base) \
	do { \
		uintptr_t fiberReturnAddress = 0xDEADC0DE; \
		uintptr_t* sb = reinterpret_cast<uintptr_t*>(stack_base); \
		sb--; \
		*sb = fiberReturnAddress; \
		sb-=4; \
		create_context((ctx), (fiber_func), (sb)); \
		(ctx)->stack_base = (uintptr_t)(stack_base); \
		(ctx)->stack_limit = (uintptr_t)(stack_top); \
		(ctx)->deallocation_stack = (uintptr_t)(stack_top); \
	} while(0) \


#define ReadTeb(offset) __readgsqword(offset);
#define WriteTeb(offset, v) __writegsqword(offset, v)

inline context_env_t* get_environment()
{
	//note: teb offsets can be easily hardcoded!
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
	// --- allocate custom stack
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	int pageSize = (int)systemInfo.dwPageSize;

	// see _XCPT_GUARD_PAGE_VIOLATION and _XCPT_UNABLE_TO_GROW_STACK
	int stackSize = pageSize * 2; // min 2 pages because of stack probing! might need additional guard page to handle "stack overflow"
	//stackSize = 8192 bytes
	void* stack_top = VirtualAlloc(NULL, stackSize, MEM_COMMIT, PAGE_READWRITE);
	assert(stack_top);

	printf("custom stack [%p .. %p]\n", stack_top, reinterpret_cast<char*>(stack_top) + stackSize);
	void* stack_base = reinterpret_cast<char*>(stack_top) + stackSize;

	void* stackTop = (void*)ReadTeb(FIELD_OFFSET(NT_TIB, StackBase));
	void* stackBottom = (void*)ReadTeb(FIELD_OFFSET(NT_TIB, StackLimit));
	printf("main thread stack [%p .. %p]\n", stackTop, stackBottom);

	/*
	// stack guard-page
	DWORD oldProtect = 0;
	BOOL res = VirtualProtect(stack_top, pageSize, PAGE_NOACCESS, &oldProtect);
	assert(res);
	*/

	// -- create execution context
	context_t ctx_func;
	create(&ctx_func, test_func, stack_top, stack_base);

	test_func();

	printf("before\n");

	// -- switch execution context
	context_t global_ctx;
	switch_to(&global_ctx, &ctx_func);

	printf("after\n");

	return 0;
}




