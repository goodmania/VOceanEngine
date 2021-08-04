#pragma once

#include "VOceanEngine/Layer.h"

namespace voe {

	class ImguiLayer : public Layer
	{

		ImguiLayer();
		~ImguiLayer();

		void OnAttach();
		void OnDetach();
		void OnUpdate();
		void OnEvent(Event& event);
	};
}


