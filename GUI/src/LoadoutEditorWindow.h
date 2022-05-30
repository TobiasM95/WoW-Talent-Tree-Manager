#pragma once

#include "TalentTrees.h"
#include "TalentTreeManagerDefinitions.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"

namespace TTM {
	static void AttachLoadoutEditTooltip(const UIData& uiData, Engine::Talent_s talent);

	void RenderLoadoutEditorWindow(UIData& uiData, TalentTreeCollection& talentTreeCollection);


	void placeLoadoutEditorTreeElements(UIData& uiData, TalentTreeCollection& talentTreeCollection);
}