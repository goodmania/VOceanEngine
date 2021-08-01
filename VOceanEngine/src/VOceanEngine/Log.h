#pragma once

#include <memory>

#include "Core.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

namespace voe{

	class VOE_API Log
	{
	public:

		static void Init();

		static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

	private:

		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
}

#define VOE_CORE_ERROR(...) ::voe::Log::GetCoreLogger()->error(__VA_ARGS__)
#define VOE_CORE_WARN(...)  ::voe::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define VOE_CORE_INFO(...)  ::voe::Log::GetCoreLogger()->info(__VA_ARGS__)
#define VOE_CORE_TRACE(...) ::voe::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define VOE_CORE_FATAL(...) ::voe::Log::GetCoreLogger()->fatal(__VA_ARGS__)

#define VOE_ERROR(...)		::voe::Log::GetClientLogger()->error(__VA_ARGS__)
#define VOE_WARN(...)		::voe::Log::GetClientLogger()->warn(__VA_ARGS__)
#define VOE_INFO(...)		::voe::Log::GetClientLogger()->info(__VA_ARGS__)
#define VOE_TRACE(...)		::voe::Log::GetClientLogger()->trace(__VA_ARGS__)
#define VOE_FATAL(...)		::voe::Log::GetClientLogger()->fatal(__VA_ARGS__)

