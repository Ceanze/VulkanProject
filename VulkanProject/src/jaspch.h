#pragma once

// Standard libraries
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <set>
#include <sstream>
#include <chrono>

// Vulkan
#include <vulkan/vulkan.h>

// Externals
#include <glm/glm.hpp>

// Custom
#include "Core/Logger.h"

// Defines
#define ERROR_CHECK(F, E) if(F != VK_SUCCESS) { JAS_ERROR(E); throw std::runtime_error(E); }
#define ERROR_CHECK_REF(F, E, P) if(F != VK_SUCCESS) { std::stringstream ss; ss<<E<<" [Reference]: 0x"<<std::hex<<P; JAS_ERROR(ss.str()); throw std::runtime_error(ss.str()); }

#ifdef JAS_DEBUG
#define JAS_ASSERT(exp, ...) {if(!(exp)){JAS_ERROR(__VA_ARGS__);} assert(exp); }
#else
#define JAS_ASSERT(exp, ...) {}
#endif
