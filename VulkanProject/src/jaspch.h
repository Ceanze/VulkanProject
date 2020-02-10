#pragma once

#include <string>
#include <stdexcept>
#include "Core/Logger.h"
#include <vector>

#include <vulkan/vulkan.h>

#define ERROR_CHECK(F, E) if(F != VK_SUCCESS) { JAS_ERROR(E); throw std::runtime_error(E); }