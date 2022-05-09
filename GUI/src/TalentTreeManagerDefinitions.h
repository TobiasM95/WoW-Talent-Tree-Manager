#pragma once

#include <vector>
#include <memory>
#include <filesystem>

#include "imgui.h"

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