#pragma once
#include <stack>

#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.Graphics.Display.h>

namespace DX
{
	// Provides an interface for an application that owns DeviceResources to be notified of the device being lost or created.
	interface IDeviceNotify
	{
		virtual void OnDeviceLost() = 0;
		virtual void OnDeviceRestored() = 0;
	};

	// Controls all the DirectX device resources.
	class DeviceResources
	{
	public:
		DeviceResources();
		DeviceResources(bool noprapere_tag);
		void SetWindow(winrt::Windows::UI::Core::CoreWindow const& window);
		void SetLogicalSize(winrt::Windows::Foundation::Size logicalSize);
		void SetCurrentOrientation(winrt::Windows::Graphics::Display::DisplayOrientations currentOrientation);
		void SetDpi(float dpi);
		void ValidateDevice();
		void HandleDeviceLost();
		void RegisterDeviceNotify(IDeviceNotify* deviceNotify);
		void Trim();
		void Present();
		void SetBackBufferRenderTarget();
		void ClearCurrentDepthBuffer();

		void saveCurrentTargetViews(ID3D11RenderTargetView* render_target, ID3D11DepthStencilView* depth_target);
		void removeCurrentTargetViews();
		//ID3D11DepthStencilView*		GetDepthStencilView() const {
		//	if (m_depthStencilViewStack.empty())	return m_d3dDepthStencilView.Get();
		//	return m_depthStencilViewStack.top();
		//}
		ID3D11DepthStencilView* GetDepthStencilView();
		void SetDepthStencilView(ID3D11DepthStencilView* input) {
			current_depth_view = input;
		}
		// The size of the render target, in pixels.
		winrt::Windows::Foundation::Size	GetOutputSize() const					{ return m_outputSize; }

		// The size of the render target, in dips.
		winrt::Windows::Foundation::Size	GetLogicalSize() const					{ return m_logicalSize; }
		float						GetDpi() const							{ return m_effectiveDpi; }

		// D3D Accessors.
		ID3D11Device3*				GetD3DDevice() const					{ return m_d3dDevice.get(); }
		ID3D11DeviceContext3*		GetD3DDeviceContext() const				{ return m_d3dContext.get(); }
		IDXGISwapChain3*			GetSwapChain() const					{ return m_swapChain.get(); }
		D3D_FEATURE_LEVEL			GetDeviceFeatureLevel() const			{ return m_d3dFeatureLevel; }
		ID3D11RenderTargetView1*	GetBackBufferRenderTargetView() const	{ return m_d3dRenderTargetView.get(); }
		//ID3D11DepthStencilView*		GetDepthStencilView() const				{ return m_d3dDepthStencilView.Get(); }
		D3D11_VIEWPORT				GetScreenViewport() const				{ return m_screenViewport; }
		DirectX::XMFLOAT4X4			GetOrientationTransform3D() const		{ return m_orientationTransform3D; }

		// D2D Accessors.
		ID2D1Factory3*				GetD2DFactory() const					{ return m_d2dFactory.get(); }
		ID2D1Device2*				GetD2DDevice() const					{ return m_d2dDevice.get(); }
		ID2D1DeviceContext2*		GetD2DDeviceContext() const				{ return m_d2dContext.get(); }
		ID2D1Bitmap1*				GetD2DTargetBitmap() const				{ return m_d2dTargetBitmap.get(); }
		IDWriteFactory3*			GetDWriteFactory() const				{ return m_dwriteFactory.get(); }
		IWICImagingFactory2*		GetWicImagingFactory() const			{ return m_wicFactory.get(); }
		D2D1::Matrix3x2F			GetOrientationTransform2D() const		{ return m_orientationTransform2D; }

	protected:
		void CreateDeviceIndependentResources();
		void CreateDeviceResources();
		void CreateWindowSizeDependentResources();
		void UpdateRenderTargetSize();
		DXGI_MODE_ROTATION ComputeDisplayRotation();

		// Direct3D objects.
		winrt::com_ptr<ID3D11Device3>			m_d3dDevice { nullptr };
		winrt::com_ptr<ID3D11DeviceContext3>	m_d3dContext{ nullptr };
		winrt::com_ptr<IDXGISwapChain3>			m_swapChain{ nullptr };

		// Direct3D rendering objects. Required for 3D.
		winrt::com_ptr<ID3D11RenderTargetView1>	m_d3dRenderTargetView{ nullptr };
		winrt::com_ptr<ID3D11DepthStencilView>	m_d3dDepthStencilView{ nullptr };
		D3D11_VIEWPORT									m_screenViewport;

		// Direct2D drawing components.
		winrt::com_ptr<ID2D1Factory3>		m_d2dFactory{ nullptr };
		winrt::com_ptr<ID2D1Device2>		m_d2dDevice{ nullptr };
		winrt::com_ptr<ID2D1DeviceContext2>	m_d2dContext{ nullptr };
		winrt::com_ptr<ID2D1Bitmap1>		m_d2dTargetBitmap{ nullptr };

		// DirectWrite drawing components.
		winrt::com_ptr<IDWriteFactory3>		m_dwriteFactory{ nullptr };
		winrt::com_ptr<IWICImagingFactory2>	m_wicFactory{ nullptr };

		// Cached reference to the Window.
		winrt::agile_ref<winrt::Windows::UI::Core::CoreWindow> m_window;

		// Cached device properties.
		D3D_FEATURE_LEVEL								m_d3dFeatureLevel;
		winrt::Windows::Foundation::Size						m_d3dRenderTargetSize;
		winrt::Windows::Foundation::Size						m_outputSize;
		winrt::Windows::Foundation::Size						m_logicalSize;
		winrt::Windows::Graphics::Display::DisplayOrientations	m_nativeOrientation;
		winrt::Windows::Graphics::Display::DisplayOrientations	m_currentOrientation;
		float											m_dpi;

		// This is the DPI that will be reported back to the app. It takes into account whether the app supports high resolution screens or not.
		float m_effectiveDpi;

		// Transforms used for display orientation.
		D2D1::Matrix3x2F	m_orientationTransform2D;
		DirectX::XMFLOAT4X4	m_orientationTransform3D;

		// The IDeviceNotify can be held directly as it owns the DeviceResources.
		IDeviceNotify* m_deviceNotify;

		std::stack<ID3D11RenderTargetView*> m_renderTargetViewStack;
		std::stack<ID3D11DepthStencilView*> m_depthStencilViewStack;
		
		ID3D11RenderTargetView* current_render_target_view = nullptr;
		ID3D11DepthStencilView* current_depth_view = nullptr;

		ID3D11RenderTargetView* restoreRenderTargetView();
		ID3D11DepthStencilView* restoreDepthStencilView();
	};
}