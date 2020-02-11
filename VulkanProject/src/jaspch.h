#pragma once

// Standard libraries
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <set>

// Vulkan
#include <vulkan/vulkan.h>

// Custom
#include "Core/Logger.h"

// Defines
#define ERROR_CHECK(F, E) if(F != VK_SUCCESS) { JAS_ERROR(E); throw std::runtime_error(E); }