#pragma once

#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include "Core/Logger.h"
#include <vector>
#include <set>

#include <vulkan/vulkan.h>

#define ERROR_CHECK(F, E) if(F != VK_SUCCESS) { JAS_ERROR(E); throw std::runtime_error(E); }