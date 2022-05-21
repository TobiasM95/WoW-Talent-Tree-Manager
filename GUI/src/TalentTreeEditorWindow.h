#pragma once

#include "TalentTrees.h"
#include "TalentTreeManagerDefinitions.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"

namespace TTM {
	static void AttachTalentEditTooltip(Engine::Talent_s talent);

	void RenderTalentTreeEditorWindow(UIData& uiData, TalentTreeCollection& talentTreeCollection);

	//TTMTODO: add consts to functions that do not modify uiData and talentTreeCollection
	void validateAndInsertTalent(UIData& uiData, TalentTreeCollection& talentTreeCollection, std::map<int, Engine::Talent_s> comboIndexTalentMap);
	void validateTalentUpdate(UIData& uiData, TalentTreeCollection& talentTreeCollection, std::map<int, Engine::Talent_s> comboIndexTalentMap);
	std::filesystem::path getCustomTreePath();
	bool updateCustomTreeFileList(UIData& uiData);
	bool checkIfTreeFileExists(UIData& uiData, TalentTreeCollection& talentTreeCollection);
	bool saveTreeToFile(UIData& uiData, TalentTreeCollection& talentTreeCollection);
	bool loadTreeFromFile(UIData& uiData, TalentTreeCollection& talentTreeCollection);
	bool deleteTreeFromFile(UIData& uiData, TalentTreeCollection& talentTreeCollection);
	std::string treeNameToFileName(std::string treeName);
	bool getTreeNameFromFile(std::filesystem::path entry, std::string& treeName);
	void placeTreeEditorTreeElements(UIData& uiData, TalentTreeCollection& talentTreeCollection);
	void selectTalent(UIData& uiData, TalentTreeCollection& talentTreeCollection, std::pair<int, Engine::Talent_s> talent);
	void repositionTalent(
		UIData& uiData,
		TalentTreeCollection& talentTreeCollection,
		std::pair<int, Engine::Talent_s> talent,
		ImVec2 mouseClickedPos,
		ImVec2 deltaMouseTot,
		ImVec2 buttonPos);
}