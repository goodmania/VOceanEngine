#pragma once
#include "VOceanEngine/core.h"
#include "Events/Event.h"

namespace voe {

	class VOE_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();
	};

	Application* CreateApplication();

}

