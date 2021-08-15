#include "PreCompileHeader.h"
#include "Surface.h"

#include "VulkanCore/Tools.h"
#include "VulkanCore/Instance.h"

namespace voe
{
	Surface::Surface(const Instance* instance, const Window* window)
		: m_Instance(instance), m_Window(window)
	{
		auto glfwWindow = static_cast<GLFWwindow*>(window->GetNativeWindow());

		VOE_CHECK_RESULT(glfwCreateWindowSurface(
			instance->GetVkInstance(),
			glfwWindow,
			nullptr,
			&m_Surface));
	}

	Surface::~Surface()
	{
		vkDestroySurfaceKHR(m_Instance->GetVkInstance(), m_Surface, nullptr);
	}

	VkSurfaceFormatKHR Surface::ChooseSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
				availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return availableFormat;
			}
		}
		return availableFormats[0];
	}

}
