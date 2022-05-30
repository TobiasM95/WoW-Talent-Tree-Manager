#pragma once

#include <tuple>
#include <vector>
#include <memory>
#include <filesystem>

#include "imgui.h"

#include "TalentTrees.h"
#include "TreeSolver.h"
#include "TTMGUIPresets.h"

namespace TTM {

	struct TextFilters
	{
		//only letters and numbers and blank space
		static int FilterNameLetters(ImGuiInputTextCallbackData* data)
		{
			ImWchar c = data->EventChar;
			if (c >= 'A' && c <= 'Z') return 0;
			if (c >= 'a' && c <= 'z') return 0;
			if (c >= '0' && c <= '9') return 0;
			if (c == ' ') return 0;
			if (c == ':') return 0;
			if (c == '(' || c == ')') return 0;
			return 1;
		}
	};

	enum class EditorView {
		None, TreeEdit, LoadoutEdit, LoadoutSolver
	};

	enum class TreeEditPage {
		TreeInformation, TreeEdit, SaveLoadTree
	};

	enum class LoadoutEditPage {
		LoadoutInformation, SkillsetEdit
	};

	enum class LoadoutSolverPage {
		SolutionResults
	};

	struct UIData {
		Presets::STYLES style = Presets::STYLES::COMPANY_GREY;
		EditorView editorView = EditorView::None;
		bool showAboutPopup = false;
		bool showHelpPopup = false;

		int deleteTreeIndex = -1;

		//################# TREE EDITOR VARIABLES #######################
		TreeEditPage treeEditPage = TreeEditPage::TreeInformation;

		std::vector<std::tuple<Engine::Talent_s, int, int, int>> treeEditorTempReplacedTalents;

		Engine::Talent_s treeEditorCreationTalent = std::make_shared<Engine::Talent>();
		std::vector<int> treeEditorCreationTalentParentsPlaceholder;
		std::vector<int> treeEditorCreationTalentChildrenPlaceholder;

		bool talentJustSelected = false;
		Engine::Talent_s treeEditorSelectedTalent = nullptr;
		std::vector<int> treeEditorSelectedTalentParentsPlaceholder;
		std::vector<int> treeEditorSelectedTalentChildrenPlaceholder;

		int treeEditorShiftAllRowsBy = 0;
		int treeEditorShiftAllColumnsBy = 0;
		int minRowShift, maxRowShift, minColShift, maxColShift;

		int treeEditorEmptyActiveNodes = 0;
		int treeEditorEmptyPassiveNodes = 0;
		int treeEditorEmptySwitchNodes = 0;

		int treeEditorPresetClassCombo = 0;
		int treeEditorPresetSpecCombo = 0;
		bool treeEditorPresetsKeepLoadout = false;

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

		//############# LOADOUT EDITOR VARIABLES ########################
		bool isLoadoutInitValidated = false;
		LoadoutEditPage loadoutEditPage = LoadoutEditPage::LoadoutInformation;

		int loadoutEditorRightClickIndex = 0;

		std::string loadoutEditorExportActiveSkillsetString;
		std::string loadoutEditorExportAllSkillsetsString;
		std::string loadoutEditorImportSkillsetsString;
		std::pair<int, int> loadoutEditorImportSkillsetsResult;

		//############# LOADOUT SOLVER VARIABLES ########################
		const int loadoutSolverMaxTalentPoints = 64;
		int loadoutSolverTalentPointLimit = 1;
		LoadoutSolverPage loadoutSolverPage = LoadoutSolverPage::SolutionResults;
		int loadoutSolverAllCombinationsAdded;
		int loadoutSolverTalentPointSelection = -1;
		const int loadoutSolverResultsPerPage = 50;
		bool loadoutSolverAutoApplyFilter = false;
		//the selected/requested page of skillset results
		int loadoutSolverSkillsetResultPage = -1;
		//the currently buffered results page which should differ from the selected/requested page only for 1 frame
		int loadoutSolverBufferedPage = -1;
		std::vector<std::pair<Engine::SIND, int>> loadoutSolverPageResults;
		int selectedFilteredSkillsetIndex = -1;
		Engine::SIND selectedFilteredSkillset = 0;
	};

	struct TalentTreeData {
		Engine::TalentTree tree;

		bool isTreeSolveProcessed = false;
		bool isTreeSolveFiltered = false;
		std::shared_ptr<Engine::TreeDAGInfo> treeDAGInfo;
		std::shared_ptr<Engine::TalentSkillset> skillsetFilter;
		bool restrictTalentPoints = false;
		int restrictedTalentPoints = 0;
	};

	struct TalentTreeCollection {
		int activeTreeIndex = -1;
		std::vector<TalentTreeData> trees;

		TalentTreeData& activeTreeData() {
			if (activeTreeIndex >= 0 && activeTreeIndex < trees.size())
				return trees[activeTreeIndex];
			else
				throw std::logic_error("Active tree index is -1 or larger than tree vector!");
		}
		Engine::TalentTree& activeTree() {
			if (activeTreeIndex >= 0 && activeTreeIndex < trees.size())
				return trees[activeTreeIndex].tree;
			else
				throw std::logic_error("Active tree index is larger than tree vector!");
		}

		std::shared_ptr<Engine::TalentSkillset> activeSkillset() {
			if (trees[activeTreeIndex].tree.activeSkillsetIndex >= 0 && trees[activeTreeIndex].tree.activeSkillsetIndex < trees[activeTreeIndex].tree.loadout.size())
				return trees[activeTreeIndex].tree.loadout[trees[activeTreeIndex].tree.activeSkillsetIndex];
			else
				throw std::logic_error("Active skillset index is -1 or larger than loadout size!");
		}
	};

	void drawArrowBetweenTalents(
		Engine::Talent_s t1,
		Engine::Talent_s t2,
		ImDrawList* drawList,
		ImVec2 windowPos,
		ImVec2 offset,
		ImVec2 talentWindowPadding,
		int talentHalfSpacing,
		int talentSize,
		float talentPadding,
		UIData& uiData);

	void clearSolvingProcess(UIData& uiData, TalentTreeCollection& talentTreeCollection);
}