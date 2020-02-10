#pragma once

#include <string>
#include <stdexcept>
#include "Core/Logger.h"

#define ERROR_CHECK(F, E) if(F != VK_SUCCESS) { JASE_ERROR(E); throw std::runtime_error(E); }