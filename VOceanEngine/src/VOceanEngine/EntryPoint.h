#pragma once

#ifdef VOE_PLATFORM_WINDOWS

extern voe::Application* voe::CreateApplication();

int main(int argc, char** argv)
{
	voe::Log::Init();
	VOE_CORE_WARN("Initialized Log");
	int a = 6;
	VOE_INFO("Hello Var = {0}", a);

	auto app = voe::CreateApplication();
	app->Run();
	delete app;
}

#endif
