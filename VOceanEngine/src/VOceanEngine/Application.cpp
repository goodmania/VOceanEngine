#include "Application.h"

#include "VOceanEngine/Events/AppEvent.h"
#include "VOceanEngine/Log.h"

namespace voe {

	Application::Application()
	{
	}

	Application::~Application()
	{
	}

	void Application::Run()
	{
		WindowResizeEvent e(1280, 720);
		if (e.IsInCategory(EventCategoryApplication))
		{
			VOE_TRACE(e);
		}
		if(e.IsInCategory(EventCategoryInput))
		{
			VOE_TRACE(e);
		}
		while (true);
	}
}


