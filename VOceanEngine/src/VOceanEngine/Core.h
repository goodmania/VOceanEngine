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

#define BIT(x) (1 << x)