#pragma once

#include "VOceanEngine/Window.h"

namespace voe {

	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(const WindowProps& props);
		virtual ~WindowsWindow();

		void OnUpdate() override;

		uint32_t GetWidth() const override { return m_Data.Width; }
		uint32_t GetHeight() const override { return m_Data.Height; }

		// describe the size of a rectangular region of pixels within an image or framebuffer,
		// as (width,height) for two-dimensional images
		VkExtent2D GetExtent() const override { return { static_cast<uint32_t>(m_Data.Width), static_cast<uint32_t>(m_Data.Height) }; }

		virtual void* GetNativeWindow() const { return m_Window; }

		void CreateWindowSurface(VkInstance instance, VkSurfaceKHR* surface);

		bool WasWindowResized() override { return m_Data.FramebufferResized; }
		void ResetWindowResizedFlag() override { m_Data.FramebufferResized = false; }

		// Window attributes
		virtual void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
		virtual void SetVSync(bool enabled) override;
		virtual bool IsVSync() const override;

	private:
		virtual void Init(const WindowProps& props);
		virtual void Shutdown();

		struct WindowData
		{
			std::string Title;
			unsigned int Width, Height;
			bool VSync;
			EventCallbackFn EventCallback;
			bool FramebufferResized = false;
		};

		WindowData  m_Data;
		GLFWwindow* m_Window;
	};
}