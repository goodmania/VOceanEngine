#include "PreCompileHeader.h"
#include "Application.h"

#include "VOceanEngine/Events/AppEvent.h"
#include "VOceanEngine/Log.h"
#include "VOceanEngine/Input.h"

#include "VulkanCore/Device.h"


namespace voe {

#define BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)

	Application* Application::s_Instance = nullptr;

	Application::Application() : m_Running(true)
	{
		VOE_CORE_ASSERT(!s_Instance, "Application already exists!")
		s_Instance = this;

		m_Window = std::shared_ptr<Window>(Window::Create());
		m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));
		m_VulkanBase = std::make_unique<VulkanBase>(m_Window);
		LoadGameObjects();
	}

	Application::~Application()
	{

	}

	void Application::Run()
	{
		while (m_Running)
		{
			for (Layer* layer : m_LayerStack)
				layer->OnUpdate();

			if (auto commandBuffer = m_VulkanBase->BeginFrame())
			{
				m_VulkanBase->BeginSwapchainRenderPass(commandBuffer);
				m_VulkanBase->GetRenderer().RenderGameObjects(commandBuffer, m_GameObjects, m_Camera);
				m_VulkanBase->EndSwapchainRenderPass(commandBuffer);
				m_VulkanBase->EndFrame();
			}
			m_Window->OnUpdate();
		}
	}

	void Application::LoadGameObjects() 
	{
		auto device = m_VulkanBase->GetDevice();
		std::shared_ptr<Model> model = Model::CreateModelFromFile(*device, "Assets/Models/ocean-high-poly/Ocean.obj");
		auto flatVase = GameObject::CreateGameObject();
		flatVase.m_Model = model;
		flatVase.m_Transform.Translation = { -.5f, .5f, 2.5f };
		flatVase.m_Transform.Scale = { 3.f, 1.5f, 3.f };
		m_GameObjects.push_back(std::move(flatVase));

		/*model = Model::CreateModelFromFile(*device, "Assets/Models/ocean-high-poly/Ocean.obj");
		auto smoothVase = GameObject::CreateGameObject();
		smoothVase.m_Model = model;
		smoothVase.m_Transform.Translation = { .5f, .5f, 2.5f };
		smoothVase.m_Transform.Scale = { 3.f, 1.5f, 3.f };
		m_GameObjects.push_back(std::move(smoothVase));*/
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));

		VOE_CORE_TRACE("{ 0 }", e);

		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();)
		{
			(*--it)->OnEvent(e);
			if (e.Handled)
				break;
		}
	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
	}

	void Application::PushOverlay(Layer* layer)
	{
		m_LayerStack.PushOverlay(layer);
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}
}


