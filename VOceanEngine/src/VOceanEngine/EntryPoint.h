#pragma once

#ifdef VOE_PLATFORM_WINDOWS

extern voe::Application* voe::CreateApplication();

int main(int argc, char** argv)
{
	voe::Log::Init();
	VOE_CORE_WARN("Initialized Log");

	auto app = voe::CreateApplication();
	app->Run();
	delete app;
}

#endif
