project "ImGui"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"

    targetdir ("bin/" .. OUTPUT_DIR .. "/%{prj.name}")
    objdir ("bin-int/" .. OUTPUT_DIR .. "/%{prj.name}")

    files
    {
        "imconfig.h",
        "imgui.h",
        "imgui.cpp",
        "imgui_draw.cpp",
        "imgui_internal.h",
        "imgui_widgets.cpp",
        "imstb_rectpack.h",
        "imstb_textedit.h",
        "imstb_truetype.h",
        "imgui_demo.cpp",
        "imgui_impl_glfw.h",
        "imgui_impl_glfw.cpp",
        "imgui_impl_vulkan.h",
        "imgui_impl_vulkan.cpp"
    }

    includedirs
    {
        "../../vulkan/include",
        "../../glfw/include"
    }
    
    filter "system:windows"
        systemversion "latest"
        staticruntime "On"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"