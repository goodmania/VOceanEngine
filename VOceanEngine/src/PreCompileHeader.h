#pragma once

#include <iostream>
#include <iomanip>

#include <memory>
#include <utility>
#include <algorithm>
#include <functional>
#include <random>

#include <string>
#include <sstream>
#include <fstream>

#include <array>
#include <vector>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <optional>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "VOceanEngine/Log.h"
#include "VulkanCore/Tools.h"
#include "Renderer/GameObject.h"

#ifdef VOE_PLATFORM_WINDOWS
#include <Windows.h>
#endif
