#include "pch.h"
#include "dataManager.h"
#include <sstream> 
#include <Common/DirectXHelper.h>
dataManager::dataManager() {
	setup_local_datasets();
}

dataManager::dataManager(const std::shared_ptr<rpcHandler>& rpc_handler)
:m_rpc_handler(rpc_handler){
	m_rpc_handler->getRemoteDatasets(m_remote_datasets);
	setup_local_datasets();
}

void dataManager::setup_local_datasets() {
	std::string config_path = dvr::CACHE_FOLDER_NAME + "/" + dvr::CONFIG_NAME;

	auto readIndexFileTask = DX::ReadDataAsync(std::wstring(config_path.begin(), config_path.end()), Windows::Storage::ApplicationData::Current->LocalFolder);
	readIndexFileTask.then([this](const std::vector<byte>& fileData) {
		if (fileData.empty()) return;

		std::stringstream ss(reinterpret_cast<const char*>(&fileData[0]), fileData.size());
		std::string line, substr;
		std::vector<std::string> linfo(50);

		datasetResponse::datasetInfo tinfo;
		volumeResponse::volumeInfo vinfo;

		for (int idx = 0; std::getline(ss, line); idx++) {
			if (idx % 2 == 0) { //record info
				std::stringstream ssv(line);
				for (int iv = 0; ssv.good(); iv++) {
					std::getline(ssv, linfo[iv], '#');
				}
				//Larry Smarr/2016-10-26/Larry_Smarr_2016/series_23_Cor_LAVA_PRE-Amira/series_23_Cor_LAVA_PRE-Amira/512, 512, 144/1.0, -0.0, 0.0, -0.0, -0.0, -1.0/0.8984, 0.8984/243.10002131
				tinfo.set_patient_name(linfo[0]);
				tinfo.set_date(linfo[1]);
				tinfo.set_folder_name(linfo[2]);

				vinfo.set_folder_name(linfo[3]);
				vinfo.set_folder_path(linfo[4]);
				vinfo.set_volume_loc_range(std::stof(linfo[8]));
				vinfo.set_with_mask(linfo[9].compare("true") == 0);
				vinfo.set_data_source((volumeResponse::volumeInfo::DataSource) std::stoi(linfo[10]));
				//set dimension
				std::stringstream ssd(linfo[5]);
				while (ssd.good()) {
					std::getline(ssd, substr, ',');
					vinfo.add_dims(std::stoi(substr));
				}
				//set orientation
				std::stringstream sso(linfo[6]);
				while (sso.good()) {
					std::getline(sso, substr, ',');
					vinfo.add_orientation(std::stof(substr));
				}
				//set resolution
				std::stringstream ssr(linfo[7]);
				while (ssr.good()) {
					std::getline(ssr, substr, ',');
					vinfo.add_resolution(std::stof(substr));
				}
				//todo: sample image
			}
			else { //score
				std::stringstream ssv(line);
				int ids = 0;
				for (ids = 0; ssv.good(); ids++)
					std::getline(ssv, linfo[ids], '#');
				
				//group/rank/rscore/...../volscore*3
				volumeResponse::scoreInfo sInfo;
				sInfo.set_rgroup_id(std::stoi(linfo[0]));
				sInfo.set_rank_id(std::stoi(linfo[1]));
				sInfo.set_rank_score(std::stof(linfo[2]));
				int param_end_id = ids - 3;
				for (int ri = 3; ri < param_end_id; ri++)
					sInfo.add_raw_score(std::stof(linfo[ri]));
				for (int ni = param_end_id; ni < ids; ni++)
					sInfo.add_vol_score(std::stof(linfo[ni]));
				vinfo.set_allocated_scores(&sInfo);
				update_local_info(tinfo, vinfo);
			}
		}
	});
}
void dataManager::update_local_info(datasetResponse::datasetInfo tInfo, volumeResponse::volumeInfo vInfo) {
	std::string dsname = tInfo.folder_name();
	if (m_local_dv_map.count(dsname) == 0) m_local_datasets.push_back(tInfo);
	m_local_dv_map[dsname].insert(vInfo);
}

