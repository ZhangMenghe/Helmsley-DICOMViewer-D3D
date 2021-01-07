#pragma once

#include <ppltasks.h>	// For create_task

namespace DX
{
	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			// Set a breakpoint on this line to catch Win32 API errors.
			throw Platform::Exception::CreateException(hr);
		}
	}
	inline Concurrency::task<std::vector<byte>> ReadDataAsync(const std::wstring& filename, Windows::Storage::StorageFolder^ folder) {
		using namespace Windows::Storage;
		using namespace Concurrency;

		return create_task(folder->GetFileAsync(Platform::StringReference(filename.c_str()))).then([](StorageFile^ file)
		{
			return FileIO::ReadBufferAsync(file);
		}).then([](Streams::IBuffer^ fileBuffer) -> std::vector<byte>
		{
			std::vector<byte> returnBuffer;
			returnBuffer.resize(fileBuffer->Length);
			Streams::DataReader::FromBuffer(fileBuffer)->ReadBytes(Platform::ArrayReference<byte>(returnBuffer.data(), fileBuffer->Length));
			return returnBuffer;
		});
	}

	// Function that reads from a binary file asynchronously.
	inline Concurrency::task<std::vector<byte>> ReadDataAsync(const std::wstring& filename)
	{
		return ReadDataAsync(filename, Windows::ApplicationModel::Package::Current->InstalledLocation);
	}

	inline Concurrency::task<void> WriteDataAsync(const std::wstring& filename, byte* fileData, int dsize){//const Platform::Array<byte>^ fileData) {
		using namespace Windows::Storage;
		using namespace Concurrency;

		auto folder = ApplicationData::Current->LocalFolder;

		return create_task(folder->CreateFileAsync(Platform::StringReference(filename.c_str()), CreationCollisionOption::ReplaceExisting)).then([=](StorageFile^ file)
		{
			Platform::Array<byte>^ data = ref new Platform::Array<byte>(fileData,dsize);
			FileIO::WriteBytesAsync(file, data);
		});
	}
	// Converts a length in device-independent pixels (DIPs) to a length in physical pixels.
	inline float ConvertDipsToPixels(float dips, float dpi)
	{
		static const float dipsPerInch = 96.0f;
		return floorf(dips * dpi / dipsPerInch + 0.5f); // Round to nearest integer.
	}

#if defined(_DEBUG)
	// Check for SDK Layer support.
	inline bool SdkLayersAvailable()
	{
		HRESULT hr = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_NULL,       // There is no need to create a real hardware device.
			0,
			D3D11_CREATE_DEVICE_DEBUG,  // Check for the SDK layers.
			nullptr,                    // Any feature level will do.
			0,
			D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION for Microsoft Store apps.
			nullptr,                    // No need to keep the D3D device reference.
			nullptr,                    // No need to know the feature level.
			nullptr                     // No need to keep the D3D device context reference.
			);

		return SUCCEEDED(hr);
	}
#endif
}
