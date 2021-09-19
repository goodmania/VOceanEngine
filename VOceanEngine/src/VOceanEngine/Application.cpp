#include "PreCompileHeader.h"
#include "Application.h"

#include "VOceanEngine/Events/AppEvent.h"
#include "VOceanEngine/Log.h"
#include "VOceanEngine/Input.h"

#include "VulkanCore/Device.h"
#include "Renderer/GameObject.h"
#include "Renderer/CameraController.h"

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
		auto viewerObject = GameObject::CreateGameObject();
		auto currentTime = std::chrono::high_resolution_clock::now();
		CameraController cameraController;

		while (m_Running)
		{
			auto newTime = std::chrono::high_resolution_clock::now();
			auto frameTime 
				= std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
			if (frameTime > 0.25f)
				frameTime = 0.25f;

			currentTime = newTime;
			cameraController.OnUpdate(frameTime, viewerObject);

			// m_Camera OnUpdate 
			float aspect = m_VulkanBase->GetAspectRatio();
			m_Camera.SetViewYXZ(viewerObject.m_Transform.Translation, viewerObject.m_Transform.Rotation);
			m_Camera.SetFrustumProjectionMatrix(glm::radians(50.f), aspect, 0.1f, 20.f);

			if (auto commandBuffer = m_VulkanBase->BeginFrame())
			{
				m_VulkanBase->BeginSwapchainRenderPass(commandBuffer);
				m_VulkanBase->GetRenderer().BuildComputeCommandBuffer(commandBuffer);
				m_VulkanBase->GetRenderer().RenderGameObjects(commandBuffer, m_GameObjects, m_Camera);
				m_VulkanBase->EndSwapchainRenderPass(commandBuffer);
				m_VulkanBase->EndFrame();
			}

			for (Layer* layer : m_LayerStack)
				layer->OnUpdate();

			m_VulkanBase->GetRenderer().OnUpdate(frameTime);
			m_Window->OnUpdate();
		}
	}

	void Application::LoadGameObjects() 
	{
		auto device = m_VulkanBase->GetDevice();
		std::shared_ptr<Model> model = Model::CreateModelFromFile(*device, "Assets/Models/plane_5_256_256.obj");
		//std::shared_ptr<Model> model = Model::CreateModelFromFile(*device, "Assets/Models/flat_vase.obj");
		auto ocean = GameObject::CreateGameObject();
		ocean.m_Model = model;
		ocean.m_Transform.Translation = { 0.f, 2.f, 0.f };
		ocean.m_Transform.Scale = { 1.0f, 1.0f, 1.0f };
		m_GameObjects.push_back(std::move(ocean));
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


