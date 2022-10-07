#pragma once

#include "TalentTrees.h"
#include "TalentTreeManagerDefinitions.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"

namespace TTM {
	bool OptionSwitch(
		std::string leftText,
		std::string rightText,
		int* switchVariable,
		float switchWidth,
		bool centered,
		std::string uniqueSliderId,
		bool showHelperTooltip,
		std::string helperTooltipHovered,
		std::string helperTooltipHelptext);
	void PrintColoredRatio(float v1, float v2, float min, float max, int colorMode, std::string formatText);
	static void HelperTooltip(std::string hovered, std::string helptext);
	static void AttachSimAnalysisTooltip(UIData& uiData, const Engine::AnalysisResult& result, Engine::Talent_s talent);

	void RenderSimAnalysisWindow(UIData& uiData, TalentTreeCollection& talentTreeCollection);


	void placeSimAnalysisTreeElements(UIData& uiData, TalentTreeCollection& talentTreeCollection);

	void AnalyzeRawResults(Engine::TalentTree& tree);
	void CalculateAnalysisRankings(UIData& uiData, Engine::AnalysisResult& result);
	void UpdateColorGlowTextures(UIData& uiData, TalentTreeCollection& talentTreeCollection, Engine::AnalysisResult& result);

	//taken from: https://stackoverflow.com/questions/17074324/how-can-i-sort-two-vectors-in-the-same-way-with-criteria-that-uses-only-one-of
	template <typename T, typename Compare>
	std::vector<std::size_t> sort_permutation(
		const std::vector<T>& vec,
		Compare compare);
	template <typename T>
	std::vector<T> apply_permutation(
		const std::vector<T>& vec,
		const std::vector<std::size_t>& p);

}