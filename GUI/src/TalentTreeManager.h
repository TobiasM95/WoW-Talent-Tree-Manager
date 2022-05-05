#pragma once

#include <vector>
#include <string>
#include <stdexcept>
#include <filesystem>

#include "TalentTrees.h"

namespace TTM {

	enum class EditorView {
		None, TreeEdit, LoadoutEdit, LoadoutSolver
	};

	enum class TreeEditPage {
		TreeInformation, TreeEdit, SaveLoadTree
	};

	struct UIData {
		EditorView editorView = EditorView::None;

		//################# TREE EDITOR VARIABLES #######################
		TreeEditPage treeEditPage = TreeEditPage::TreeInformation;

		std::shared_ptr<Engine::Talent> treeEditorCreationTalent = std::make_shared<Engine::Talent>();
		std::vector<int> treeEditorCreationTalentParentsPlaceholder;
		std::vector<int> treeEditorCreationTalentChildrenPlaceholder;

		bool talentJustSelected = false;
		std::shared_ptr<Engine::Talent> treeEditorSelectedTalent = nullptr;
		std::vector<int> treeEditorSelectedTalentParentsPlaceholder;
		std::vector<int> treeEditorSelectedTalentChildrenPlaceholder;

		int treeEditorPresetClassCombo = 0;
		int treeEditorPresetSpecCombo = 0;

		bool treeEditorIsCustomTreeFileListValid = false;
		std::vector<std::pair<std::filesystem::path, std::string>> treeEditorCustomTreeFileList;
		int treeEditorCustomTreeListCurrent = 0;

		std::string treeEditorImportTreeString = "";
		std::string treeEditorExportTreeString = "";

		//Important constants
		const int treeEditorBaseTalentHalfSpacing = 30;
		const int treeEditorBaseTalentSize = 30;
		const int treeEditorTalentWindowPaddingY = 25;

		int treeEditorDragTalentStartRow = -1;
		int treeEditorDragTalentStartColumn = -1;

		//These values are used to set position after mousewheel zooming in tree editor view
		ImVec2 scrollBuffer;
		ImVec2 maxScrollBuffer;
		float treeEditorZoomFactor = 1.f;
		ImVec2 treeEditorWindowPos;
		ImVec2 treeEditorWindowSize;
		ImVec2 treeEditorMousePos;
		ImVec2 treeEditorRelWorldMousePos;
		//################ END ##########################################
	};

	struct TalentTreeData {
		Engine::TalentTree tree;
	};

	struct TalentTreeCollection {
		int activeTreeIndex = -1;
		std::vector<TalentTreeData> trees;

		TalentTreeData& activeTreeData() {
			if (activeTreeIndex >= 0 && activeTreeIndex < trees.size())
				return trees[activeTreeIndex];
			else
				throw std::logic_error("Active tree index is larger than tree vector!");
		}
		Engine::TalentTree& activeTree() {
			if (activeTreeIndex >= 0 && activeTreeIndex < trees.size())
				return trees[activeTreeIndex].tree;
			else
				throw std::logic_error("Active tree index is larger than tree vector!");
		}
	};

	//TTMTODO: add consts to functions that do not modify uiData and talentTreeCollection
	static void AttachTalentEditTooltip(Engine::Talent talent);
	void RenderMainWindow(UIData& uiData, TalentTreeCollection& talentTreeCollection);
	void RenderMenuBar(UIData& uiData);
	void RenderWorkArea(UIData& uiData, TalentTreeCollection& talentTreeCollection);
	void RenderTalentTreeTabs(UIData& uiData, TalentTreeCollection& talentTreeCollection);
	void RenderTreeViewTabs(UIData& uiData, TalentTreeCollection& talentTreeCollection);
	void SubmitDockSpace();
	void RenderWorkAreaWindow(UIData& uiData, TalentTreeCollection& talentTreeCollection);
	void RenderTalentTreeEditorWindow(UIData& uiData, TalentTreeCollection& talentTreeCollection);
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
	void RenderStatusBar(UIData& uiData, TalentTreeCollection& talentTreeCollection);

}