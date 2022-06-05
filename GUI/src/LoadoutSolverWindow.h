#pragma once

#include "TalentTrees.h"
#include "TalentTreeManagerDefinitions.h"

namespace TTM {
	static void AttachLoadoutSolverTooltip(const UIData& uiData, Engine::Talent_s talent, int assignedPointsTarget);
	void RenderLoadoutSolverWindow(UIData& uiData, TalentTreeCollection& talentTreeCollection);
	void placeLoadoutSolverTreeElements(UIData& uiData, TalentTreeCollection& talentTreeCollection);
	void displayFilteredSkillsetSelector(UIData& uiData, TalentTreeCollection& talentTreeCollection);
	int getResultsPage(UIData& uiData, TalentTreeCollection& talentTreeCollection, int pageNumber);
	
}