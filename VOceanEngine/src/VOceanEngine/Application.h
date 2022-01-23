#pragma once

#include "VOceanEngine/Core.h"
#include "VOceanEngine/LayerStack.h"
#include "VOceanEngine/Events/Event.h"
#include "VOceanEngine/Events/AppEvent.h"

#include "Window.h"
#include "Renderer/VulkanBase.h"
#include "Renderer/Camera.h"

namespace voe {

	class Camera;

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
		static Application* s_Instance;
		void LoadGameObjects();
		void CreateOceanFFTObjects();

		Camera m_Camera {};
		std::shared_ptr<Window> m_Window;
		bool m_Running;
		LayerStack m_LayerStack;

		std::unique_ptr<VulkanBase> m_VulkanBase;
		std::vector<GameObject> m_GameObjects;

#ifdef VOE_DEBUG
		const bool m_EnableImgui = true;
#else
		const bool m_EnableImgui = true;
#endif
	};

	Application* CreateApplication();
}

