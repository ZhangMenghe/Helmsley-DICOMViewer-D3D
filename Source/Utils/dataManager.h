#ifndef UTILS_DATA_MANAGER_H
#define UTILS_DATA_MANAGER_H
#include <grpc/rpcHandler.h>
struct vInfoCmp {
	bool operator() (const volumeResponse::volumeInfo& lhs, const volumeResponse::volumeInfo& rhs) const
	{
		return lhs.folder_name().compare(rhs.folder_name()) == 0;
	}
};
class dataManager {
public:
	dataManager(const std::shared_ptr<dicomLoader>& dicom_loader);
	dataManager(const std::shared_ptr<dicomLoader>& dicom_loader, const std::shared_ptr<rpcHandler>& rpc_handler);
	
	bool removeLocalData(std::string dsName, volumeResponse::volumeInfo vInfo);
	bool loadData(std::string dsName, volumeResponse::volumeInfo vInfo, bool isLocal);
	
	//getter
	std::vector<datasetResponse::datasetInfo> getAvailableDataset(bool isLocal);
	std::vector<volumeResponse::volumeInfo> getAvailableVolumes(std::string dsname, bool isLocal);

private:
	const std::string ASSET_RESERVE_DS = "Larry_Smarr_2016";
	const std::string ASSET_RESERVE_VL = "series_23_Cor_LAVA_PRE-Amira";
	const std::string m_index_file_path = dvr::CACHE_FOLDER_NAME + "\\" + dvr::CONFIG_NAME;

	std::shared_ptr<rpcHandler> m_rpc_handler;
	std::shared_ptr<dicomLoader> m_dicom_loader;

	datasetResponse::datasetInfo m_target_ds;
	volumeResponse::volumeInfo m_target_vl;
	std::vector<datasetResponse::datasetInfo> m_remote_datasets, m_local_datasets;
	std::unordered_map<std::string, std::set<volumeResponse::volumeInfo, vInfoCmp>> m_local_dv_map;

	bool m_local_initialized = false;
	void setup_local_datasets();
	void update_local_info(datasetResponse::datasetInfo tInfo, volumeResponse::volumeInfo vInfo);
	void save_target_dcmi();
};

#endif