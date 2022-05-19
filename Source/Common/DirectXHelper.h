#pragma once

#include <ppltasks.h>	// For create_task
//#include <winrt/ppl.h>
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <sstream>

#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.ApplicationModel.h>
#include <cstdarg>
namespace DX
{
	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			// Set a breakpoint on this line to catch Win32 API errors.
			throw winrt::hresult_error(hr);
		}
	}

	inline void getSubDirs(std::string path, std::vector<std::wstring>& sub_folders) {
		std::string substr;
		std::stringstream ss(path);
		while (ss.good()) {
			std::getline(ss, substr, '\\');
			sub_folders.push_back(std::wstring(substr.begin(), substr.end()));
		}
	}
	////https://msdn.microsoft.com/en-us/library/mt299098.aspx
	//Platform::String^ appInstallFolder = Windows::ApplicationModel::Package::Current->InstalledLocation->Path; //where program in installed. Have read only access
	//Platform::String^ localfolder = Windows::Storage::ApplicationData::Current->LocalFolder->Path;	//for local saving for future
	//Platform::String^ roamingFolder = Windows::Storage::ApplicationData::Current->RoamingFolder->Path;	//for sync between devices
	//Platform::String^ temporaryFolder = Windows::Storage::ApplicationData::Current->TemporaryFolder->Path;	//for temp saving. Cleared often by system
	
	inline std::string getFilePath(std::string fileName, bool from_asset = false) {
		if (from_asset) 
			return "Assets/" + fileName;

		winrt::hstring localfolder = winrt::Windows::Storage::ApplicationData::Current().LocalFolder().Path();
		std::wstring folderNameW(localfolder.c_str());
		std::string folderNameA(folderNameW.begin(), folderNameW.end());
		const char* charStr = folderNameA.c_str();
		char outPath[512];
		sprintf_s(outPath, "%s\\%s", charStr, fileName.c_str());
		return std::string(outPath);
	}

	inline concurrency::task<std::vector<byte>> ReadDataAsync(const std::wstring& filename, winrt::Windows::Storage::StorageFolder const & folder) {
		using namespace winrt::Windows::Storage;
		using namespace concurrency;

		auto tsk0 = create_task([=]() {
			auto data = folder.TryGetItemAsync(filename.c_str()).get(); 
			if (data == nullptr)
				return std::vector<byte>();
			else {
				auto file = folder.GetFileAsync(filename.c_str()).get();
				auto fileBuffer = FileIO::ReadBufferAsync(file).get();
				std::vector<byte> returnBuffer;
				returnBuffer.resize(fileBuffer.Length());
				Streams::DataReader::FromBuffer(fileBuffer).ReadBytes(returnBuffer);
				return returnBuffer;
			}
		}); 
		return tsk0;
	}

	// Function that reads from a binary file asynchronously.
	inline concurrency::task<std::vector<byte>> ReadDataAsync(const std::wstring& filename)
	{
		return ReadDataAsync(filename, winrt::Windows::ApplicationModel::Package::Current().InstalledLocation());
	}

	//inline Concurrency::task<void> WriteDataAsync(const std::wstring& filename, byte* fileData, int dsize){//const Platform::Array<byte>^ fileData) {
	//	auto folder = ApplicationData::Current->LocalFolder;

	//	return create_task(folder->CreateFileAsync(Platform::StringReference(filename.c_str()), CreationCollisionOption::ReplaceExisting)).then([=](StorageFile^ file)
	//	{
	//		Platform::Array<byte>^ data = ref new Platform::Array<byte>(fileData,dsize);
	//		FileIO::WriteBytesAsync(file, data);
	//	});
	//}
	inline winrt::Windows::Foundation::IAsyncAction WriteDataAsync(const std::string filepath, byte* fileData, int dsize) {
		using namespace winrt::Windows::Storage;
		using namespace concurrency;

		std::vector<std::wstring> sub_folders;
		getSubDirs(filepath, sub_folders); 
		std::vector<byte> data(fileData, fileData + dsize);

		if (sub_folders.size() <= 4) { //create_task([]() {return; });
			winrt::hstring desiredName(sub_folders.back());
			//auto write_tsk = ;

			StorageFolder const& dst_folder = ApplicationData::Current().LocalFolder();
			//create folders if not exist
			if (sub_folders.size() > 1) {
				auto tsk0 = dst_folder.CreateFolderAsync(sub_folders[0], CreationCollisionOption::OpenIfExists).get();
				if (sub_folders.size() > 2) {
					auto tsk1 = tsk0.CreateFolderAsync(sub_folders[1], CreationCollisionOption::OpenIfExists).get();
					if (sub_folders.size() > 3) {
						auto tsk2 = tsk1.CreateFolderAsync(sub_folders[2], CreationCollisionOption::OpenIfExists).get();
						auto tsk3 = tsk2.CreateFileAsync(desiredName.c_str(), CreationCollisionOption::ReplaceExisting).get();
						co_await FileIO::WriteBytesAsync(tsk3, data);
					}
					else {
						auto tsk2 = tsk1.CreateFileAsync(desiredName.c_str(), CreationCollisionOption::ReplaceExisting).get();
						co_await FileIO::WriteBytesAsync(tsk2, data);
					}
				}
				else {
					auto tsk1 = tsk0.CreateFileAsync(desiredName.c_str(), CreationCollisionOption::ReplaceExisting).get();
					co_await FileIO::WriteBytesAsync(tsk1, data);
				}
			}
			else {
				auto tsk0 = dst_folder.CreateFileAsync(desiredName.c_str(), CreationCollisionOption::ReplaceExisting).get();
				co_await FileIO::WriteBytesAsync(tsk0, data);
			}
		}
	}

	inline winrt::Windows::Foundation::IAsyncAction CopyDataAsync(const std::wstring& src_name, winrt::Windows::Storage::StorageFolder const& src_folder, 
		const std::wstring& dest_name, winrt::Windows::Storage::StorageFolder const& dst_folder) {
		using namespace winrt::Windows::Storage;
		using namespace concurrency; 
		auto data = src_folder.GetItemAsync(src_name.c_str()).get();
		if (data != nullptr) {
			auto src = src_folder.GetFileAsync(src_name.c_str()).get();
			auto fileBuffer = FileIO::ReadBufferAsync(src).get();
			auto dst = dst_folder.CreateFileAsync(dest_name.c_str(),CreationCollisionOption::ReplaceExisting).get();
			std::vector<byte> dataBuffer(fileBuffer.Length());
			Streams::DataReader::FromBuffer(fileBuffer).ReadBytes(dataBuffer);
			FileIO::WriteBytesAsync(dst, dataBuffer).get();
		}
		/*auto task0 = create_task(src_folder.GetFileAsync(src_name.c_str()));
		return task0.then([=](IStorageItem const& data) {
			if (data == nullptr) return create_task([]() {return; });
			else {
				auto read_task = create_task(src_folder.GetFileAsync(src_name.c_str())).then([](StorageFile const& file)
				{
					return FileIO::ReadBufferAsync(file);
				}).then([=](Streams::IBuffer const& fileBuffer) {
					auto open_write_task = create_task(dst_folder.CreateFileAsync(
						dest_name.c_str(),
						CreationCollisionOption::ReplaceExisting));
					open_write_task.then([=](StorageFile const& file)
					{
						std::vector<byte> dataBuffer(fileBuffer.Length());
						Streams::DataReader::FromBuffer(fileBuffer).ReadBytes(dataBuffer);
						FileIO::WriteBytesAsync(file, dataBuffer);
					});
				});
			}
		});*/
	}
	inline winrt::Windows::Foundation::IAsyncAction CopyAssetDataAsync(const std::string src_name,
		const std::wstring& dest_name, 
		winrt::Windows::Storage::StorageFolder const& dst_folder) {
		using namespace winrt::Windows::Storage;
		using namespace concurrency;
		auto tsk0 = dst_folder.CreateFolderAsync(L"helmsley_cached", CreationCollisionOption::OpenIfExists).get();
		auto tsk1 = tsk0.CreateFileAsync(dest_name.c_str(), CreationCollisionOption::ReplaceExisting).get();
		std::ifstream inFile("Assets/" + src_name, std::ios::in | std::ios::binary);
		if (inFile.is_open()) {
			std::vector<byte> data(1024);
			char buffer[1024];
			for (int id = 0; !inFile.eof(); id++) {
				inFile.read(reinterpret_cast<char*>(data.data()), 1024);
				std::streamsize len = inFile.gcount();
				FileIO::WriteBytesAsync(tsk1, data);
			}
		}
	}

	inline winrt::Windows::Foundation::IAsyncAction removeDataAsync(const std::string dirPath) {
		using namespace winrt::Windows::Storage;
		using namespace concurrency;
		std::vector<std::wstring> sub_folders;
		getSubDirs(dirPath, sub_folders);
		if (sub_folders.size() <= 4) {

			StorageFolder& dst_folder = ApplicationData::Current().LocalFolder();

			//create folders if not exist
			if (sub_folders.size() > 1) {
				auto tsk0 = dst_folder.CreateFolderAsync(sub_folders[0].c_str(), CreationCollisionOption::OpenIfExists).get();
				if (sub_folders.size() > 2) {
					auto tsk1 = tsk0.CreateFolderAsync(sub_folders[1].c_str(), CreationCollisionOption::OpenIfExists).get();
					if (sub_folders.size() > 3) {
						auto tsk2 = tsk1.CreateFolderAsync(sub_folders[2].c_str(), CreationCollisionOption::OpenIfExists).get();
						co_await tsk2.DeleteAsync(StorageDeleteOption::PermanentDelete);
					}
					else {
						co_await tsk1.DeleteAsync(StorageDeleteOption::PermanentDelete);
					}
				}
				else {
					co_await tsk0.DeleteAsync(StorageDeleteOption::PermanentDelete);
				}
			}
		}
	}

	inline bool WriteLinesSync(std::string dest_name, std::vector<std::string>& contents, bool overwrite = true) {
		std::ofstream outFile(getFilePath(dest_name), overwrite?std::ios::out : std::ofstream::app);
		if (!outFile.is_open())
			return false;
		for (auto line : contents) {
			outFile << line << std::endl;
		}
		outFile.close();
		return true;
	}

	inline winrt::Windows::Foundation::IAsyncOperation<bool> CopyAssetData(const std::string src_name, const std::string dest_name, bool overwrite) {
		using namespace winrt::Windows::Storage;
		using namespace concurrency;
		std::vector<std::wstring> sub_folders;
		getSubDirs(dest_name, sub_folders);

		if (sub_folders.size() <= 4) {
			auto copy_data_func = [=]()->bool {
				std::ifstream inFile("Assets\\" + src_name, std::ios::in | std::ios::binary);
				if (inFile.is_open()) {
					std::ofstream outFile(getFilePath(dest_name), std::ios::out | std::ios::binary);
					if (outFile.is_open()) {
						outFile << inFile.rdbuf();
						inFile.close();
						outFile.close();
						return true;
					}
				}
				return false;
			};

			auto copy_func = [=](StorageFolder const& folder) {
				if (!overwrite) {
					auto data = folder.TryGetItemAsync(sub_folders.back().c_str()).get();
					if (data == nullptr) return copy_data_func();
				}
				return copy_data_func();
			};

			StorageFolder const& dst_folder = ApplicationData::Current().LocalFolder();
			//create folders if not exist
			if (sub_folders.size() > 1) {
				StorageFolder tsk0 = dst_folder.CreateFolderAsync(sub_folders[0].c_str(), CreationCollisionOption::OpenIfExists).get();
				if (sub_folders.size() > 2) {
					auto tsk1 = tsk0.CreateFolderAsync(sub_folders[1].c_str(), CreationCollisionOption::OpenIfExists).get();
					if (sub_folders.size() > 3) {
						auto tsk2 = tsk1.CreateFolderAsync(sub_folders[2].c_str(), CreationCollisionOption::OpenIfExists).get();
						co_return copy_func(tsk2);
					}
					else {
						co_return copy_func(tsk1);
					}
				}
				else {
					co_return copy_func(tsk0);
				}
			}
		}
		else {
			co_return false;
		}
	}

	inline bool ReadAllLines(const std::string filename, std::vector<std::string>& lines, bool from_asset = false) {
		std::ifstream inFile(getFilePath(filename, from_asset), std::ios::in);
		if (!inFile.is_open())
			return false;
		std::string line = "";
		while (getline(inFile, line))lines.push_back(line);
		inFile.close();
		return true;
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
