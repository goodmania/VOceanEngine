#pragma once

#include "Platform/Windows/WindowsWindow.h"

namespace voe {

	class Instance;

	class VOE_API Surface 
	{
	public:
		Surface(const Instance* instance, const Window* window);
		~Surface();

		const VkSurfaceKHR& GetVkSurface() const { return m_Surface; }
		VkSurfaceFormatKHR ChooseSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	private:
		const Instance* m_Instance;
		const Window* m_Window;

		VkSurfaceKHR m_Surface;
	};
}