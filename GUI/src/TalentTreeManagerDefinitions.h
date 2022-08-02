#pragma once

#include <d3d11.h>

#include <tuple>
#include <vector>
#include <memory>
#include <filesystem>
#include <chrono>

#include "imgui.h"
#include "imgui_internal.h"

#include "ImageHandler.h"
#include "TalentTrees.h"
#include "TreeSolver.h"
#include "TTMGUIPresets.h"

namespace TTM {
	enum class UpdateStatus {
		UPTODATE, 
		OUTDATED, 
		UPDATEERROR, 
		NOTCHECKED, 
		IGNOREUPDATE, 
		UPDATEINITIATED, 
		UPDATEINPROGRESS, 
		RESETRESOURCES
	};

	static const int TTM_RESOURCE_TYPE_COUNT = 3;
	enum class ResourceType {
		TTMVERSION = 0, PRESET = 1, ICONS = 2
	};

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
			if (c == '\'') return 0;
			if (c == '-') return 0;
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
		bool showResetPopup = false;
		bool resetWorkspace = false;

		std::string menuBarUpdateLabel = "";

		int deleteTreeIndex = -1;

		//################# GENERAL #####################################
		int loadedIconTreeIndex = -1;
		ID3D11Device* g_pd3dDevice;
		std::filesystem::path defaultIconPath = "resources/icons/default.png";
		std::map<std::string, std::filesystem::path> iconPathMap;
		std::map<int, std::pair<TextureInfo, TextureInfo>> iconIndexMap;
		std::map<int, TextureInfo> splitIconIndexMap;
		TextureInfo redIconGlow;
		TextureInfo greenIconGlow;
		TextureInfo goldIconGlow;
		TextureInfo blueIconGlow;
		bool enableGlow = true;

		std::string talentSearchString = "";
		Engine::TalentVec searchedTalents;

		std::chrono::duration<long> autoSaveInterval = std::chrono::seconds(300);
		std::chrono::steady_clock::time_point lastSaveTime = std::chrono::high_resolution_clock::now();

		//################# UPDATER #####################################
		//used for displaying some warning messages for specific updates
		std::string updateMessage;
		//used for displaying the screen at least once before a long blocking operation is shown
		bool renderedOnce = false;
		UpdateStatus updateStatus = UpdateStatus::NOTCHECKED;
		std::vector<ResourceType> outOfDateResources;
		bool updateCurrentWorkspace = false;
		bool presetToCustomOverride = false;

		//################# TREE EDITOR VARIABLES #######################
		TreeEditPage treeEditPage = TreeEditPage::TreeInformation;

		std::vector<std::tuple<Engine::Talent_s, int, int, int>> treeEditorTempReplacedTalents;

		Engine::Talent_s treeEditorCreationTalent = std::make_shared<Engine::Talent>();
		std::pair<TextureInfo, TextureInfo> treeEditorCreationTalentIcons;
		std::string treeEditorCreationIconNameFilter;
		std::vector<int> treeEditorCreationTalentParentsPlaceholder;
		std::vector<int> treeEditorCreationTalentChildrenPlaceholder;

		bool talentJustSelected = false;
		Engine::Talent_s treeEditorSelectedTalent = nullptr;
		std::pair<TextureInfo, TextureInfo> treeEditorSelectedTalentIcons;
		std::string treeEditorEditIconNameFilter;
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
		int treeEditorCustomTreeListCurrent = -1;

		std::string treeEditorImportTreeString = "";
		std::string treeEditorExportTreeString = "";
		std::string treeEditorPastebinExportTreeString = "";
		bool pastebinExportCooldownActive = false;
		std::chrono::steady_clock::time_point lastPastebinExportTime = std::chrono::high_resolution_clock::now() - std::chrono::seconds(60);
		std::string treeEditorReadableExportTreeString = "";

		//Important constants
		const int treeEditorBaseTalentHalfSpacing = 20;
		const int treeEditorBaseTalentSize = 40;
		const int treeEditorTalentWindowPaddingX = 25;
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
		int loadoutEditorMiddleClickIndex = 0;

		std::string loadoutEditorExportActiveSkillsetString;
		std::string loadoutEditorExportAllSkillsetsString;
		std::string loadoutEditorImportSkillsetsString;
		std::pair<int, int> loadoutEditorImportSkillsetsResult;

		//############# LOADOUT SOLVER VARIABLES ########################
		bool loadoutSolveInitiated = false;
		bool loadoutSolveInProgress = false;
		const int loadoutSolverMaxTalentPoints = 64;
		int loadoutSolverTalentPointLimit = 1;
		LoadoutSolverPage loadoutSolverPage = LoadoutSolverPage::SolutionResults;
		int loadoutSolverAllCombinationsAdded;
		int loadoutSolverTalentPointSelection = -1;
		const int loadoutSolverResultsPerPage = 50;
		bool loadoutSolverAutoApplyFilter = false;
		//the selected/requested page of skillset results
		int loadoutSolverSkillsetResultPage = -1;
		std::string loadoutSolverSkillsetPrefix = "";
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

		std::map<std::string, std::string> presets;

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

	void refreshIconList(UIData& uiData);
	void loadActiveIcons(UIData& uiData, TalentTreeCollection& talentTreeCollection, bool forceReload = false);
	void loadIcon(UIData& uiData, int index, std::string iconName, ID3D11ShaderResourceView* defaultTexture, int defaultImageWidth, int defaultImageHeight, bool first, Engine::TalentType talentType);
	void loadSplitIcon(UIData& uiData, Engine::Talent_s talent, ID3D11ShaderResourceView* defaultTexture, int defaultImageWidth, int defaultImageHeight);
	TextureInfo loadTextureInfoFromFile(UIData& uiData, std::string iconName, Engine::TalentType talentType = Engine::TalentType::ACTIVE);

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
		UIData& uiData,
		bool colored = false);

	void drawLoadoutEditorShapeAroundTalent(
		Engine::Talent_s talent,
		ImDrawList* drawList,
		ImVec4* colors,
		ImVec2 pos,
		int talentSize,
		ImVec2 windowPos,
		ImVec2 scroll,
		UIData& uiData,
		TalentTreeCollection& talentTreeCollection,
		float disabledAlpha,
		bool searchActive,
		bool talentIsSearchedFor);

	void drawTreeEditorShapeAroundTalent(
		Engine::Talent_s talent,
		ImDrawList* drawList,
		ImVec4* colors,
		ImVec2 pos,
		int talentSize,
		ImVec2 windowPos,
		ImVec2 scroll,
		UIData& uiData,
		TalentTreeCollection& talentTreeCollection,
		bool selected,
		bool searchActive,
		bool talentIsSearchedFor);

	void drawLoadoutSolverShapeAroundTalent(
		Engine::Talent_s talent,
		ImDrawList* drawList,
		ImVec4* colors,
		ImVec2 pos,
		int talentSize,
		ImVec2 windowPos,
		ImVec2 scroll,
		UIData& uiData,
		TalentTreeCollection& talentTreeCollection,
		std::shared_ptr<Engine::TalentSkillset> skillsetFilter,
		bool searchActive,
		bool talentIsSearchedFor);

	void clearSolvingProcess(UIData& uiData, TalentTreeCollection& talentTreeCollection);
}