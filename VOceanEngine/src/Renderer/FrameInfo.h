#pragma once

#include "Renderer/Camera.h"

#include <vulkan/vulkan.h>

namespace voe {
	struct FrameInfo  {
		int FrameIndex;
		float FrameTime;
		VkCommandBuffer CommandBuffer;
		Camera& CameraObj;
	};



}