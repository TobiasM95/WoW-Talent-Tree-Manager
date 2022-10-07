#pragma once

#include <d3d11.h>

#include <array>
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
		None, TreeEdit, LoadoutEdit, LoadoutSolver, SimAnalysis
	};

	enum class TreeEditPage {
		TreeInformation, TreeEdit, SaveLoadTree
	};

	enum class LoadoutEditPage {
		LoadoutInformation, SkillsetEdit
	};

	enum class LoadoutSolverPage {
		SolutionResults, TreeSolveStatus
	};

	enum class SimAnalysisPage {
		Settings, Breakdown, Ranking
	};

	struct TalentTreeData {
		Engine::TalentTree tree;

		bool isTreeSolveInProgress = false;
		bool isTreeSolveProcessed = false;
		bool isTreeSolveFiltered = false;
		bool safetyGuardTriggered = false;
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

	struct UIData {
		Presets::STYLES style = Presets::STYLES::COMPANY_GREY;
		EditorView editorView = EditorView::None;
		EditorView editorViewTarget = EditorView::None;

		//################# GENERAL #####################################
		int loadedIconTreeIndex = -1;
		HWND* hwnd;
		ID3D11Device* g_pd3dDevice;
		//This stores the base texture of the icon as well as its gray counterpart
		std::map<std::string, std::pair<TextureInfo, TextureInfo>> iconMap;
		std::map<int, std::pair<TextureInfo*, TextureInfo*>> iconIndexMap;
		std::map<int, std::pair<TextureInfo*, TextureInfo*>> iconIndexMapGrayed;
		TextureInfo defaultIcon;
		TextureInfo defaultIconGray;
		TextureInfo redIconGlow[3];
		TextureInfo greenIconGlow[3];
		TextureInfo goldIconGlow[3];
		TextureInfo blueIconGlow[3];
		TextureInfo purpleIconGlow[3];
		//contains 3 masks for each style (ACTIVE, PASSIVE, CHOICE) with the respective window background color
		std::array <std::array<TextureInfo, 3>, static_cast<int>(Presets::STYLES::STYLE_COUNT) > talentIconMasks;
		bool enableGlow = true;

		Presets::FONTSIZE fontsize = Presets::FONTSIZE::DEFAULT;

		std::string menuBarUpdateLabel = "";

		int deleteTreeIndex = -1;

		bool showAboutPopup = false;
		bool showHelpPopup = false;
		bool showChangelogPopup = false;
		bool showResetPopup = false;
		bool resetWorkspace = false;

		std::string talentSearchString = "";
		Engine::TalentVec searchedTalents;

		std::chrono::duration<long> autoSaveInterval = std::chrono::seconds(300);
		std::chrono::steady_clock::time_point lastSaveTime = std::chrono::high_resolution_clock::now();

		unsigned int leftWindowDockId = 0;
		unsigned int rightWindowDockId = 0;
		float dockWindowRatio = 0.0f;

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
		std::pair<TextureInfo*, TextureInfo*> treeEditorCreationTalentIcons;
		std::string treeEditorCreationIconNameFilter;
		std::vector<int> treeEditorCreationTalentParentsPlaceholder;
		std::vector<int> treeEditorCreationTalentChildrenPlaceholder;

		bool talentJustSelected = false;
		Engine::Talent_s treeEditorSelectedTalent = nullptr;
		std::pair<TextureInfo*, TextureInfo*> treeEditorSelectedTalentIcons;
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
		bool loadoutEditorExportActiveSkillsetSimcProfilesetCheckbox = false;
		std::string loadoutEditorExportActiveSkillsetSimcString;
		std::string loadoutEditorExportAllSkillsetsSimcString;
		std::string loadoutEditorImportSkillsetsString;
		std::pair<int, int> loadoutEditorImportSkillsetsResult;

		//############# LOADOUT SOLVER VARIABLES ########################
		const int maxConcurrentSolvers = 3;
		const std::chrono::duration<long long, std::milli> currentSolversUpdateInterval = std::chrono::milliseconds(500);
		std::chrono::steady_clock::time_point currentSolversLastUpdateTime = std::chrono::steady_clock::now();
		std::vector<std::pair<std::string, TalentTreeData*>> currentSolvers;
		std::vector<std::pair<std::string, TalentTreeData*>> solvedTrees;
		const int loadoutSolverMaxTalentPoints = 64;
		int loadoutSolverTalentPointLimit = 30;
		LoadoutSolverPage loadoutSolverPage = LoadoutSolverPage::SolutionResults;
		int loadoutSolverTalentPointSelection = -1;
		const int loadoutSolverResultsPerPage = 50;
		bool loadoutSolverAutoApplyFilter = false;
		//the selected/requested page of skillset results
		int loadoutSolverSkillsetResultPage = -1;
		std::string loadoutSolverSkillsetPrefix = "";
		int loadoutSolverAddRandomLoadoutCount = 1;
		const int loadoutSolverAddAllLimit = 5000;
		//the currently buffered results page which should differ from the selected/requested page only for 1 frame
		int loadoutSolverBufferedPage = -1;
		std::vector<Engine::SIND> loadoutSolverPageResults;
		int selectedFilteredSkillsetIndex = -1;
		Engine::SIND selectedFilteredSkillset = 0;
		std::shared_ptr<Engine::TalentSkillset> hoveredFilteredSkillset = nullptr;

		//############# SIM ANALYSIS VARIABLES ########################
		SimAnalysisPage simAnalysisPage = SimAnalysisPage::Settings;
		std::string raidbotsInputURL = "";
		int simAnalysisIconRatingSwitch = 0;
		int topMedianPerformanceSwitch = 0;
		int relativeDpsRankingSwitch = 0;
		int showLowestHighestSwitch = 0;
		std::map<int, ImVec4> simAnalysisTalentColor;
		std::map<int, std::string> simAnalysisButtonRankingText;
		int analysisTooltipLastTalentIndex = -1;
		int analysisTooltipTalentRank = -1;
		int analysisBreakdownTalentIndex = -1;
		std::shared_ptr<Engine::TalentSkillset> hoveredAnalysisSkillset = nullptr;
		int hoveredAnalysisSkillsetIndex = 0;
	};

	void refreshIconMap(UIData& uiData);
	void loadActiveIcons(UIData& uiData, TalentTreeCollection& talentTreeCollection, bool forceReload = false);
	//void loadIcon(UIData& uiData, int index, std::string iconName, ID3D11ShaderResourceView* defaultTexture, ID3D11ShaderResourceView* defaultTextureGray, int defaultImageWidth, int defaultImageHeight, bool first, Engine::TalentType talentType);
	//void loadSplitIcon(UIData& uiData, Engine::Talent_s talent, ID3D11ShaderResourceView* defaultTexture, ID3D11ShaderResourceView* defaultTextureGray, int defaultImageWidth, int defaultImageHeight);
	std::pair<TextureInfo, TextureInfo> loadTextureInfoFromFile(UIData& uiData, std::string path);

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

	void drawSkillsetPreviewShapeAroundTalent(
		Engine::Talent_s talent,
		int pointsSpent,
		ImDrawList* drawList,
		ImVec4* colors,
		ImVec2 pos,
		int talentSize,
		ImVec2 windowPos,
		ImVec2 scroll,
		UIData& uiData,
		TalentTreeCollection& talentTreeCollection,
		float disabledAlpha);

	void drawSimAnalysisShapeAroundTalent(
		Engine::Talent_s talent,
		ImDrawList* drawList,
		ImVec4* colors,
		ImVec2 pos,
		int talentSize,
		ImVec2 windowPos,
		ImVec2 scroll,
		UIData& uiData,
		TalentTreeCollection& talentTreeCollection,
		bool searchActive,
		bool talentIsSearchedFor);

	void drawSkillsetPreview(UIData& uiData, TalentTreeCollection& talentTreeCollection, std::shared_ptr<Engine::TalentSkillset> skillset);

	void updateSolverStatus(UIData& uiData, TalentTreeCollection& talentTreeCollection, bool forceUpdate = false);
	void stopAllSolvers(TalentTreeCollection& talentTreeCollection);
	void clearSolvingProcess(UIData& uiData, TalentTreeCollection& talentTreeCollection, bool onlyUIData = false);
	void clearSolvingProcess(UIData& uiData, TalentTreeData& talentTreeData);
	void clearSimAnalysisProcess(UIData& uiData, TalentTreeCollection& talentTreeCollection, bool onlyUIData = false);
	void AddWrappedText(std::string text, ImVec2 position, float padding, ImVec4 color, float maxWidth, float maxHeight, ImDrawList* draw_list);

	void HelperTooltip(std::string hovered, std::string helptext);
}