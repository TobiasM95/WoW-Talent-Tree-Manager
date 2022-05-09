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




    
	
}