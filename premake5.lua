workspace "VulkanProject"
    architecture "x64"
    language "C++"
    cppdialect "C++17"
    startproject "VulkanProject"

    configurations
	{
		"Debug",
		"Release"
    }
    
    flags
	{
		"MultiProcessorCompile"
    }
    
    
	filter "configurations:Debug"
        defines "JAS_DEBUG"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines "JAS_RELEASE"
        runtime "Release"
        optimize "on"

OUTPUT_DIR = "%{cfg.buildcfg}-%{cfg.architecture}"

-- Static library
include "libs/glfw"
include "libs/imgui/imgui"

project "VulkanProject"
    location "VulkanProject"
    kind "ConsoleApp"

    pchheader "jaspch.h"
    pchsource "VulkanProject/src/jaspch.cpp"

    targetdir ("build/bin/" .. OUTPUT_DIR)
    objdir ("build/obj/" .. OUTPUT_DIR)

    files
    {
        "%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
    }

    links
    {
        "vulkan-1",
        "glfw",
        "imgui"
    }

    includedirs
    {
        "%{prj.name}/src",
        "libs/glfw/include",
        "libs/spdlog/include",
        "libs/glm",
        "libs/stb",
        "libs/glTF",
        "libs/imgui/imgui"
    }

    libdirs
    {
        "C:/VulkanSDK/1.1.130.0/Lib"
    }

    sysincludedirs
	{
		"C:/VulkanSDK/1.1.130.0/Include"
    }
    
    filter "system:windows"
        systemversion "latest"