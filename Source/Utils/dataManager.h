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
	dataManager();
	dataManager(const std::shared_ptr<rpcHandler>& rpc_handler);
private:
	std::shared_ptr<rpcHandler> m_rpc_handler;

	std::vector<datasetResponse::datasetInfo> m_remote_datasets, m_local_datasets;
	std::unordered_map<std::string, std::set<volumeResponse::volumeInfo, vInfoCmp>> m_local_dv_map;

	void setup_local_datasets();
	void update_local_info(datasetResponse::datasetInfo tInfo, volumeResponse::volumeInfo vInfo);
};

#endif