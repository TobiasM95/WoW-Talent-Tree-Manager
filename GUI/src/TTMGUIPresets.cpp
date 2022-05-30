/*
	WoW Talent Tree Manager is an application for creating/editing/sharing talent trees and setups.
	Copyright(C) 2022 TobiasM95

	This program is free software : you can redistribute it and /or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see < https://www.gnu.org/licenses/>.

	Contact via https://github.com/TobiasM95/WoW-Talent-Tree-Manager/discussions or BuffMePls#2973 on Discord
*/

#include "TTMGUIPresets.h"

namespace Presets {

	void SET_GUI_STYLE(STYLES style) {
		switch (style) {
		case STYLES::COMPANY_GREY: {SET_GUI_STYLE_COMPANY_GREY(); }break;
		case STYLES::PATH_OF_TALENT_TREE: {SET_GUI_STYLE_PATH_OF_TALENT_TREE(); }break;
		case STYLES::LIGHT_MODE: {SET_GUI_STYLE_LIGHT_MODE(); }break;
		}
	}

	void SET_GUI_STYLE_COMPANY_GREY() {
		//ImGui::StyleColorsClassic();
		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		style.FrameRounding = 3.0f;
		style.GrabRounding = 3.0f;
		style.WindowBorderSize = 1.0f;
		style.FrameBorderSize = 1.0f;
		style.TabBorderSize = 1.0f;

		ImVec4* colors = style.Colors;

		colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		colors[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.12f, 0.71f);
		colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.42f, 0.42f, 0.42f, 0.54f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.67f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.335f, 0.335f, 0.335f, 1.000f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.24f, 0.24f, 0.24f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.54f, 0.54f, 0.54f, 0.35f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.52f, 0.52f, 0.52f, 0.59f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.76f, 0.76f, 0.76f, 0.77f);
		colors[ImGuiCol_Separator] = ImVec4(0.000f, 0.000f, 0.000f, 0.137f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.700f, 0.671f, 0.600f, 0.290f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.702f, 0.671f, 0.600f, 0.674f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);

		colors[ImGuiCol_Tab] = ImVec4(0.54f, 0.54f, 0.54f, 0.35f);
		colors[ImGuiCol_TabActive] = ImVec4(0.74f, 0.74f, 0.74f, 0.35f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.94f, 0.94f, 0.94f, 0.35f);
		//colors[ImGuiCol_TabUnfocused] = ImVec4();
		//colors[ImGuiCol_TabUnfocusedActive] = ImVec4();
	}

	void SET_GUI_STYLE_PATH_OF_TALENT_TREE() {
		//ImGui::StyleColorsClassic();
		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		style.FrameRounding = 0.0f;
		style.GrabRounding = 0.0f;
		style.WindowBorderSize = 1.0f;
		style.FrameBorderSize = 1.0f;
		style.TabBorderSize = 1.0f;

		ImVec4* colors = style.Colors;

		colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		colors[ImGuiCol_Border] = ImVec4(0.82f, 0.82f, 0.82f, 0.91f);
		colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.54f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.67f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.135f, 0.135f, 0.135f, 1.000f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.42f, 0.42f, 0.42f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.66f, 0.66f, 0.66f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.44f, 0.44f, 0.44f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.24f, 0.24f, 0.24f, 0.35f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.22f, 0.22f, 0.22f, 0.59f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.46f, 0.46f, 0.46f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.77f);
		colors[ImGuiCol_Separator] = ImVec4(0.500f, 0.500f, 0.500f, 0.937f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.700f, 0.671f, 0.600f, 0.290f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.702f, 0.671f, 0.600f, 0.674f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.30f, 0.30f, 0.30f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);

		colors[ImGuiCol_Tab] = ImVec4(0.24f, 0.24f, 0.24f, 0.35f);
		colors[ImGuiCol_TabActive] = ImVec4(0.54f, 0.54f, 0.54f, 0.65f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.64f, 0.64f, 0.64f, 0.65f);
		//colors[ImGuiCol_TabUnfocused] = ImVec4();
		//colors[ImGuiCol_TabUnfocusedActive] = ImVec4();
	}

	void SET_GUI_STYLE_LIGHT_MODE() {
		//ImGui::StyleColorsClassic();
		//ImGui::StyleColorsDark();
		ImGui::StyleColorsLight();

		ImGuiStyle& style = ImGui::GetStyle();
		style.FrameRounding = 3.0f;
		style.GrabRounding = 3.0f;
		style.WindowBorderSize = 1.0f;
		style.FrameBorderSize = 1.0f;
		style.TabBorderSize = 1.0f;

		ImVec4* colors = style.Colors;

		colors[ImGuiCol_Button] = ImVec4(0.74f, 0.74f, 0.74f, 0.35f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.74f, 0.74f, 0.74f, 0.59f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.76f, 0.76f, 0.76f, 0.45f);
		/*colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		colors[ImGuiCol_Border] = ImVec4(0.82f, 0.82f, 0.82f, 0.91f);
		colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.54f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.67f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.135f, 0.135f, 0.135f, 1.000f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.42f, 0.42f, 0.42f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.66f, 0.66f, 0.66f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.44f, 0.44f, 0.44f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.77f);
		colors[ImGuiCol_Separator] = ImVec4(0.000f, 0.000f, 0.000f, 0.137f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.700f, 0.671f, 0.600f, 0.290f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.702f, 0.671f, 0.600f, 0.674f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);

		colors[ImGuiCol_Tab] = ImVec4(0.24f, 0.24f, 0.24f, 0.35f);
		colors[ImGuiCol_TabActive] = ImVec4(0.44f, 0.44f, 0.44f, 0.35f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.64f, 0.64f, 0.64f, 0.35f);*/
		//colors[ImGuiCol_TabUnfocused] = ImVec4();
		//colors[ImGuiCol_TabUnfocusedActive] = ImVec4();
	}

	/*
	Don't forget to call RESET_STATS_BAR_COLORS after calling this function and rendering the status bar!
	*/
	void SET_STATUS_BAR_COLOR(STYLES style, std::string presetName) {
		if (presetName.find("deathknight_") != std::string::npos) {
			ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.77f, 0.12f, 0.23f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
			return;
		}
		if (presetName.find("demonhunter_") != std::string::npos) {
			ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.64f, 0.19f, 0.79f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
			return;
		}
		if (presetName.find("druid_") != std::string::npos) {
			ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(1.f, 0.49f, 0.04f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));
			return;
		}
		if (presetName.find("evoker_") != std::string::npos) {
			ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.2f, 0.58f, 0.5f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));
			return;
		}
		if (presetName.find("hunter_") != std::string::npos) {
			ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.67f, 0.83f, 0.45f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));
			return;
		}
		if (presetName.find("mage_") != std::string::npos) {
			ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.25f, 0.78f, 0.92f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));
			return;
		}
		if (presetName.find("monk_") != std::string::npos) {
			ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.0f, 1.f, 0.6f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));
			return;
		}
		if (presetName.find("paladin_") != std::string::npos) {
			ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.96f, 0.55f, 0.73f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));
			return;
		}
		if (presetName.find("priest_") != std::string::npos) {
			ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(1.f, 1.f, 1.f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));
			return;
		}
		if (presetName.find("rogue_") != std::string::npos) {
			ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(1.f, 0.96f, 0.41f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));
			return;
		}
		if (presetName.find("shaman_") != std::string::npos) {
			ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.f, 0.44f, 0.87f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
			return;
		}
		if (presetName.find("warlock_") != std::string::npos) {
			ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.53f, 0.53f, 0.93f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));
			return;
		}
		if (presetName.find("warrior_") != std::string::npos) {
			ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.78f, 0.61f, 0.43f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));
			return;
		}

		ImGuiStyle& imStyle = ImGui::GetStyle();
		ImVec4* colors = imStyle.Colors;
		ImGui::PushStyleColor(ImGuiCol_MenuBarBg, colors[ImGuiCol_MenuBarBg]);
		ImGui::PushStyleColor(ImGuiCol_Text, colors[ImGuiCol_Text]);
	}

	void RESET_STATUS_BAR_COLOR() {
		ImGui::PopStyleColor(2);
	}

	/*
	Don't forget to call RESET_STATS_BAR_COLORS after calling this function and rendering the status bar!
	*/
	void SET_TAB_ITEM_COLOR(STYLES style, std::string presetName) {
		float regularDelta = -0.05f;
		float activeDelta = 0.0f;
		float hoveredDelta = 0.2f;
		float regularAlpha = 0.85f;
		ImVec4 c;
		if (presetName.find("deathknight_") != std::string::npos) {
			if (style == STYLES::LIGHT_MODE) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
			}
			c = ImVec4(0.77f, 0.12f, 0.23f, 1.f);
			ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(c.x + regularDelta, c.y + regularDelta, c.z + regularDelta, regularAlpha));
			ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(c.x + activeDelta, c.y + activeDelta, c.z + activeDelta, 1.f));
			ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(c.x + hoveredDelta, c.y + hoveredDelta, c.z + hoveredDelta, 1.f));
			return;
		}
		if (presetName.find("demonhunter_") != std::string::npos) {
			if (style == STYLES::LIGHT_MODE) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
			}
			c = ImVec4(0.64f, 0.19f, 0.79f, 1.f);
			ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(c.x + regularDelta, c.y + regularDelta, c.z + regularDelta, regularAlpha));
			ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(c.x + activeDelta, c.y + activeDelta, c.z + activeDelta, 1.f));
			ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(c.x + hoveredDelta, c.y + hoveredDelta, c.z + hoveredDelta, 1.f));
			return;
		}
		if (presetName.find("druid_") != std::string::npos) {
			if (style == STYLES::LIGHT_MODE) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
			}
			c = ImVec4(1.f, 0.49f, 0.04f, 1.f);
			ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(c.x + regularDelta, c.y + regularDelta, c.z + regularDelta, regularAlpha));
			ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(c.x + activeDelta, c.y + activeDelta, c.z + activeDelta, 1.f));
			ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(c.x + hoveredDelta, c.y + hoveredDelta, c.z + hoveredDelta, 1.f));
			return;
		}
		if (presetName.find("evoker_") != std::string::npos) {
			if (style == STYLES::LIGHT_MODE) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
			}
			c = ImVec4(0.2f, 0.58f, 0.5f, 1.f);
			ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(c.x + regularDelta, c.y + regularDelta, c.z + regularDelta, regularAlpha));
			ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(c.x + activeDelta, c.y + activeDelta, c.z + activeDelta, 1.f));
			ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(c.x + hoveredDelta, c.y + hoveredDelta, c.z + hoveredDelta, 1.f));
			return;
		}
		if (presetName.find("hunter_") != std::string::npos) {
			if (style == STYLES::LIGHT_MODE) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));
			}
			c = ImVec4(0.67f, 0.83f, 0.45f, 1.f);
			ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(c.x + regularDelta, c.y + regularDelta, c.z + regularDelta, regularAlpha));
			ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(c.x + activeDelta, c.y + activeDelta, c.z + activeDelta, 1.f));
			ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(c.x + hoveredDelta, c.y + hoveredDelta, c.z + hoveredDelta, 1.f));
			return;
		}
		if (presetName.find("mage_") != std::string::npos) {
			if (style == STYLES::LIGHT_MODE) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));
			}
			c = ImVec4(0.25f, 0.78f, 0.92f, 1.f);
			ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(c.x + regularDelta, c.y + regularDelta, c.z + regularDelta, regularAlpha));
			ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(c.x + activeDelta, c.y + activeDelta, c.z + activeDelta, 1.f));
			ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(c.x + hoveredDelta, c.y + hoveredDelta, c.z + hoveredDelta, 1.f));
			return;
		}
		if (presetName.find("monk_") != std::string::npos) {
			if (style == STYLES::LIGHT_MODE) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));
			}
			c = ImVec4(0.0f, 1.f, 0.6f, 1.f);
			ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(c.x + regularDelta, c.y + regularDelta, c.z + regularDelta, regularAlpha));
			ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(c.x + activeDelta, c.y + activeDelta, c.z + activeDelta, 1.f));
			ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(c.x + hoveredDelta, c.y + hoveredDelta, c.z + hoveredDelta, 1.f));
			return;
		}
		if (presetName.find("paladin_") != std::string::npos) {
			if (style == STYLES::LIGHT_MODE) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));
			}
			c = ImVec4(0.96f, 0.55f, 0.73f, 1.f);
			ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(c.x + regularDelta, c.y + regularDelta, c.z + regularDelta, regularAlpha));
			ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(c.x + activeDelta, c.y + activeDelta, c.z + activeDelta, 1.f));
			ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(c.x + hoveredDelta, c.y + hoveredDelta, c.z + hoveredDelta, 1.f));
			return;
		}
		if (presetName.find("priest_") != std::string::npos) {
			if (style == STYLES::LIGHT_MODE) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));
			}
			c = ImVec4(1.f, 1.f, 1.f, 1.f);
			ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(c.x + regularDelta, c.y + regularDelta, c.z + regularDelta, regularAlpha));
			ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(c.x + activeDelta, c.y + activeDelta, c.z + activeDelta, 1.f));
			ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(c.x + hoveredDelta, c.y + hoveredDelta, c.z + hoveredDelta, 1.f));
			return;
		}
		if (presetName.find("rogue_") != std::string::npos) {
			if (style == STYLES::LIGHT_MODE) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));
			}
			c = ImVec4(1.f, 0.96f, 0.41f, 1.f);
			ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(c.x + regularDelta, c.y + regularDelta, c.z + regularDelta, regularAlpha));
			ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(c.x + activeDelta, c.y + activeDelta, c.z + activeDelta, 1.f));
			ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(c.x + hoveredDelta, c.y + hoveredDelta, c.z + hoveredDelta, 1.f));
			return;
		}
		if (presetName.find("shaman_") != std::string::npos) {
			if (style == STYLES::LIGHT_MODE) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
			}
			c = ImVec4(0.f, 0.44f, 0.87f, 1.f);
			ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(c.x + regularDelta, c.y + regularDelta, c.z + regularDelta, regularAlpha));
			ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(c.x + activeDelta, c.y + activeDelta, c.z + activeDelta, 1.f));
			ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(c.x + hoveredDelta, c.y + hoveredDelta, c.z + hoveredDelta, 1.f));
			return;
		}
		if (presetName.find("warlock_") != std::string::npos) {
			if (style == STYLES::LIGHT_MODE) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
			}
			c = ImVec4(0.53f, 0.53f, 0.93f, 1.f);
			ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(c.x + regularDelta, c.y + regularDelta, c.z + regularDelta, regularAlpha));
			ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(c.x + activeDelta, c.y + activeDelta, c.z + activeDelta, 1.f));
			ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(c.x + hoveredDelta, c.y + hoveredDelta, c.z + hoveredDelta, 1.f));
			return;
		}
		if (presetName.find("warrior_") != std::string::npos) {
			if (style == STYLES::LIGHT_MODE) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));
			}
			c = ImVec4(0.78f, 0.61f, 0.43f, 1.f);
			ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(c.x + regularDelta, c.y + regularDelta, c.z + regularDelta, regularAlpha));
			ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(c.x + activeDelta, c.y + activeDelta, c.z + activeDelta, 1.f));
			ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(c.x + hoveredDelta, c.y + hoveredDelta, c.z + hoveredDelta, 1.f));
			return;
		}

		ImGuiStyle& imStyle = ImGui::GetStyle();
		ImVec4* colors = imStyle.Colors;
		ImGui::PushStyleColor(ImGuiCol_Text, colors[ImGuiCol_Text]);
		ImGui::PushStyleColor(ImGuiCol_Tab, colors[ImGuiCol_Tab]);
		ImGui::PushStyleColor(ImGuiCol_TabActive, colors[ImGuiCol_TabActive]);
		ImGui::PushStyleColor(ImGuiCol_TabHovered, colors[ImGuiCol_TabHovered]);
	}

	void RESET_TAB_ITEM_COLOR() {
		ImGui::PopStyleColor(4);
	}

	ImVec4 GET_TOOLTIP_TALENT_TYPE_COLOR(STYLES style) {
		if (style == STYLES::LIGHT_MODE) {
			return ImVec4(0.92f, 0.24f, 0.24f, 1.0f);
		}
		else {
			return ImVec4(0.92f, 0.44f, 0.44f, 1.0f);
		}
	}
	ImVec4 GET_TOOLTIP_TALENT_DESC_COLOR(STYLES style) {
		if (style == STYLES::LIGHT_MODE) {
			return ImVec4(0.533f, 0.533f, 1.0f, 1.0f);
		}
		else {
			return ImVec4(0.633f, 0.633f, 1.0f, 1.0f);
		}
	}
}