#ifndef DEV_RES_D3D_H
#define DEV_RES_D3D_H
#include "pch.h"
class DeviceResourceD3D {
public:
	DeviceResourceD3D();
	DeviceResourceD3D(IDXGIAdapter1* adapter);
	//getter
	ID3D11Device* GetD3DDevice() const { return m_d3dDevice;}
	ID3D11DeviceContext* GetD3DDeviceContext() const { return m_d3dContext; }
	Windows::Foundation::Size GetOutputSize() const { return m_outputSize; }
	Windows::Foundation::Size GetLogicalSize() const { return m_logicalSize; }

	//setter
	void SetWindow(Windows::UI::Core::CoreWindow^ window);
	void SetLogicalSize(Windows::Foundation::Size logicalSize);

private:
	ID3D11Device* m_d3dDevice = nullptr;
	ID3D11DeviceContext* m_d3dContext = nullptr;

	//CONSTANTS
	Windows::Foundation::Size m_outputSize;
	Windows::Foundation::Size m_logicalSize;




	D3D_FEATURE_LEVEL m_d3dFeatureLevel;

	void CreateWindowSizeDependentResources();


};
#endif // !DEV_RES_D3D_H