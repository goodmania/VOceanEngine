#pragma once

#include "Platform/Windows/WindowsWindow.h"


namespace voe {

	class Instance;

	class VOE_API Surface 
	{
	public:
		Surface(const Instance* instance, const WindowsWindow* window);
		~Surface();

		const VkSurfaceKHR& GetVkSurface() const { return m_Surface; }

	private:
		const Instance* m_Instance;
		const WindowsWindow* m_Window;

		VkSurfaceKHR m_Surface;
	};
}