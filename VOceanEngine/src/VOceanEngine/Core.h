#pragma once

#ifdef VOE_PLATFORM_WINDOWS
	#ifdef VOE_BUILD_DLL
		#define VOE_API __declspec(dllexport)
	#else
		#define VOE_API __declspec(dllimport)
	#endif
#else
	#error VOceanEngine only support windows
#endif

#ifdef VOE_ENABLE_ASSERTS
	#define VOE_ASSERT(x, ...) {if(!(x)) {VOE_ERROR("Assertion Failed {0}, __VA_ARGS__"); __debugbreak();}}
	#define VOE_CORE_ASSERT(x, ...) {if(!(x)) {VOE_CORE_ERROR("Assertion Failed {0}, __VA_ARGS__"); __debugbreak();}}
#else
	#define VOE_ASSERT(x, ...)
	#define VOE_CORE_ASSERT(x, ...)
#endif // VOE_ENABLE_ASSERTS


#define BIT(x) (1 << x)