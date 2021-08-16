#include "PreCompileHeader.h"
#include "Window.h"

#ifdef VOE_PLATFORM_WINDOWS
#include "Platform/Windows/WindowsWindow.h"
#endif

namespace voe
{
	std::shared_ptr<Window> Window::Create(const WindowProps& props)
	{
#ifdef VOE_PLATFORM_WINDOWS
		return std::make_shared<WindowsWindow>(props);
#else
		return nullptr;
#endif
	}
}
