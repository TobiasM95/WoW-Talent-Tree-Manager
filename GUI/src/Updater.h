#pragma once

#include <vector>

#include "TalentTreeManagerDefinitions.h"

namespace TTM {
	static size_t write_memory(void* buffer, size_t size, size_t nmemb, void* param);
	inline void flagAllResources(UIData& uiData);
	void checkForUpdate(UIData& uiData);
	void updateResources(UIData& uiData, TalentTreeCollection& talentTreeCollection);
	void updatePresets(UIData& uiData);
}