#pragma once

#include "VOceanEngine/Core.h"
#include "Events/Event.h"
#include "VOceanEngine/Events/AppEvent.h"

#include "Window.h"

namespace voe {

	class VOE_API Application
	{
	public:
		Application();
		virtual ~Application();
		void Run();

		void OnEvent(Event& e);
	private:
		std::unique_ptr<Window> m_Window;
		bool m_Running;
		bool OnWindowClose(WindowCloseEvent& e);
	};

	Application* CreateApplication();

}

