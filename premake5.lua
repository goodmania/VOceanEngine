workspace "VOceanEngine"
	architecture "x64"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "VOceanEngine"
	location "VOceanEngine"
	kind "SharedLib"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vender/spdlog/include"
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

		defines
		{
			"VOE_PLATFORM_WINDOWS",
			"VOE_BUILD_DLL",
		}

		postbuildcommands
		{
			("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Sandbox")
		}

	filter "configurations:Debug"
		defines "VOE_DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "VOE_RELEASE"
		optimize "On"

	filter "configurations:Dist"
		defines "VOE_DIST"
		optimize "On"

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
	}

	includedirs
	{
		"VOceanEngine/vender/spdlog/include",
		"VOceanEngine/src"
	}

	links
	{
		"VOceanEngine"
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

		defines
		{
			"VOE_PLATFORM_WINDOWS",
		}

	filter "configurations:Debug"
		defines "VOE_DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "VOE_RELEASE"
		optimize "On"

	filter "configurations:Dist"
		defines "VOE_DIST"
		optimize "On"
