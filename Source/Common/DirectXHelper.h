#pragma once

#include <ppltasks.h>	// For create_task
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <sstream>
using namespace Windows::Storage;
using namespace Concurrency;
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
		if (from_asset) return "Assets/" + fileName;

		Platform::String^ localfolder = Windows::Storage::ApplicationData::Current->LocalFolder->Path;
		std::wstring folderNameW(localfolder->Begin());
		std::string folderNameA(folderNameW.begin(), folderNameW.end());
		const char* charStr = folderNameA.c_str();
		char outPath[512];
		sprintf_s(outPath, "%s\\%s", charStr, fileName.c_str());
		return std::string(outPath);
	}
	inline Concurrency::task<std::vector<byte>> ReadDataAsync(const std::wstring& filename, Windows::Storage::StorageFolder^ folder) {
		auto tsk0 = create_task(folder->TryGetItemAsync(Platform::StringReference(filename.c_str())));
		return tsk0.then([filename, folder](IStorageItem^ data) {
			if (data == nullptr)
				return create_task([]() {return std::vector<byte>(); });
			else {
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
		});
	}

	// Function that reads from a binary file asynchronously.
	inline Concurrency::task<std::vector<byte>> ReadDataAsync(const std::wstring& filename)
	{
		return ReadDataAsync(filename, Windows::ApplicationModel::Package::Current->InstalledLocation);
	}

	//inline Concurrency::task<void> WriteDataAsync(const std::wstring& filename, byte* fileData, int dsize){//const Platform::Array<byte>^ fileData) {
	//	auto folder = ApplicationData::Current->LocalFolder;

	//	return create_task(folder->CreateFileAsync(Platform::StringReference(filename.c_str()), CreationCollisionOption::ReplaceExisting)).then([=](StorageFile^ file)
	//	{
	//		Platform::Array<byte>^ data = ref new Platform::Array<byte>(fileData,dsize);
	//		FileIO::WriteBytesAsync(file, data);
	//	});
	//}
	inline void WriteDataAsync(const std::string filepath, byte* fileData, int dsize) {
		std::vector<std::wstring> sub_folders;
		getSubDirs(filepath, sub_folders);

		if (sub_folders.size() > 4) return; //create_task([]() {return; });

		auto write_tsk = [=](StorageFolder^ folder) {
			create_task(folder->CreateFileAsync(Platform::StringReference(sub_folders.back().c_str()),
				CreationCollisionOption::ReplaceExisting)).then([=](StorageFile^ file)
			{
				Platform::Array<byte>^ data = ref new Platform::Array<byte>(fileData, dsize);
				FileIO::WriteBytesAsync(file, data);
			}).then([]() {return; });
		};

		Windows::Storage::StorageFolder^ dst_folder = Windows::Storage::ApplicationData::Current->LocalFolder;
		//create folders if not exist
		if (sub_folders.size() > 1) {
			auto tsk0 = create_task(dst_folder->CreateFolderAsync(Platform::StringReference(sub_folders[0].c_str()), CreationCollisionOption::OpenIfExists));
			if (sub_folders.size() > 2) {
				tsk0.then([=](StorageFolder^ folder) {
					auto tsk1 = create_task(folder->CreateFolderAsync(Platform::StringReference(sub_folders[1].c_str()), CreationCollisionOption::OpenIfExists));
					if (sub_folders.size() > 3) {
						tsk1.then([=](StorageFolder^ folder) {
							auto tsk2 = create_task(folder->CreateFolderAsync(Platform::StringReference(sub_folders[2].c_str()), CreationCollisionOption::OpenIfExists));
							tsk2.then([=](StorageFolder^ folder) {
								write_tsk(folder);
							});
						});
					}
					else {
						tsk1.then([=](StorageFolder^ folder) {
							write_tsk(folder);
						});
					}
				}
				);
			}
			else {
				tsk0.then([=](StorageFolder^ folder) {
					write_tsk(folder);
				});
			}
		}
		else write_tsk(dst_folder);
	}
	inline Concurrency::task<void> CopyDataAsync(const std::wstring& src_name, Windows::Storage::StorageFolder^ src_folder, 
		const std::wstring& dest_name, Windows::Storage::StorageFolder^ dst_folder) {
		auto task0 = create_task(src_folder->TryGetItemAsync(Platform::StringReference(src_name.c_str())));
		return task0.then([=](IStorageItem^ data) {
			if (data == nullptr) return create_task([]() {return; });
			else {
				auto read_task = create_task(src_folder->GetFileAsync(Platform::StringReference(src_name.c_str()))).then([](StorageFile^ file)
				{
					return FileIO::ReadBufferAsync(file);
				}).then([=](Streams::IBuffer^ fileBuffer) {
					auto open_write_task = create_task(dst_folder->CreateFileAsync(
						Platform::StringReference(dest_name.c_str()),
						CreationCollisionOption::ReplaceExisting));
					open_write_task.then([=](StorageFile^ file)
					{
						Platform::Array<byte>^ dataBuffer = ref new Platform::Array<byte>(fileBuffer->Length);
						Streams::DataReader::FromBuffer(fileBuffer)->ReadBytes(dataBuffer);
						FileIO::WriteBytesAsync(file, dataBuffer);
					});
				});

			}
		});
	}
	inline Concurrency::task<void> CopyAssetDataAsync(const std::string src_name,
		const std::wstring& dest_name, 
		Windows::Storage::StorageFolder^ dst_folder) {
		auto tsk0 = create_task(dst_folder->CreateFolderAsync(L"helmsley_cached", CreationCollisionOption::OpenIfExists));
		return tsk0.then([=](StorageFolder^ folder) {
		create_task(folder->CreateFileAsync(Platform::StringReference(dest_name.c_str()), CreationCollisionOption::ReplaceExisting)).then([=](StorageFile^ file)
		{
			std::ifstream inFile("Assets/" + src_name, std::ios::in | std::ios::binary);
			if (!inFile.is_open())
				return create_task([]() {return; });

			Platform::Array<byte>^ data = ref new Platform::Array<byte>(1024);
			char buffer[1024];

			for (int id = 0; !inFile.eof(); id++) {
				inFile.read(reinterpret_cast<char*>(data->Data), 1024);
				std::streamsize len = inFile.gcount();
				FileIO::WriteBytesAsync(file, data);
			}
			return create_task([]() {return; });
		});
		});
	}
	inline Concurrency::task<void> removeDataAsync(const std::string dirPath) {
		std::vector<std::wstring> sub_folders;
		getSubDirs(dirPath, sub_folders);
		if (sub_folders.size() > 4) return create_task([]() {return; });

		Windows::Storage::StorageFolder^ dst_folder = Windows::Storage::ApplicationData::Current->LocalFolder;

		//create folders if not exist
		if (sub_folders.size() > 1) {
			auto tsk0 = create_task(dst_folder->CreateFolderAsync(Platform::StringReference(sub_folders[0].c_str()), CreationCollisionOption::OpenIfExists));
			if (sub_folders.size() > 2) {
				tsk0.then([=](StorageFolder^ folder) {
					auto tsk1 = create_task(folder->CreateFolderAsync(Platform::StringReference(sub_folders[1].c_str()), CreationCollisionOption::OpenIfExists));
					if (sub_folders.size() > 3) {
						tsk1.then([=](StorageFolder^ folder) {
							auto tsk2 = create_task(folder->CreateFolderAsync(Platform::StringReference(sub_folders[2].c_str()), CreationCollisionOption::OpenIfExists));
							tsk2.then([=](StorageFolder^ folder) {
								return create_task(folder->DeleteAsync(StorageDeleteOption::PermanentDelete)).then([=]() {});
							});
						});
					}
					else {
						tsk1.then([=](StorageFolder^ folder) {
							return create_task(folder->DeleteAsync(StorageDeleteOption::PermanentDelete)).then([=]() {});
						});
					}
				}
				);
			}
			else {
				tsk0.then([=](StorageFolder^ folder) {
					return create_task(folder->DeleteAsync(StorageDeleteOption::PermanentDelete)).then([=]() {
					});
				});
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

	inline bool CopyAssetData(const std::string src_name, const std::string dest_name, bool overwrite) {
		std::vector<std::wstring> sub_folders;
		getSubDirs(dest_name, sub_folders);

		if (sub_folders.size() > 4) return false;
		
		auto copy_func = [=](StorageFolder^ folder) {
			auto copy_data_func = [=]() {
				std::ifstream inFile("Assets\\" + src_name, std::ios::in | std::ios::binary);
				if (!inFile.is_open())
					return false;
				std::ofstream outFile(getFilePath(dest_name), std::ios::out | std::ios::binary);
				if (!outFile.is_open())
					return false;
				outFile << inFile.rdbuf();
				inFile.close();
				outFile.close();
				return true;
			};
			if (!overwrite) {
				create_task(folder->TryGetItemAsync(Platform::StringReference(sub_folders.back().c_str()))).then([=](IStorageItem^ data) {
					if (data != nullptr) return true;
					return copy_data_func();
				});
			}
			return copy_data_func();
		};

		Windows::Storage::StorageFolder^ dst_folder = Windows::Storage::ApplicationData::Current->LocalFolder;
		//create folders if not exist
		if (sub_folders.size() > 1) {
			auto tsk0 = create_task(dst_folder->CreateFolderAsync(Platform::StringReference(sub_folders[0].c_str()), CreationCollisionOption::OpenIfExists));
			if (sub_folders.size() > 2) {
				tsk0.then([=](StorageFolder^ folder) {
					auto tsk1 = create_task(folder->CreateFolderAsync(Platform::StringReference(sub_folders[1].c_str()), CreationCollisionOption::OpenIfExists));
					if (sub_folders.size() > 3) {
						tsk1.then([=](StorageFolder^ folder) {
							auto tsk2 = create_task(folder->CreateFolderAsync(Platform::StringReference(sub_folders[2].c_str()), CreationCollisionOption::OpenIfExists));
							tsk2.then([=](StorageFolder^ folder) {
								return copy_func(folder);
							});
						});
					}
					else {
						tsk1.then([=](StorageFolder^ folder) {
							return copy_func(folder);
						});
					}
				}
				);
			}
			else {
				tsk0.then([=](StorageFolder^ folder) {
					return copy_func(folder);
				});
			}
		}
		
		else return copy_func(dst_folder);
	}
	inline bool ReadAllLines(const std::string filename, std::vector<std::string>& lines) {
		std::ifstream inFile(getFilePath(filename), std::ios::in);
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
