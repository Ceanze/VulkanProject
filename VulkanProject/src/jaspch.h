#pragma once

// Standard libraries
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <set>
#include <sstream>

// Vulkan
#include <vulkan/vulkan.h>

// Custom
#include "Core/Logger.h"

// Defines
#define ERROR_CHECK(F, E) if(F != VK_SUCCESS) { JAS_ERROR(E); throw std::runtime_error(E); }
#define ERROR_CHECK_REF(F, E, P) if(F != VK_SUCCESS) { std::stringstream ss; ss<<E<<" [Reference]: 0x"<<std::hex<<P; JAS_ERROR(ss.str()); throw std::runtime_error(ss.str()); }
