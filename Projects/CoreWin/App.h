#pragma once
#include "CoreWinMain.h"
#include <Common/DeviceResources.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.ApplicationModel.Activation.h>
#include <winrt/Windows.ApplicationModel.Core.h>
//#include <winrt/Windows.ApplicationModel.Preview.Holographic.h>
#include <winrt/Windows.UI.Core.h>
//#include <winrt/Windows.UI.Text.Core.h>
//#include <winrt/Windows.UI.ViewManagement.h>
//#include <winrt/Windows.Graphics.Holographic.h>

namespace windows {
	using namespace winrt::Windows::ApplicationModel;
	using namespace winrt::Windows::ApplicationModel::Activation;
	using namespace winrt::Windows::ApplicationModel::Core;
	using namespace winrt::Windows::UI::Core;
	using namespace winrt::Windows::Graphics::Display;
	using namespace winrt::Windows::Foundation;
	//using namespace winrt::Windows::UI::Text::Core;
	//using namespace winrt::Windows::UI::ViewManagement;
	//using namespace winrt::Windows::Graphics::Holographic;
	//using namespace winrt::Windows::ApplicationModel::Preview::Holographic;
} // namespace windows

namespace CoreWin
{
	// Main entry point for our app. Connects the app with the Windows shell and handles application lifecycle events.
	class App : winrt::implements<App, winrt::Windows::ApplicationModel::Core::IFrameworkView>{
	public:
		App();

		// IFrameworkView Methods.
		void Initialize(windows::CoreApplicationView const& applicationView);
		void SetWindow(windows::CoreWindow const& window);
		void Load(winrt::hstring const& entryPoint);
		void Run();
		void Uninitialize();

	protected:
		// Application lifecycle event handlers.
		void OnActivated(windows::CoreApplicationView const&, windows::IActivatedEventArgs const& args);
		void OnSuspending(windows::IInspectable const& sender, winrt::Windows::ApplicationModel::SuspendingEventArgs const& args);
		void OnResuming(windows::IInspectable const& sender, windows::IInspectable const& args);

		// Window event handlers.
		void OnWindowSizeChanged(windows::CoreWindow const& sender, windows::WindowSizeChangedEventArgs const& args);
		void OnVisibilityChanged(windows::CoreWindow const& sender, windows::VisibilityChangedEventArgs const& args);
		void OnWindowClosed(windows::CoreWindow const& sender, windows::CoreWindowEventArgs const& args);

		// DisplayInformation event handlers.
		void OnDpiChanged(windows::DisplayInformation const& sender, windows::IInspectable const& args);
		void OnOrientationChanged(windows::DisplayInformation const& sender, windows::IInspectable const& args);
		void OnDisplayContentsInvalidated(windows::DisplayInformation const& sender, windows::IInspectable const& args);
		
		//interaction
		void OnPointerPressed(windows::CoreWindow const& sender, windows::PointerEventArgs const& args);
		void OnPointerMoved(windows::CoreWindow const& sender, windows::PointerEventArgs const& args);
		void OnPointerReleased(windows::CoreWindow const& sender, windows::PointerEventArgs const& args);
	private:
		std::shared_ptr<DX::DeviceResources> m_deviceResources;
		std::unique_ptr<CoreWinMain> m_main;
		bool m_windowClosed;
		bool m_windowVisible;
	};
}
class Direct3DApplicationSource : winrt::implements<App, windows::IFrameworkViewSource> {
	windows::IFrameworkView CreateView() {
		return winrt::make<App>();
	}
};