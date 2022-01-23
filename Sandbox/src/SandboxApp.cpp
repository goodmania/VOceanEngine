#include <VOceanEngine.h>

class ExampleLayer : public voe::Layer
{
public:
	ExampleLayer() : Layer("Example")
	{

	}

	void OnUpdate() override
	{
		//VOE_INFO("ExampleLayer::Update");
	}

	void OnEvent(voe::Event& event) override
	{
		//VOE_TRACE("{0}", event);
	}
};

class Sandbox : public voe::Application
{
public:
	Sandbox() 
	{
		PushLayer(new ExampleLayer());
	}

	~Sandbox() 
	{

	}
};

voe::Application* voe::CreateApplication()
{
	return new Sandbox();
}