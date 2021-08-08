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
}
