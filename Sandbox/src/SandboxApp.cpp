#include <VOceanEngine.h>

class Sandbox : public voe::Application
{
public:
	Sandbox() {}
	~Sandbox() {}
};

voe::Application* voe::CreateApplication()
{
	return new Sandbox();
}