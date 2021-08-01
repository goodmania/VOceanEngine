#pragma once
#include "core.h"

namespace voe {

	class VOE_API Application
	{
	public:
		Application();
		virtual ~Application();

		void run();
	};

	Application* CreateApplication();

}

