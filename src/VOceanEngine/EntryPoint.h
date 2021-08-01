#pragma once

#ifdef VOE_PLATFORM_WINDOWS

extern voe::Application* voe::CreateApplication();

int main(int argc, char** argv)
{
	auto app = voe::CreateApplication();
	app->run();
	delete app;
}

#endif
