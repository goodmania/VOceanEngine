#pragma once

#include "Platform/Windows/WindowsWindow.h"

namespace voe {

	class Instance;
	class PhDevice;
	class Surface;

	class VOE_API Device
	{
		Device(const Instance* instance, const PhDevice* phDevice, const Surface* surface);
	};

}



