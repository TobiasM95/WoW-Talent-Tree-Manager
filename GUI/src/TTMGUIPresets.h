#pragma once

#include <string>

#include "imgui.h"

namespace Presets {
    enum class STYLES {
        COMPANY_GREY, PATH_OF_TALENT_TREE, LIGHT_MODE
    };

    const ImVec4 TALENT_SELECTED_BORDER_COLOR = ImVec4(0.85f, 0.15f, 0.15f, 1.0f);
    const ImVec4 TALENT_DEFAULT_BORDER_COLOR = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    const ImVec4 TALENT_DEFAULT_BORDER_COLOR_COMPANY_GREY = ImVec4(0.05f, 0.05f, 0.05f, 1.0f);
    const ImVec4 TALENT_MAXED_BORDER_COLOR = ImVec4(0.8f, 0.63f, 0.0f, 1.0f);
    const ImVec4 TALENT_PARTIAL_BORDER_COLOR = ImVec4(0.1f, 0.8f, 0.1f, 1.0f);
    const ImVec4 TALENT_SEARCHED_BORDER_COLOR = ImVec4(0.0f, 0.73f, 1.0f, 1.0f);
    const ImVec4 TALENT_PURPLE_BORDER_COLOR = ImVec4(0.73f, 0.0f, 1.0f, 1.0f);

    const double PASTEBIN_EXPORT_COOLDOWN = 60.0;

    //GUI/STYLE PRESETS
    void SET_GUI_STYLE(STYLES style);
    void SET_GUI_STYLE_COMPANY_GREY();
    void SET_GUI_STYLE_PATH_OF_TALENT_TREE();
    void SET_GUI_STYLE_LIGHT_MODE();

    void SET_STATUS_BAR_COLOR(STYLES style, std::string presetName);
    void RESET_STATUS_BAR_COLOR();

    void SET_TAB_ITEM_COLOR(STYLES style, std::string presetName);
    void RESET_TAB_ITEM_COLOR();
    
    ImVec4 GET_TOOLTIP_TALENT_TYPE_COLOR(STYLES style);
    ImVec4 GET_TOOLTIP_TALENT_DESC_COLOR(STYLES style);
}