#include "pch.h"
#include "dataManager.h"
#include <sstream> 
#include <Common/DirectXHelper.h>
dataManager::dataManager(const std::shared_ptr<dicomLoader>& dicom_loader)
	:m_dicom_loader(dicom_loader) {
	setup_local_datasets();
}

dataManager::dataManager(const std::shared_ptr<dicomLoader>& dicom_loader, const std::shared_ptr<rpcHandler>& rpc_handler)
:m_dicom_loader(dicom_loader), m_rpc_handler(rpc_handler){
	m_rpc_handler->getRemoteDatasets(m_remote_datasets);
	setup_local_datasets();
}

bool dataManager::removeLocalData(std::string dsName, volumeResponse::volumeInfo vInfo) {
	if (m_local_dv_map.count(dsName) == 0) return false;
	//remove from index file

	std::vector<std::string> config_lines;
	if (!DX::ReadAllLines("helmsley_cached\\pacs_local.txt", config_lines))
		return false;

	std::string vlName = vInfo.folder_name();
	std::vector<std::string> linfo(50);

	for (int i = 0; i < config_lines.size(); i += 2) {
		std::stringstream ssv(config_lines[i]);
		for (int iv = 0; ssv.good()&&iv<4; iv++) std::getline(ssv, linfo[iv], '#');
		if (linfo[2].compare(dsName) != 0 || linfo[3].compare(vlName) != 0) continue;

		//update local_dv_map and m_local_ds
		for (auto itr = m_local_dv_map[dsName].begin(); itr != m_local_dv_map[dsName].end(); itr++) {
			if (itr->folder_name().compare(vlName) == 0) {
				m_local_dv_map[dsName].erase(itr);
				if (m_local_dv_map[dsName].empty()) {
					m_local_dv_map.erase(dsName);
					m_local_datasets.erase(std::find_if(m_local_datasets.begin(), m_local_datasets.end(), [dsName](const datasetResponse::datasetInfo& s) { return s.folder_name().compare(dsName) == 0; }));
				}
				break;
			}
		}
		//update info contents
		config_lines.erase(config_lines.begin() + i + 1);
		config_lines.erase(config_lines.begin() + i);
		
		//write back to file
		if (!DX::WriteLinesSync(m_index_file_path, config_lines)) return false;
		break;
	}

	//remove data directory
	DX::removeDataAsync(dsName + "\\" + vInfo.folder_name()).get();
	return true;
}
bool dataManager::loadData(std::string dsName, volumeResponse::volumeInfo vInfo, bool isLocal) {
	if (dsName.compare(m_target_ds.folder_name()) != 0)return false;
	m_target_vl = vInfo;
	std::string vl_path = dvr::CACHE_FOLDER_NAME + "\\" + dsName + "\\" + vInfo.folder_name() + "\\";

	if (isLocal) {
		bool b_asset = dsName.compare(ASSET_RESERVE_DS) == 0 && vInfo.folder_name().compare(ASSET_RESERVE_VL) == 0;
		if (!m_dicom_loader->loadData(vl_path, vInfo.with_mask(), b_asset)) return false;
		m_dicom_loader->sendDataDone();
	}
	else {
		if (m_rpc_handler == nullptr) return false;
		std::string path = dsName + "/" + vInfo.folder_name();
		 m_rpc_handler->DownloadVolumeAsync(path).then([this, path, vInfo, vl_path]() {
			if (vInfo.with_mask()) {
				m_rpc_handler->DownloadMasksAndCenterlinesAsync(path).then([this, vl_path]() {
					save_target_dcmi();
					m_dicom_loader->sendDataDone();
					m_dicom_loader->saveAndUseCenterLineData(vl_path + "centerline.txt");
				});
			}else {
				save_target_dcmi();
				m_dicom_loader->sendDataDone();
			}
		});
	}
	return true;
}
std::vector<datasetResponse::datasetInfo> dataManager::getAvailableDataset(bool isLocal) {
	return isLocal ? m_local_datasets : m_remote_datasets;
}
std::vector<volumeResponse::volumeInfo> dataManager::getAvailableVolumes(std::string dsname, bool isLocal) {
	if (isLocal) {
		for (auto tInfo : m_local_datasets) {
			if (tInfo.folder_name().compare(dsname) == 0) {
				m_target_ds = tInfo; break;
			}
		}
		return m_local_dv_map[dsname];
	}
	if (m_rpc_handler == nullptr)return std::vector<volumeResponse::volumeInfo>();
	
	for (auto tInfo : m_remote_datasets) {
		if (tInfo.folder_name().compare(dsname) == 0) {
			m_target_ds = tInfo; break;
		}
	}
	return m_rpc_handler->getVolumeFromDataset(dsname);
}

void dataManager::setup_local_datasets() {
	std::vector<std::string> fileData;
	if (!DX::ReadAllLines(m_index_file_path, fileData) || fileData.empty())
		return;
	std::string substr;
	std::vector<std::string> linfo(50);

	datasetResponse::datasetInfo tinfo;
	volumeResponse::volumeInfo vinfo;
	for (int idx = 0; idx < fileData.size(); idx++) {
		std::stringstream ssv(fileData[idx]);
		if (idx % 2 == 0) {
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
		else {
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
			vinfo.release_scores();
			tinfo.Clear();
			vinfo.Clear();
		}
	}
	m_local_initialized = true;
	linfo.clear();
}
void dataManager::update_local_info(datasetResponse::datasetInfo tInfo, volumeResponse::volumeInfo vInfo) {
	std::string dsname = tInfo.folder_name();
	if (m_local_dv_map.count(dsname) == 0) m_local_datasets.push_back(tInfo);
	
	auto itr = std::find_if(m_local_dv_map[dsname].begin(),
		m_local_dv_map[dsname].end(), 
		[vInfo](const volumeResponse::volumeInfo& s) { return s.folder_name().compare(vInfo.folder_name()) == 0; });
	if (itr == m_local_dv_map[dsname].end()) m_local_dv_map[dsname].push_back(vInfo);
	else *itr = vInfo;
}
void dataManager::save_target_dcmi() {
	update_local_info(m_target_ds, m_target_vl);
	//update pac_local
	std::vector<std::string> app_info;
	std::string vs = m_target_ds.patient_name()
		+ "#" + m_target_ds.date()
		+ "#" + m_target_ds.folder_name()
		+ "#" + m_target_vl.folder_name()
		+ "#" + m_target_vl.folder_name()
		+ "#";


	//dims
	for (auto dim : m_target_vl.dims()) vs += std::to_string(dim) + ",";
	vs[vs.size() - 1] = '#';
	//set orientation
	for (auto ori : m_target_vl.orientation()) vs += std::to_string(ori) + ",";
	vs[vs.size() - 1] = '#';
	//set resolution
	for (auto res : m_target_vl.resolution()) vs += std::to_string(res) + ",";
	vs[vs.size() - 1] = '#';
	vs += std::to_string(m_target_vl.volume_loc_range())
		+ "#" + (m_target_vl.with_mask() ? "true" : "false")
		+ "#" + std::to_string(int(m_target_vl.data_source()));
	
	auto sinfo = m_target_vl.scores();
	std::string ss = std::to_string(sinfo.rgroup_id())
		+ "#" + std::to_string(sinfo.rank_id())
		+ "#" + std::to_string(sinfo.rank_score());
	for (auto rs : sinfo.raw_score()) ss += "#" + std::to_string(rs);
	for (auto rs : sinfo.vol_score()) ss += "#" + std::to_string(rs);
	app_info.push_back(vs); app_info.push_back(ss);
	if (!DX::WriteLinesSync(m_index_file_path, app_info, false)) return;

	//save data
	std::string vl_path = dvr::CACHE_FOLDER_NAME + "\\" + m_target_ds.folder_name() + "\\" + m_target_vl.folder_name() + "\\";
	
	vl_path += m_target_vl.with_mask() ? "data_w_mask" : "data";
	m_dicom_loader->saveData(vl_path);
}

