#include "pch.h"
//#include "App.h"

#include <ppltasks.h>

#include "CoreWinMain.h"
#include <Common/DeviceResources.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Numerics.h>

#include <winrt/Windows.ApplicationModel.Activation.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.UI.Input.h>
#include <winrt/Windows.UI.Composition.h>

namespace windows {
	using namespace winrt::Windows::ApplicationModel;
	using namespace winrt::Windows::ApplicationModel::Activation;
	using namespace winrt::Windows::ApplicationModel::Core;
	using namespace winrt::Windows::UI;
	using namespace winrt::Windows::UI::Core;
	using namespace winrt::Windows::UI::Composition;
	using namespace winrt::Windows::Graphics::Display;
	using namespace winrt::Windows::Foundation;
	using namespace winrt::Windows::Foundation::Numerics;
} // namespace windows

namespace CoreWin
{
	// Main entry point for our app. Connects the app with the Windows shell and handles application lifecycle events.
	struct App : winrt::implements<App, windows::IFrameworkViewSource, windows::IFrameworkView> {
	//public:
		App();
		windows::IFrameworkView CreateView()
		{
			return *this;
		}
		// IFrameworkView Methods.
		void Initialize(windows::CoreApplicationView const& applicationView);
		void SetWindow(windows::CoreWindow const& window);
		void Load(winrt::hstring const& entryPoint);
		void Run();
		void Uninitialize();

	//protected:
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
	//private:
		std::shared_ptr<DX::DeviceResources> m_deviceResources;
		std::unique_ptr<CoreWinMain> m_main;
		bool m_windowClosed;
		bool m_windowVisible;

		//tmp button solution
		windows::CompositionTarget m_target{ nullptr };
		windows::Visual m_selected{ nullptr };
		windows::VisualCollection m_visuals{ nullptr };
		windows::float2 m_offset{}; windows::float2 m_size{};
		void AddVisual(windows::float2 const point);

	};

	App::App() :
		m_windowClosed(false),
		m_windowVisible(true)
	{
	}

	void App::Initialize(windows::CoreApplicationView const& applicationView) {
		applicationView.Activated({ this, &App::OnActivated });
		windows::CoreApplication::Suspending({ this, &App::OnSuspending });
		windows::CoreApplication::Resuming({ this, &App::OnResuming });

		m_deviceResources = std::make_shared<DX::DeviceResources>();
	}

	// Called when the CoreWindow object is created (or re-created).
	void App::SetWindow(winrt::Windows::UI::Core::CoreWindow const& window) {
		windows::Compositor compositor;
		windows::ContainerVisual root = compositor.CreateContainerVisual();
		//m_target = compositor.CreateTargetForCurrentView();
		//m_target.Root(root);
		m_visuals = root.Children();
		AddVisual(windows::float2(100.0f, 100.0f));

		window.SizeChanged({ this, &App::OnWindowSizeChanged });

		window.Closed({ this, &App::OnWindowClosed });

		window.VisibilityChanged({ this, &App::OnVisibilityChanged });

		windows::DisplayInformation currentDisplayInformation{ windows::DisplayInformation::GetForCurrentView() };

		currentDisplayInformation.DpiChanged({ this, &App::OnDpiChanged });

		currentDisplayInformation.OrientationChanged({ this, &App::OnOrientationChanged });

		windows::DisplayInformation::DisplayContentsInvalidated({ this, &App::OnDisplayContentsInvalidated });

		//window.PointerPressed(std::bind(&App::OnPointerPressed, this, std::placeholders::_1, std::placeholders::_2));
		//window.PointerMoved(std::bind(&App::OnPointerMoved, this, std::placeholders::_1, std::placeholders::_2));
		//window.PointerReleased(std::bind(&App::OnPointerReleased, this, std::placeholders::_1, std::placeholders::_2));
		
		window.PointerPressed({ this, &App::OnPointerPressed });
		window.PointerReleased({ this, &App::OnPointerReleased });
		window.PointerMoved({ this, &App::OnPointerMoved });

		m_deviceResources->SetWindow(window);
	}

	// Initializes scene resources, or loads a previously saved app state.
	void App::Load(winrt::hstring const& entryPoint) {
		if (m_main == nullptr)
		{
			m_main = std::unique_ptr<CoreWinMain>(new CoreWinMain(m_deviceResources));
		}
	}

	// This method is called after the window becomes active.
	void App::Run() {
		while (!m_windowClosed)
		{
			windows::CoreWindow window = windows::CoreWindow::GetForCurrentThread();

			if (m_windowVisible)
			{

				window.Dispatcher().ProcessEvents(windows::CoreProcessEventsOption::ProcessAllIfPresent);

				m_main->Update();

				if (m_main->Render())
				{
					m_deviceResources->Present();
				}
			}
			else			{
				window.Dispatcher().ProcessEvents(windows::CoreProcessEventsOption::ProcessOneAndAllPending);
			}
		}
	}

	// Required for IFrameworkView.
	// Terminate events do not cause Uninitialize to be called. It will be called if your IFrameworkView
	// class is torn down while the app is in the foreground.
	void App::Uninitialize()
	{
	}

	// Application lifecycle event handlers.

	void App::OnActivated(windows::CoreApplicationView const&, windows::IActivatedEventArgs const& args) {
		windows::CoreWindow::GetForCurrentThread().Activate();
	}
	void App::OnSuspending(windows::IInspectable const& sender, winrt::Windows::ApplicationModel::SuspendingEventArgs const& args) {
		windows::SuspendingDeferral deferral = args.SuspendingOperation().GetDeferral();

		concurrency::create_task([this, deferral]
		{
			m_deviceResources->Trim();
			deferral.Complete();
		});
	}
	void App::OnResuming(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::Foundation::IInspectable const& args) {
	}

	// Window event handlers.

	void App::OnWindowSizeChanged(windows::CoreWindow const& sender, windows::WindowSizeChangedEventArgs const& args) {
		m_deviceResources->setOutputSize(args.Size().Width, args.Size().Height);
		m_main->CreateWindowSizeDependentResources();
	}

	void App::OnVisibilityChanged(windows::CoreWindow const& sender, windows::VisibilityChangedEventArgs const& args) {
		m_windowVisible = args.Visible();
	}

	void App::OnWindowClosed(windows::CoreWindow const& sender, windows::CoreWindowEventArgs const& args) {
		m_windowClosed = true;
	}

	// DisplayInformation event handlers.

	void App::OnDpiChanged(windows::DisplayInformation const& sender, windows::IInspectable const& args) {
		m_deviceResources->SetDpi(sender.LogicalDpi());
		m_main->CreateWindowSizeDependentResources();
	}

	void App::OnOrientationChanged(windows::DisplayInformation const& sender, windows::IInspectable const& args)
	{
		m_deviceResources->SetCurrentOrientation(sender.CurrentOrientation());
		m_main->CreateWindowSizeDependentResources();
	}

	void App::OnDisplayContentsInvalidated(windows::DisplayInformation const& sender, windows::IInspectable const& args) {
		m_deviceResources->ValidateDevice();
	}

	void App::OnPointerPressed(windows::CoreWindow const& sender, windows::PointerEventArgs const& args) {
		windows::float2 const point = args.CurrentPoint().Position();
		if (m_main != nullptr) m_main->OnPointerPressed(point.x, point.y);

		//check button
		if (point.x >= m_offset.x &&
			point.x < m_offset.x + m_size.x &&
			point.y >= m_offset.y &&
			point.y < m_offset.y + m_size.y) {
			OutputDebugString(L"============clicked===========");
			if (m_main != nullptr)m_main->onMainButtonClicked();
		}
	}
	void App::OnPointerMoved(windows::CoreWindow const& sender, windows::PointerEventArgs const& args) {
		if (m_main != nullptr) m_main->OnPointerMoved(args.CurrentPoint().Position().X, args.CurrentPoint().Position().Y);
	}
	void App::OnPointerReleased(windows::CoreWindow const& sender, windows::PointerEventArgs const& args) {
		if (m_main != nullptr) m_main->OnPointerReleased();
	}
	void App::AddVisual(windows::float2 const point) {
		windows::Compositor compositor = m_visuals.Compositor();
		windows::SpriteVisual visual = compositor.CreateSpriteVisual();

		static windows::Color colors[] =
		{
			{ 0xDC, 0x5B, 0x9B, 0xD5 },
			{ 0xDC, 0xED, 0x7D, 0x31 },
			{ 0xDC, 0x70, 0xAD, 0x47 },
			{ 0xDC, 0xFF, 0xC0, 0x00 }
		};

		static unsigned last = 0;
		unsigned const next = ++last % _countof(colors);
		visual.Brush(compositor.CreateColorBrush(colors[next]));

		float const BlockSize = 100.0f;

		visual.Size(
			{
				BlockSize,
				BlockSize
			});

		visual.Offset(
			{
				point.x - BlockSize / 2.0f,
				point.y - BlockSize / 2.0f,
				0.0f,
			});

		m_visuals.InsertAtTop(visual);

		m_selected = visual;
		m_offset.x = -BlockSize / 2.0f;
		m_offset.y = -BlockSize / 2.0f;
		m_size = visual.Size();
	}
}

[Platform::MTAThread]
int APIENTRY wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int) {
	windows::CoreApplication::Run(winrt::make<CoreWin::App>());
}
