#pragma once

#include <vector>

#include "TalentTreeManagerDefinitions.h"

namespace TTM {
	static size_t write_memory(void* buffer, size_t size, size_t nmemb, void* param);
	inline void flagAllResources(UIData& uiData);
	std::string checkForUpdate(UIData& uiData);
	int compareVersions(std::string clientVersion, std::string remoteVersion);
	void updateResources(UIData& uiData, TalentTreeCollection& talentTreeCollection);
	void updatePresets(UIData& uiData);
	void updateIcons(UIData& uiData);
	void testUpdateIcons(UIData& uiData);

	std::string exportToPastebin(std::string treeRep, std::string treeName);
}