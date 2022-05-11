#pragma once

#include "TalentTrees.h"
#include "TalentTreeManagerDefinitions.h"

namespace TTM {
	static void AttachLoadoutSolverTooltip(Engine::Talent talent);
	void RenderLoadoutSolverWindow(UIData& uiData, TalentTreeCollection& talentTreeCollection);
	void placeLoadoutSolverTreeElements(UIData& uiData, TalentTreeCollection& talentTreeCollection);

	
}