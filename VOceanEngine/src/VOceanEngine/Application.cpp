#include "PreCompileHeader.h"
#include "Application.h"

#include "VOceanEngine/Events/AppEvent.h"
#include "VOceanEngine/Log.h"
#include "VOceanEngine/Input.h"

#include "VulkanCore/Device.h"
#include "Renderer/GameObject.h"
#include "Renderer/CameraController.h"
#include "Renderer/Buffer.h"
#include "Renderer/FrameInfo.h"

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
		//LoadGameObjects();
		CreateOceanFFTObjects();
	}

	Application::~Application()
	{

	}

	void Application::Run()
	{
		auto currentTime = std::chrono::high_resolution_clock::now();

		auto viewerObject = GameObject::CreateGameObject();
		viewerObject.m_Transform.Translation.z = -2.5f;
		CameraController cameraController{};

		while (m_Running)
		{
			auto newTime = std::chrono::high_resolution_clock::now();
			auto frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();

			if (frameTime > 0.25f)
			{
				frameTime = 0.25f;
			}
				
			currentTime = newTime;
			cameraController.OnUpdate(frameTime, viewerObject);

			// m_Camera OnUpdate 
			float aspect = m_VulkanBase->GetAspectRatio();
			m_Camera.SetViewYXZ(viewerObject.m_Transform.Translation, viewerObject.m_Transform.Rotation);
			m_Camera.SetPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 1000.f);
			m_Camera.SetCameraPos(viewerObject.m_Transform.Translation);

			if (auto commandBuffer = m_VulkanBase->BeginFrame())
			{
				int frameIndex = m_VulkanBase->GetFrameIndex();
				FrameInfo frameInfo{ frameIndex, frameTime, commandBuffer, m_Camera };

				m_VulkanBase->GetRenderer().OnUpdate(frameTime, frameInfo);

				// render
				m_VulkanBase->BeginSwapchainRenderPass(commandBuffer);
				m_VulkanBase->GetRenderer().RenderGameObjects(frameInfo, m_GameObjects);
				m_VulkanBase->EndSwapchainRenderPass(commandBuffer);
				m_VulkanBase->EndFrame();
			}

			for (Layer* layer : m_LayerStack)
				layer->OnUpdate();

			m_Window->OnUpdate();
		}
	}

	void Application::LoadGameObjects() 
	{
		auto device = m_VulkanBase->GetDevice();
		std::shared_ptr<Model> model = Model::CreateModelFromFile(*device, "Assets/Models/304.obj");
		//std::shared_ptr<Model> model = Model::CreateModelFromFile(*device, "Assets/Models/flat_vase.obj");
		auto ocean = GameObject::CreateGameObject();
		ocean.m_Model = model;
		ocean.m_Transform.Translation = { 0.f, 2.f, 0.f };
		ocean.m_Transform.Scale = { 1.0f, 1.0f, 1.0f };
		m_GameObjects.push_back(std::move(ocean));
	}

	void Application::CreateOceanFFTObjects()
	{
		auto device = m_VulkanBase->GetDevice();

		uint32_t width = m_VulkanBase->GetRenderer().GetGridSize();
		uint32_t height = m_VulkanBase->GetRenderer().GetGridSize();
		uint32_t oceanSize = m_VulkanBase->GetRenderer().GetOceanSize();

		std::shared_ptr<Model> model = Model::CreateXZPlaneModelFromProcedural(*device, width, height, oceanSize);
		auto ocean = GameObject::CreateGameObject();
		ocean.m_Model = model;
		ocean.m_Transform.Translation = { 0.f, 300.f, 0.f };
		ocean.m_Transform.Scale = { 1.0f, 1.0f, 1.0f } ;
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