#ifndef UTILS_DATA_MANAGER_H
#define UTILS_DATA_MANAGER_H
#include <grpc/rpcHandler.h>
struct vInfoCmp {
	bool operator() (const volumeInfo& lhs, const volumeInfo& rhs) const
	{
		return lhs.folder_name().compare(rhs.folder_name()) == 0;
	}
};
class dataManager {
public:
	dataManager(const std::shared_ptr<dicomLoader>& dicom_loader);
	dataManager(const std::shared_ptr<dicomLoader>& dicom_loader, const std::shared_ptr<rpcHandler>& rpc_handler);

	bool removeLocalData(std::string dsName, volumeInfo vInfo);
	bool loadData(std::string dsName, std::string vlName);
	bool loadData(std::string dsName, volumeInfo vInfo, bool isLocal);

	//getter
	std::vector<datasetResponse::datasetInfo> getAvailableDataset(bool isLocal);
	void getAvailableVolumes(std::string dsname, std::vector<volumeInfo>& ret, bool isLocal);

private:
	const std::string m_index_file_path;

	std::shared_ptr<rpcHandler> m_rpc_handler;
	std::shared_ptr<dicomLoader> m_dicom_loader;

	std::string m_target_ds_name;
	datasetResponse::datasetInfo m_target_ds;
	volumeInfo m_target_vl;
	std::vector<datasetResponse::datasetInfo> m_remote_datasets, m_local_datasets;
	std::unordered_map<std::string, std::vector<volumeInfo>> m_local_dv_map;

	bool m_local_initialized = false;
	void setup_local_datasets();
	void update_local_info(datasetResponse::datasetInfo tInfo, volumeInfo vInfo);
	void save_target_dcmi();
	void prepare_target_volume();
};

#endif