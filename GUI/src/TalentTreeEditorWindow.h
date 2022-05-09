#pragma once

#include "TalentTrees.h"
#include "TalentTreeManagerDefinitions.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"

namespace TTM {
	static void AttachTalentEditTooltip(Engine::Talent talent);

	void RenderTalentTreeEditorWindow(UIData& uiData, TalentTreeCollection& talentTreeCollection);

	//TTMTODO: add consts to functions that do not modify uiData and talentTreeCollection
	void validateAndInsertTalent(UIData& uiData, TalentTreeCollection& talentTreeCollection, std::map<int, std::shared_ptr<Engine::Talent>> comboIndexTalentMap);
	void validateTalentUpdate(UIData& uiData, TalentTreeCollection& talentTreeCollection, std::map<int, std::shared_ptr<Engine::Talent>> comboIndexTalentMap);
	bool updateCustomTreeFileList(UIData& uiData);
	bool checkIfTreeFileExists(UIData& uiData, TalentTreeCollection& talentTreeCollection);
	bool saveTreeToFile(UIData& uiData, TalentTreeCollection& talentTreeCollection);
	bool loadTreeFromFile(UIData& uiData, TalentTreeCollection& talentTreeCollection);
	bool deleteTreeFromFile(UIData& uiData, TalentTreeCollection& talentTreeCollection);
	std::string treeNameToFileName(std::string treeName);
	bool getTreeNameFromFile(std::filesystem::path entry, std::string& treeName);
	void placeTreeElements(UIData& uiData, TalentTreeCollection& talentTreeCollection);
	void selectTalent(UIData& uiData, TalentTreeCollection& talentTreeCollection, std::pair<int, std::shared_ptr<Engine::Talent>> talent);
	void repositionTalent(
		UIData& uiData,
		TalentTreeCollection& talentTreeCollection,
		std::pair<int, std::shared_ptr<Engine::Talent>> talent,
		ImVec2 mouseClickedPos,
		ImVec2 deltaMouseTot,
		ImVec2 buttonPos);
	void drawArrowBetweenTalents(
		std::shared_ptr<Engine::Talent> t1,
		std::shared_ptr<Engine::Talent> t2,
		ImDrawList* drawList,
		ImVec2 windowPos,
		ImVec2 offset,
		ImVec2 talentWindowPadding,
		int talentHalfSpacing,
		int talentSize,
		float talentPadding,
		UIData& uiData);
}