#pragma once

#include "VOceanEngine/Core.h"
#include "VOceanEngine/LayerStack.h"
#include "VOceanEngine/Events/Event.h"
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
		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		static Application& Get() { return *s_Instance; }
		Window& GetWindow() { return *m_Window; }

	private:
		bool OnWindowClose(WindowCloseEvent& e);

		std::unique_ptr<Window> m_Window;
		bool m_Running;
		LayerStack m_LayerStack;

		static Application* s_Instance;
	};

	Application* CreateApplication();
}

