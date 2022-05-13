#pragma once

#include <vector>
#include <string>
#include <stdexcept>
#include <filesystem>

#include "TalentTreeManagerDefinitions.h"
#include "TalentTrees.h"

namespace TTM {

	//TTMTODO: add consts to functions that do not modify uiData and talentTreeCollection
	void RenderMainWindow(UIData& uiData, TalentTreeCollection& talentTreeCollection, bool& done);
	void RenderMenuBar(UIData& uiData, TalentTreeCollection& talentTreeCollection, bool& done);
	void RenderWorkArea(UIData& uiData, TalentTreeCollection& talentTreeCollection);
	void RenderTalentTreeTabs(UIData& uiData, TalentTreeCollection& talentTreeCollection);
	void RenderTreeViewTabs(UIData& uiData, TalentTreeCollection& talentTreeCollection);
	void SubmitDockSpace();
	void RenderWorkAreaWindow(UIData& uiData, TalentTreeCollection& talentTreeCollection);
	void RenderStatusBar(UIData& uiData, TalentTreeCollection& talentTreeCollection);
	std::filesystem::path getAppPath();
	std::filesystem::path getCustomTreePath();

	void saveWorkspace(UIData& uiData, TalentTreeCollection& talentTreeCollection);
	TalentTreeCollection loadWorkspace(UIData& uiData);
}