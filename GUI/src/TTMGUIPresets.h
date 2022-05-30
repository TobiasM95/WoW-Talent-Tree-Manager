#pragma once

#include <string>

#include "imgui.h"

namespace Presets {
    enum class STYLES {
        COMPANY_GREY, PATH_OF_TALENT_TREE, LIGHT_MODE
    };

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