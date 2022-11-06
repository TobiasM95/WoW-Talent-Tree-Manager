/*
    WoW Talent Tree Manager is an application for creating/editing/sharing talent trees and setups.
    Copyright(C) 2022 Tobias Mielich

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

#include "TalentTreeManager.h"

#include <fstream>
#include <algorithm>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"

#include "TTMGUIPresets.h"
#include "TTMEnginePresets.h"

#include "Updater.h"
#include "ImageHandler.h"
#include "TalentTreeEditorWindow.h"
#include "LoadoutEditorWindow.h"
#include "LoadoutSolverWindow.h"
#include "SimAnalysisWindow.h"

//TTMTODO: Replace all talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree with talentTreeCollection.activeTree()
namespace TTM {
    
    void RenderUpdateWindow(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        if (uiData.renderedOnce && uiData.updateStatus == UpdateStatus::NOTCHECKED) {
            uiData.updateMessage = checkForUpdate(uiData);
        }
        if (uiData.updateStatus == UpdateStatus::UPTODATE) {
            return;
        }
        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoSavedSettings;

        // We demonstrate using the full viewport area or the work area (without menu-bars, task-bars etc.)
        // Based on your use case you may want one of the other.
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        if (ImGui::Begin("MainWindow", nullptr, flags))
        {
            ImGui::PopStyleVar();

            if (uiData.updateStatus == UpdateStatus::NOTCHECKED) {
                ImVec2 contentRegion = ImGui::GetContentRegionAvail();
                ImVec2 textSize = ImGui::CalcTextSize("Looking for updates...");
                ImGui::SetCursorPos(ImVec2(0.5f * contentRegion.x - 0.5f * textSize.x, 0.5f * contentRegion.y - 0.5f * textSize.y));
                ImGui::Text("Looking for updates...");
            }
            else if (uiData.updateStatus == UpdateStatus::UPDATEERROR) {
                uiData.menuBarUpdateLabel = "Check for updates failed. Presets might be outdated!";
            }
            else if (uiData.updateStatus == UpdateStatus::OUTDATED) {
                //center an information rectangle
                ImVec2 contentRegion = ImGui::GetContentRegionAvail();
                float centerX = 0.5f * contentRegion.x;
                float centerY = 0.5f * contentRegion.y;
                float wrapWidth = 250;
                float offsetY = -100;
                float boxPadding = 8;
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImVec2 pos = ImVec2(centerX - 0.5f * wrapWidth, centerY + offsetY);
                ImGui::SetCursorPos(pos);
                ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + wrapWidth);
                ImGui::Text("New update was detected. Do you want to update? (This includes presets, images, etc.)");
                ImVec2 l1TopLeft = ImGui::GetItemRectMin();
                ImVec2 l1BottomRight = ImGui::GetItemRectMax();
                l1TopLeft.x -= boxPadding;
                l1TopLeft.y -= boxPadding;
                l1BottomRight.x += boxPadding;
                l1BottomRight.y += boxPadding;
                ImGui::Spacing();
                ImGui::SetCursorPosX(pos.x);
                ImGui::TextUnformattedColored(Presets::GET_TOOLTIP_TALENT_TYPE_COLOR(uiData.style), uiData.updateMessage.c_str());

                ImGui::Spacing();
                ImGui::Spacing();

                //ask for solver max talent points
                ImGui::SetCursorPosX(pos.x);
                ImGui::Text("Do you want to update the presets in your current workspace? This could invalidate and remove some of your loadouts.");
                ImGui::SetCursorPosX(pos.x);
                ImGui::Checkbox("Update current workspace##updateWindowWorkspace", &uiData.updateCurrentWorkspace);

                ImVec2 l2BottomRight = ImGui::GetItemRectMax();
                l2BottomRight.x += boxPadding;
                l2BottomRight.y += boxPadding;

                // Draw actual text bounding box, following by marker of our expected limit (should not overlap!)
                draw_list->AddRect(l1TopLeft, ImVec2(l1TopLeft.x + wrapWidth + 2 * boxPadding, l2BottomRight.y), IM_COL32(200, 200, 200, 255));
                ImGui::PopTextWrapPos();

                //center a button
                ImGui::SetCursorPos(ImVec2(centerX - 0.5f * wrapWidth - boxPadding, ImGui::GetCursorPosY() + boxPadding));
                if (ImGui::Button("Update", ImVec2(0.5f * wrapWidth + boxPadding - 4.0f, 25))) {
                    uiData.updateStatus = UpdateStatus::UPDATEINITIATED;
                }
                ImGui::SameLine();
                if (ImGui::Button("Ignore", ImVec2(0.5f * wrapWidth + boxPadding - 4.0f, 25))) {
                    uiData.updateStatus = UpdateStatus::IGNOREUPDATE;
                }
            }
            else if (uiData.updateStatus == UpdateStatus::RESETRESOURCES) {
                //center an information rectangle
                ImVec2 contentRegion = ImGui::GetContentRegionAvail();
                float centerX = 0.5f * contentRegion.x;
                float centerY = 0.5f * contentRegion.y;
                float wrapWidth = 250;
                float offsetY = -100;
                float boxPadding = 8;
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImVec2 pos = ImVec2(centerX - 0.5f * wrapWidth, centerY + offsetY);
                ImGui::SetCursorPos(pos);
                ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + wrapWidth);
                ImGui::Text("Do you want to reset your resources? This has the same effect as an update would have and can help fix corrupted files.");
                ImVec2 l1TopLeft = ImGui::GetItemRectMin();
                ImVec2 l1BottomRight = ImGui::GetItemRectMax();
                l1TopLeft.x -= boxPadding;
                l1TopLeft.y -= boxPadding;
                l1BottomRight.x += boxPadding;
                l1BottomRight.y += boxPadding;

                ImGui::Spacing();
                ImGui::Spacing();

                //ask for solver max talent points
                ImGui::SetCursorPosX(pos.x);
                ImGui::Text("Do you want to update the presets in your current workspace? This could invalidate and remove some of your loadouts.");
                ImGui::SetCursorPosX(pos.x);
                ImGui::Checkbox("Update current workspace##updateWindowWorkspace", &uiData.updateCurrentWorkspace);

                ImVec2 l2BottomRight = ImGui::GetItemRectMax();
                l2BottomRight.x += boxPadding;
                l2BottomRight.y += boxPadding;

                // Draw actual text bounding box, following by marker of our expected limit (should not overlap!)
                draw_list->AddRect(l1TopLeft, ImVec2(l1TopLeft.x + wrapWidth + 2 * boxPadding, l2BottomRight.y), IM_COL32(200, 200, 200, 255));
                ImGui::PopTextWrapPos();

                //center a button
                ImGui::SetCursorPos(ImVec2(centerX - 0.5f * wrapWidth - boxPadding, ImGui::GetCursorPosY() + boxPadding));
                if (ImGui::Button("Update", ImVec2(0.5f * wrapWidth + boxPadding - 4.0f, 25))) {
                    flagAllResources(uiData);
                    uiData.updateStatus = UpdateStatus::UPDATEINITIATED;
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(0.5f * wrapWidth + boxPadding - 4.0f, 25))) {
                    uiData.updateStatus = UpdateStatus::UPTODATE;
                }
            }
            else if (uiData.updateStatus == UpdateStatus::UPDATEINITIATED) {
                ImVec2 textSize = ImGui::CalcTextSize("Updating...");
                ImVec2 contentRegion = ImGui::GetContentRegionAvail();
                ImGui::SetCursorPos(ImVec2(0.5f * contentRegion.x - 0.5f * textSize.x, 0.5f * contentRegion.y - 0.5f * textSize.y));
                ImGui::Text("Updating...");
                uiData.updateStatus = UpdateStatus::UPDATEINPROGRESS;
            }
            else if (uiData.updateStatus == UpdateStatus::UPDATEINPROGRESS) {
                updateResources(uiData, talentTreeCollection);
                loadActiveIcons(uiData, talentTreeCollection, true);
                if (talentTreeCollection.activeTreeIndex >= 0 && talentTreeCollection.activeTreeIndex < talentTreeCollection.trees.size()
                    && talentTreeCollection.activeTree().loadout.size() > 0) {
                    Engine::activateSkillset(talentTreeCollection.activeTree(), talentTreeCollection.activeTree().activeSkillsetIndex);
                }
            }
        }
        ImGui::End();
        uiData.renderedOnce = true;
    }

	void RenderMainWindow(UIData& uiData, TalentTreeCollection& talentTreeCollection, bool& done) {

		RenderMenuBar(uiData, talentTreeCollection, done);
        RenderStatusBar(uiData, talentTreeCollection);
        RenderWorkArea(uiData, talentTreeCollection);
        //ImGui::ShowMetricsWindow(nullptr);
	}

	void RenderMenuBar(UIData& uiData, TalentTreeCollection& talentTreeCollection, bool& done) {
		if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Save", "Ctrl+S")) {
                    saveWorkspace(uiData, talentTreeCollection);
                }
                if (ImGui::MenuItem("Close")) {
                    saveWorkspace(uiData, talentTreeCollection);
                    done = true;
                }
                ImGui::EndMenu();
            }
            if ((ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)) && ImGui::IsKeyPressed(ImGuiKey_S)) {
                saveWorkspace(uiData, talentTreeCollection);
            }
			if (ImGui::BeginMenu("Styles")) {
                if (ImGui::MenuItem("Company Grey")) {
                    Presets::SET_GUI_STYLE(Presets::STYLES::COMPANY_GREY);
                    uiData.style = Presets::STYLES::COMPANY_GREY;
                }
                if (ImGui::MenuItem("Path of Talent Tree")) {
                    Presets::SET_GUI_STYLE(Presets::STYLES::PATH_OF_TALENT_TREE);
                    uiData.style = Presets::STYLES::PATH_OF_TALENT_TREE;
                }
                if (ImGui::MenuItem("Light Mode")) {
                    Presets::SET_GUI_STYLE(Presets::STYLES::LIGHT_MODE);
                    uiData.style = Presets::STYLES::LIGHT_MODE;
                }
                ImGui::Separator();
                ImGui::MenuItem("Show icon glow", NULL, &uiData.enableGlow);
                ImGui::Separator();
                bool sel = uiData.fontsize == Presets::FONTSIZE::MINI;
                ImGui::MenuItem("Font mini", NULL, &sel);
                if (sel) {
                    uiData.fontsize = Presets::FONTSIZE::MINI;
                }
                sel = uiData.fontsize == Presets::FONTSIZE::SMALL;
                ImGui::MenuItem("Font small", NULL, &sel);
                if (sel) {
                    uiData.fontsize = Presets::FONTSIZE::SMALL;
                }
                sel = uiData.fontsize == Presets::FONTSIZE::DEFAULT;
                ImGui::MenuItem("Font default", NULL, &sel);
                if (sel) {
                    uiData.fontsize = Presets::FONTSIZE::DEFAULT;
                }
                sel = uiData.fontsize == Presets::FONTSIZE::LARGE;
                ImGui::MenuItem("Font large", NULL, &sel);
                if (sel) {
                    uiData.fontsize = Presets::FONTSIZE::LARGE;
                }
                sel = uiData.fontsize == Presets::FONTSIZE::HUGE;
                ImGui::MenuItem("Font huge", NULL, &sel);
                if (sel) {
                    uiData.fontsize = Presets::FONTSIZE::HUGE;
                }
				ImGui::EndMenu();
			}
            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("Controls & Tips")) {
                    uiData.showHelpPopup = true;
                }
                if (ImGui::MenuItem("About TTM")) {
                    uiData.showAboutPopup = true;
                }
                if (ImGui::MenuItem("Visit Github")) {
                    ShellExecute(0, 0, L"https://github.com/TobiasM95/WoW-Talent-Tree-Manager", 0, 0, SW_SHOW);
                }
                if (ImGui::MenuItem("Check Updates")) {
                    uiData.updateStatus = UpdateStatus::NOTCHECKED;
                    uiData.renderedOnce = false;
                }
                if (ImGui::MenuItem("Show Changelog")) {
                    uiData.showChangelogPopup = true;
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Reset TTM")) {
                    uiData.showResetPopup = true;
                }
                if (ImGui::MenuItem("Reset resources")) {
                    uiData.updateStatus = UpdateStatus::RESETRESOURCES;
                }
                ImGui::EndMenu();
            }
            if (uiData.menuBarUpdateLabel.size() > 0) {
                ImGui::PushStyleColor(ImGuiCol_Text, Presets::GET_TOOLTIP_TALENT_TYPE_COLOR(uiData.style));
                //TTMTODO: very ugly way of doing this, should be checked in a variable instead
                if (uiData.menuBarUpdateLabel.find("New TTM update found") != std::string::npos) {
                    if (ImGui::MenuItem("> Press here to update <")) {
                        ShellExecuteW(NULL,
                            L"runas",
                            L".\\AppUpdater.exe",
                            NULL,
                            NULL,   // default dir 
                            SW_SHOWNORMAL
                        );
                    }
                }
                ImGui::PopStyleColor();
                ImGui::TextUnformattedColored(Presets::GET_TOOLTIP_TALENT_TYPE_COLOR(uiData.style), uiData.menuBarUpdateLabel.c_str());
            }
			ImGui::EndMainMenuBar();
        }
        if (uiData.showAboutPopup) {
            uiData.showAboutPopup = false;
            ImGui::OpenPopup("About##Popup");
        }
        if (uiData.showHelpPopup) {
            uiData.showHelpPopup = false;
            ImGui::OpenPopup("Controls & Tips##Popup");
        }
        if (uiData.showChangelogPopup) {
            uiData.showChangelogPopup = false;
            ImGui::OpenPopup("Changelog##Popup");
        }
        if (uiData.showResetPopup) {
            uiData.showResetPopup = false;
            ImGui::OpenPopup("Reset TTM##Popup");
        }
        static bool* close = new bool();
        *close = true;
        if (ImGui::BeginPopupModal("About##Popup", close, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("TTM Version: %s", Presets::TTM_VERSION.c_str());
            ImGui::Text("Feedback and suggestions: BuffMePls#2973 (Discord)");
            ImGui::Text("Github: https://github.com/TobiasM95/WoW-Talent-Tree-Manager");
            ImGui::Text("Dear ImGui: https://github.com/ocornut/imgui");
            ImGui::Text("libcurl: https://curl.se/libcurl/");
            ImGui::Text("stb: https://github.com/nothings/stb");
            ImGui::Text("miniz: https://github.com/richgel999/miniz");

            ImGui::SetItemDefaultFocus();
            if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }
        if (ImGui::BeginPopupModal("Controls & Tips##Popup", close, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::PushTextWrapPos(1000);
            Presets::PUSH_FONT(uiData.fontsize, 2);
            ImGui::Spacing();
            ImGui::Text("Controls");
            Presets::POP_FONT();
            Presets::PUSH_FONT(uiData.fontsize, 1);
            ImGui::Text("Tree editor");
            Presets::POP_FONT();
            ImGui::Separator();
            ImGui::Bullet();
            ImGui::Text("The talent tree editor allows you to create your own custom trees or edit one of the included specialization presets. You can edit the tree name, the description, add/edit/delete nodes as well as importing and exporting trees. If you just want to check out a spec preset and create a loadout then you don't need to use the talent tree editor much. Feel free to skip ahead to the loadout editor or solver.");
            ImGui::Bullet();
            ImGui::Text("On the left side you see the tree in its full form with all talents and their connections. If the tree exceeds the window border you can drag the screen around with the left mouse button, zoom in/out with Ctrl+mouse wheel, selecting a talent by clicking on it and dragging a talent around with the left mouse button as well. Additionally if you have a talent selected, you can create/remove connections by pressing left Alt on another talent to make that its child and Shift+left Alt to make that its parent."); 
            ImGui::Bullet();
            ImGui::Text("To create a tree, press the \"+\" button below the menu bar. You can then select between creating a new tree and loading a specialization preset. This will open the tree in a new tab, so your current tree doesn't get discarded.");
            ImGui::Bullet();
            ImGui::Text("In the \"Tree Information\" page you can change the name of the tree and add a description for the tree itself (there is a separate loadout description for all of your skillsets). You can also check what preset is currently active, how many nodes there are and how many talent points are spendable in this tree.");
            ImGui::Bullet();
            ImGui::Text("In the \"Tree Editor\" page you can create a node by expanding the \"Create Node\" header, filling in all the talent details, selecting the correct parameters such as parents and children and pressing \"Create talent\" to insert the talent into the current tree.");
            ImGui::Bullet();
            ImGui::Text("After selecting a talent or by expanding the \"Edit/Delete Node\" header you can change the talent details of the selected talent (indicated by the red marker in the tree). Same as creating a talent you can change the talent attributes and update the talent with the press of the \"Update\" below. You can also delete a talent or reset the edit screen to the original talent (before updating).");
            ImGui::Bullet();
            ImGui::Text("Under the \"Misc.\" header there are some useful tools to create trees from scratch. You can shift the whole tree by a given row/column number, insert many empty talents at once or let TTM place nodes automatically (it tries to place them without crossing any connections between talents but it is not extensively tested and might run slow for larger trees, use it with caution and only after saving!).");
            ImGui::Spacing();
            Presets::PUSH_FONT(uiData.fontsize, 1);
            ImGui::Text("Loadout editor");
            Presets::POP_FONT();
            ImGui::Separator();
            ImGui::Bullet();
            ImGui::Text("The loadout editor allows you to create one or multiple skillsets inside the loadout, give it a description and import/export one or all skillsets.");
            ImGui::Bullet();
            ImGui::Text("The left side shows the tree and all currently selected talents (and for switch talents the selected talent). Dragging the window around as well as zooming works the same way as the talent tree editor. You can select talents just like ingame by clicking on the talent. By either pressing left Ctrl and clicking on a switch talent or using the middle mouse button you can change the variant.");
            ImGui::Bullet();
            ImGui::Text("In the \"Edit Skillsets\" page you can switch between skillsets, add/delete skillsets and importing/exporting a single skillset or all skillsets. This will only share the skillset, i.e. the assigned talent points, but not the loadout description. For this, you can share the full talent tree.");
            ImGui::Spacing();
            Presets::PUSH_FONT(uiData.fontsize, 1);
            ImGui::Text("Loadout solver");
            Presets::POP_FONT();
            ImGui::Separator();
            ImGui::Bullet();
            ImGui::Text("The loadout solver gives you the ability to automatically create skillsets based on talent preferences and filters. It will find all possible combinations for every number of talent points and will display them for you to dynamically filter important ones.");
            ImGui::Bullet();
            ImGui::Text("Before you can begin you have to process the tree. During that phase, TTM will find every possible skillset and create an index for easy filtering. After it's done it will display the tree on the left (with the usual navigation controls) where you can select talents that have to be included in the skillset (indicated with green for partially filled talents and yellow for fully filled talents) and talents that must not be in the skillset.");
            ImGui::Bullet();
            ImGui::Text("You can then press the filter button to display the filtered skillset selection. The first list box shows all possible talent points and in brackets the number of viable skillsets. You can restrict the skill points to a single number or selecting the desired number in the list.");
            ImGui::Bullet();
            ImGui::Text("After selecting the skill point number you can choose individual skillsets and transfer them to your loadout to view and edit them. You can also add all shown skillsets on the current page to your loadout to have a selection of skillsets ready.");
            ImGui::Spacing();
            Presets::PUSH_FONT(uiData.fontsize, 1);
            ImGui::Text("Sim analysis");
            Presets::POP_FONT();
            ImGui::Separator();
            ImGui::Bullet();
            ImGui::Text("The sim analysis screen allows you to import a raidbots mass sim result and analyze the performance of individual talents and their ranks.");
            ImGui::Bullet();
            ImGui::Text("For more instructions hover over the (?) marks in the screen. A more in-depth explanation might be added in the future.");
            ImGui::Spacing();
            Presets::PUSH_FONT(uiData.fontsize, 2);
            ImGui::Spacing();
            ImGui::Text("Tips");
            Presets::POP_FONT();
            Presets::PUSH_FONT(uiData.fontsize, 1);
            ImGui::Text("Simulationcraft");
            Presets::POP_FONT();
            ImGui::Separator();
            ImGui::Bullet();
            ImGui::Text("To quickly sim different skillsets you can manually add multiple skillsets to your loadout and export to simc (to be implemented in the future) or you can use the loadout solver -> process tree -> filter down to a suitable number of skillsets -> add all to loadout -> export all skillsets to simc -> sim.");
            ImGui::Spacing();
            Presets::PUSH_FONT(uiData.fontsize, 1);
            ImGui::Text("Custom icons");
            Presets::POP_FONT();
            ImGui::Separator();
            ImGui::Bullet();
            ImGui::Text("To include custom icons in TTM, put .png files with size 40x40 in the %%APPDATA%%(Roaming)/Wow Talent Tree Manager/resources/icons/custom/ directory (create it if it doesn't exist). Restart the application and you should see your custom icons being available. If they have the same name as currently existing icons, they will overwrite the original. You can use this to create your own versions of existing icons without changing original icons.");
            ImGui::Spacing();
            ImGui::PopTextWrapPos();

            ImGui::SetItemDefaultFocus();
            if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            static bool scrollUp = false;
            if (!scrollUp) {
                ImGui::SetScrollY(0);
                scrollUp = true;
            }
            ImGui::EndPopup();
        }
        if (ImGui::BeginPopupModal("Changelog##Popup", close, ImGuiWindowFlags_AlwaysAutoResize))
        {
            std::vector<std::string> changeLogData;
            if (loadChangeLogData(changeLogData)) {
                ImGui::PushTextWrapPos(1000);
                for (auto& line : changeLogData) {
                    if (line != "") {
                        ImGui::Text("%s", line.c_str());
                    }
                    else {
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();
                    }
                }
                ImGui::PopTextWrapPos();
            }
            else {
                ImGui::Text("Could not load changelog data. See github releases page for changelog information.");
            }
            ImGui::SetItemDefaultFocus();
            if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            static bool scrollUp = false;
            if (!scrollUp) {
                ImGui::SetScrollY(0);
                scrollUp = true;
            }
            ImGui::EndPopup();
        }
        if (ImGui::BeginPopupModal("Reset TTM##Popup", close, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::PushTextWrapPos(240);
            ImGui::Text("Do you want to reset TTM? This will close the app, delete the workspace (all trees and loadouts) and settings!");
            ImGui::PopTextWrapPos();

            ImGui::SetItemDefaultFocus();
            if (ImGui::Button("Reset", ImVec2(120, 0))) {
                resetWorkspaceAndTrees();
                done = true;
                uiData.resetWorkspace = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) { 
                ImGui::CloseCurrentPopup(); 
            }
            ImGui::EndPopup();
        }
	}

    void RenderWorkArea(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration 
            | ImGuiWindowFlags_NoMove 
            | ImGuiWindowFlags_NoResize 
            | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_NoScrollWithMouse;

        // We demonstrate using the full viewport area or the work area (without menu-bars, task-bars etc.)
        // Based on your use case you may want one of the other.
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        if (ImGui::Begin("MainWindow", nullptr, flags))
        {
            ImGui::PopStyleVar();
            RenderTalentTreeTabs(uiData, talentTreeCollection);
            SubmitDockSpace(uiData);
        }
        ImGui::End();

        RenderWorkAreaWindow(uiData, talentTreeCollection);
    }

	void RenderTalentTreeTabs(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_AutoSelectNewTabs 
            | ImGuiTabBarFlags_Reorderable 
            | ImGuiTabBarFlags_FittingPolicyScroll 
            | ImGuiTabBarFlags_TabListPopupButton;

        if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
        {
            if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip)) {
                ImGui::OpenPopup("Create new tree");
            }
            if (ImGui::TabItemButton("X", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip)) {
                ImGui::OpenPopup("Close all confirmation");
            }

            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            static bool* close = new bool();
            *close = true;
            if (ImGui::BeginPopupModal("Create new tree", close, ImGuiWindowFlags_AlwaysAutoResize))
            {
                if (ImGui::Button("New custom tree", ImVec2(-0.01f, 0))) {
                    TalentTreeData tree;
                    tree.tree = Engine::loadTreePreset(talentTreeCollection.presets["custom"]);

                    talentTreeCollection.trees.push_back(tree);
                    talentTreeCollection.activeTreeIndex = static_cast<int>(talentTreeCollection.trees.size() - 1);
                    uiData.treeEditorImportTreeString = "";
                    uiData.treeEditorExportTreeString = "";
                    loadActiveIcons(uiData, talentTreeCollection, true);
                    uiData.treeEditorSelectedTalent = nullptr;
                    saveWorkspace(uiData, talentTreeCollection);
                    ImGui::CloseCurrentPopup();
                }


                ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("or").x) * 0.5f);
                ImGui::Text("or");

                int oldClass = uiData.treeEditorPresetClassCombo;
                ImGui::Combo("##treeEditorCreationPresetClassCombo", &uiData.treeEditorPresetClassCombo, Presets::CLASSES, IM_ARRAYSIZE(Presets::CLASSES));
                if (oldClass != uiData.treeEditorPresetClassCombo) {
                    uiData.treeEditorPresetSpecCombo = 0;
                }
                int specCount = Presets::RETURN_SPEC_COUNT(uiData.treeEditorPresetClassCombo);
                if (uiData.treeEditorPresetSpecCombo >= specCount) {
                    uiData.treeEditorPresetSpecCombo = specCount - 1;
                }

                ImGui::Combo(
                    "##treeEditorCreationPresetSpecCombo",
                    &uiData.treeEditorPresetSpecCombo,
                    Presets::RETURN_SPECS(uiData.treeEditorPresetClassCombo),
                    specCount
                );
                //ImGui::SameLine();
                if (ImGui::Button("Load preset", ImVec2(-0.01f, 0))) { 
                    TalentTreeData tree;
                    tree.tree = Engine::loadTreePreset(Presets::RETURN_PRESET(talentTreeCollection.presets, uiData.treeEditorPresetClassCombo, uiData.treeEditorPresetSpecCombo));
                    talentTreeCollection.trees.push_back(tree);
                    talentTreeCollection.activeTreeIndex = static_cast<int>(talentTreeCollection.trees.size() - 1);
                    talentTreeCollection.activeTree().activeSkillsetIndex = -1;
                    uiData.treeEditorImportTreeString = "";
                    uiData.treeEditorExportTreeString = "";
                    loadActiveIcons(uiData, talentTreeCollection, true);
                    uiData.treeEditorSelectedTalent = nullptr;
                    saveWorkspace(uiData, talentTreeCollection);
                    ImGui::CloseCurrentPopup(); 
                }

                updateCustomTreeFileList(uiData);
                uiData.treeEditorIsCustomTreeFileListValid = true;
                if (uiData.treeEditorCustomTreeFileList.size() > 0) {
                    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("or").x) * 0.5f);
                    ImGui::Text("or");
                    //uiData.treeEditorCustomTreeFileList[uiData.treeEditorCustomTreeListCurrent].first
                    uiData.treeEditorCustomTreeListCurrent = uiData.treeEditorCustomTreeListCurrent < 0 ? 0 : uiData.treeEditorCustomTreeListCurrent;
                    uiData.treeEditorCustomTreeListCurrent = uiData.treeEditorCustomTreeListCurrent >= uiData.treeEditorCustomTreeFileList.size() ? static_cast<int>(uiData.treeEditorCustomTreeFileList.size() - 1) : uiData.treeEditorCustomTreeListCurrent;
                    const char* comboPreviewValue = uiData.treeEditorCustomTreeFileList[uiData.treeEditorCustomTreeListCurrent].second.c_str();
                    if (ImGui::BeginCombo("##talentCreationIconNameCombo", comboPreviewValue))
                    {
                        for (size_t n = 0; n < uiData.treeEditorCustomTreeFileList.size(); n++)
                        {
                            auto& customTreePathNamePair = uiData.treeEditorCustomTreeFileList[n];
                            const bool is_selected = (comboPreviewValue == customTreePathNamePair.second.c_str());
                            if (ImGui::Selectable(customTreePathNamePair.second.c_str(), is_selected)) {
                                uiData.treeEditorCustomTreeListCurrent = static_cast<int>(n);
                            }

                            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                            if (is_selected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    //ImGui::SameLine();
                    if (ImGui::Button("Load custom tree", ImVec2(-0.01f, 0))) {
                        TalentTreeData tree;
                        talentTreeCollection.trees.push_back(tree);
                        talentTreeCollection.activeTreeIndex = static_cast<int>(talentTreeCollection.trees.size() - 1);
                        talentTreeCollection.activeTree().activeSkillsetIndex = -1;
                        if (!loadTreeFromFile(uiData, talentTreeCollection)) {
                            ImGui::CloseCurrentPopup();
                            ImGui::OpenPopup("Tree load error");
                        }
                        else {
                            uiData.treeEditorIsCustomTreeFileListValid = false;
                            Engine::validateLoadout(talentTreeCollection.activeTree(), true);
                            clearSolvingProcess(uiData, talentTreeCollection);

                            loadActiveIcons(uiData, talentTreeCollection, true);
                            uiData.treeEditorSelectedTalent = nullptr;

                            uiData.treeEditorImportTreeString = "";
                            uiData.treeEditorExportTreeString = "";
                            saveWorkspace(uiData, talentTreeCollection);
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }


                ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("or").x) * 0.5f);
                ImGui::Text("or");
                ImGui::InputText("##treeEditorCreationPopupImportText", &uiData.treeEditorImportTreeString, ImGuiInputTextFlags_AutoSelectAll);
                if (ImGui::Button("Import tree##popup", ImVec2(-0.01f, 0))) {
                    if (!Engine::validateAndRepairTreeStringFormat(uiData.treeEditorImportTreeString)) {
                        uiData.treeEditorImportTreeString = "Invalid import string!";
                    }
                    else {
                        TalentTreeData tree;
                        tree.tree = Engine::parseTree(uiData.treeEditorImportTreeString);
                        talentTreeCollection.trees.push_back(tree);
                        talentTreeCollection.activeTreeIndex = static_cast<int>(talentTreeCollection.trees.size() - 1);
                        uiData.treeEditorImportTreeString = "";
                        uiData.treeEditorExportTreeString = "";
                        loadActiveIcons(uiData, talentTreeCollection, true);
                        uiData.treeEditorSelectedTalent = nullptr;
                        saveWorkspace(uiData, talentTreeCollection);
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndPopup();
            }
            if (ImGui::BeginPopupModal("Close all confirmation", close, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Close all tabs?");
                if (ImGui::Button("Yes", ImVec2(80, 0))) {
                    talentTreeCollection.trees.clear();
                    talentTreeCollection.activeTreeIndex = -1;
                    uiData.editorView = EditorView::None;
                    //saveWorkspace(uiData, talentTreeCollection);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(80, 0))) {
                    uiData.deleteTreeIndex = -1;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            if (ImGui::BeginPopupModal("Tree load error", close, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("This custom tree was unable to be loaded. It might be corrupted or out of date.");
                if (ImGui::Button("Ok", ImVec2(80, 0))) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            // Submit our regular tabs
            int differentTreeIndex = -1;
            bool tabNotHovered = true;
            static bool treeSwitchCD = false;
            for (int n = 0; n < talentTreeCollection.trees.size(); )
            {
                Presets::SET_TAB_ITEM_COLOR(uiData.style, talentTreeCollection.trees[n].tree.presetName);
                bool isReset = false;
                bool open = true;
                ImGuiTabItemFlags flag = n == talentTreeCollection.activeTreeIndex ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
                std::string displayTag = talentTreeCollection.trees[n].tree.name;
                if (n == talentTreeCollection.activeTreeIndex) {
                    displayTag = "> " + displayTag;
                }
                if (ImGui::BeginTabItemNoClose((displayTag + "###treetab" + std::to_string(n)).c_str(), &open, flag))
                {
                    if (n == talentTreeCollection.activeTreeIndex) {
                        RenderTreeViewTabs(uiData, talentTreeCollection);
                    }
                    else {
                        differentTreeIndex = n;
                    }
                    ImGui::EndTabItem();
                }
                if (ImGui::IsItemHovered()) {
                    if (talentTreeCollection.activeTreeIndex != n && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        clearTextboxes(uiData);
                        resetComplementaryIndices(talentTreeCollection);
                        talentTreeCollection.activeTreeIndex = n;
                        uiData.editorView = EditorView::None;
                        uiData.isLoadoutInitValidated = false;
                        uiData.treeEditorSelectedTalent = nullptr;
                        loadActiveIcons(uiData, talentTreeCollection);
                    }
                    tabNotHovered = false;
                }

                if (!open) {
                    uiData.deleteTreeIndex = n;
                }
                else
                    n++;
                if (!isReset) {
                    Presets::RESET_TAB_ITEM_COLOR();
                }
            }
            if (tabNotHovered && differentTreeIndex >= 0 && !treeSwitchCD) {
                if (talentTreeCollection.activeTreeIndex != differentTreeIndex) {
                    treeSwitchCD = true;
                    clearTextboxes(uiData);
                    resetComplementaryIndices(talentTreeCollection);
                    talentTreeCollection.activeTreeIndex = differentTreeIndex;
                    uiData.editorView = EditorView::None;
                    uiData.isLoadoutInitValidated = false;
                    uiData.treeEditorSelectedTalent = nullptr;
                    loadActiveIcons(uiData, talentTreeCollection);
                }
            }
            else {
                treeSwitchCD = false;
            }
            if (uiData.deleteTreeIndex >= 0) {
                ImGui::OpenPopup("Delete tree confirmation");
            }
            center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            if (ImGui::BeginPopupModal("Delete tree confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Delete the tree? This can not be undone!");
                if (ImGui::Button("Yes", ImVec2(50, 0))) {
                    if (uiData.deleteTreeIndex >= 0 && uiData.deleteTreeIndex < talentTreeCollection.trees.size()) {
                        talentTreeCollection.trees.erase(talentTreeCollection.trees.begin() + uiData.deleteTreeIndex);
                        talentTreeCollection.activeTreeIndex = static_cast<int>(talentTreeCollection.trees.size() - 1);
                        loadActiveIcons(uiData, talentTreeCollection, true);
                        //saveWorkspace(uiData, talentTreeCollection);
                    }
                    if (talentTreeCollection.trees.size() == 0) {
                        uiData.editorView = EditorView::None;
                    }
                    uiData.deleteTreeIndex = -1;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(50, 0))) {
                    uiData.deleteTreeIndex = -1;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            ImGui::EndTabBar();
        }
    }

    void RenderTreeViewTabs(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        // Expose some other flags which are useful to showcase how they interact with Leading/Trailing tabs
        static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_FittingPolicyScroll;

        if (ImGui::BeginTabBar("TreeViewTabBar", tab_bar_flags))
        {
            std::string loadoutEditDisplayTag = "Talent Loadout Editor";
            if (uiData.editorView == EditorView::LoadoutEdit) {
                loadoutEditDisplayTag = "> " + loadoutEditDisplayTag;
            }
            ImGuiTabItemFlags tabFlags = uiData.editorViewTarget == EditorView::LoadoutEdit ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
            if (ImGui::BeginTabItem((loadoutEditDisplayTag + "###LoadoutEditTabID").c_str(), nullptr, tabFlags))
            {
                if (uiData.editorViewTarget == EditorView::LoadoutEdit) {
                    uiData.editorViewTarget = EditorView::None;
                }
                if (uiData.editorView != EditorView::LoadoutEdit) {
                    uiData.editorView = EditorView::LoadoutEdit;
                    //saveWorkspace(uiData, talentTreeCollection);
                }
                if (!uiData.isLoadoutInitValidated) {
                    uiData.isLoadoutInitValidated = true;
                    Engine::validateLoadout(talentTreeCollection.activeTree(), true);
                    if (talentTreeCollection.activeTree().loadout.size() > 0) {
                        talentTreeCollection.activeTree().activeSkillsetIndex = std::clamp(
                            talentTreeCollection.activeTree().activeSkillsetIndex,
                            0,
                            static_cast<int>(talentTreeCollection.activeTree().loadout.size() - 1)
                        );
                        Engine::activateSkillset(talentTreeCollection.activeTree(), talentTreeCollection.activeTree().activeSkillsetIndex);
                    }
                    else {
                        talentTreeCollection.activeTree().activeSkillsetIndex = -1;
                    }
                }
                ImGui::EndTabItem();
            }
            std::string loadoutSolverDisplayTag = "Talent Loadout Solver";
            if (uiData.editorView == EditorView::LoadoutSolver) {
                loadoutSolverDisplayTag = "> " + loadoutSolverDisplayTag;
            }
            tabFlags = uiData.editorViewTarget == EditorView::LoadoutSolver ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
            if (ImGui::BeginTabItem((loadoutSolverDisplayTag + "###LoadoutSolverTabID").c_str(), nullptr, tabFlags))
            {
                if (uiData.editorViewTarget == EditorView::LoadoutSolver) {
                    uiData.editorViewTarget = EditorView::None;
                }
                if (uiData.editorView != EditorView::LoadoutSolver) {
                    uiData.editorView = EditorView::LoadoutSolver;
                    //saveWorkspace(uiData, talentTreeCollection);
                }
                uiData.isLoadoutInitValidated = false;
                ImGui::EndTabItem();
            }
            std::string simAnalysisDisplayTag = "Sim Analysis";
            if (uiData.editorView == EditorView::SimAnalysis) {
                simAnalysisDisplayTag = "> " + simAnalysisDisplayTag;
            }
            tabFlags = uiData.editorViewTarget == EditorView::SimAnalysis ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
            if (ImGui::BeginTabItem((simAnalysisDisplayTag + "###SimAnalysisTabID").c_str(), nullptr, tabFlags))
            {
                if (uiData.editorViewTarget == EditorView::SimAnalysis) {
                    uiData.editorViewTarget = EditorView::None;
                }
                if (uiData.editorView != EditorView::SimAnalysis) {
                    uiData.editorView = EditorView::SimAnalysis;
                }
                uiData.isLoadoutInitValidated = false;
                ImGui::EndTabItem();
            }
            std::string treeEditDisplayTag = "Talent Tree Editor";
            if (uiData.editorView == EditorView::TreeEdit) {
                treeEditDisplayTag = "> " + treeEditDisplayTag;
            }
            tabFlags = uiData.editorViewTarget == EditorView::TreeEdit ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
            if (ImGui::BeginTabItem((treeEditDisplayTag + "###TreeEditTabID").c_str(), nullptr, tabFlags))
            {
                if (uiData.editorViewTarget == EditorView::TreeEdit) {
                    uiData.editorViewTarget = EditorView::None;
                }
                if (uiData.editorView != EditorView::TreeEdit) {
                    uiData.editorView = EditorView::TreeEdit;
                    //saveWorkspace(uiData, talentTreeCollection);
                }
                uiData.isLoadoutInitValidated = false;
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }

    void SubmitDockSpace(UIData& uiData) {
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoTabBar;

        ImGuiID dockspace_id = ImGui::GetID("WorkAreaDockspace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

        static bool first = true;
        if (first) {
            first = false;
            ImGui::DockBuilderRemoveNode(dockspace_id); // Clear out existing layout
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace); // Add empty node
            ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);//ImGui::GetCurrentWindow()->Size);

            ImGuiID dock_main_id = dockspace_id; // This variable will track the document node, however we are not using it here as we aren't docking anything into it.
            ImGuiID dock_id_left, dock_id_right, dock_id_bottom_left;
            float leftRightSplit = uiData.dockWindowRatio > 0.0f ? uiData.dockWindowRatio : 0.40f;
            leftRightSplit = leftRightSplit > 1.0f ? 0.4f : leftRightSplit;
            ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, leftRightSplit, &dock_id_right, &dock_id_left);
            ImGui::DockBuilderSplitNode(dock_id_left, ImGuiDir_Down, 0.045f, &dock_id_bottom_left, &dock_id_left);

            ImGui::DockBuilderDockWindow("TreeWindow", dock_id_left);
            ImGui::DockBuilderDockWindow("SettingsWindow", dock_id_right);
            ImGui::DockBuilderDockWindow("SearchWindow", dock_id_bottom_left);
            ImGui::DockBuilderFinish(dockspace_id);

            uiData.leftWindowDockId = dock_id_left;
            uiData.rightWindowDockId = dock_id_right;
        }
    }

    void RenderWorkAreaWindow(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        switch (uiData.editorView) {
        case EditorView::TreeEdit: {RenderTalentTreeEditorWindow(uiData, talentTreeCollection); } break;
        case EditorView::LoadoutEdit: {RenderLoadoutEditorWindow(uiData, talentTreeCollection); } break;
        case EditorView::LoadoutSolver: {RenderLoadoutSolverWindow(uiData, talentTreeCollection); } break;
        case EditorView::SimAnalysis: {RenderSimAnalysisWindow(uiData, talentTreeCollection); } break;
        }
        
    }

	void RenderStatusBar(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        if (talentTreeCollection.activeTreeIndex >= 0) {
            Presets::SET_STATUS_BAR_COLOR(uiData.style, talentTreeCollection.activeTree().presetName);
        }
        //TTMTODO: Implement different statusbar texts for different views
		if (ImGui::BeginViewportSideBar("MainStatusBar", NULL, ImGuiDir_Down, ImGui::GetFrameHeight(), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar)) {
			if (ImGui::BeginMenuBar()) {
                std::string text = "Currently active tree: ";
                text += talentTreeCollection.activeTreeIndex < 0 ? "none" : talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.name;
                if (talentTreeCollection.activeTreeIndex >= 0) {
                    text += " - node count: " + std::to_string(talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.nodeCount);
                    text += " - maximum skill points : " + std::to_string(talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.maxTalentPoints);
                    if (uiData.editorView == EditorView::LoadoutEdit && talentTreeCollection.activeTree().activeSkillsetIndex >= 0) {
                        int minLevel = Engine::getLevelRequirement(talentTreeCollection.activeSkillset()->talentPointsSpent, talentTreeCollection.activeTree());
                        if (minLevel > 0) {
                            text += " - required level: " + std::to_string(minLevel);
                            if (minLevel > 70) {
                                text += " - REQUIRED LEVEL IS GREATER THAN CURRENT MAX LEVEL (70)";
                            }
                        }
                    }
                }
				ImGui::Text(text.c_str());
				ImGui::EndMenuBar();
			}
		}
        ImGui::End();
        if (talentTreeCollection.activeTreeIndex >= 0) {
            Presets::RESET_STATUS_BAR_COLOR();
        }
	}

    std::filesystem::path getCustomTreePath() {
        std::filesystem::path treePath = Presets::getAppPath() / "CustomTrees";

        //create custom tree folder if it doesn't exist
        if (!std::filesystem::is_directory(treePath)) {
            std::filesystem::create_directory(treePath);
        }

        return treePath;
    }

    void initWorkspace() {
        std::filesystem::path appPath = Presets::getAppPath(); //if appPath directory doesn't exist it will be auto created here
        std::filesystem::path shippedResourcesDir = "./resources/";
        if (!std::filesystem::is_directory(appPath / "resources")) {
            std::filesystem::create_directory(appPath / "resources");
        }
        std::filesystem::copy_file(shippedResourcesDir / "resource_versions.txt", appPath / "resources" / "resource_versions.txt", std::filesystem::copy_options::skip_existing);
        std::filesystem::copy_file(shippedResourcesDir / "presets.txt", appPath / "resources" / "presets.txt", std::filesystem::copy_options::skip_existing);
        if (!std::filesystem::is_directory(appPath / "resources" / "icons")) {
            std::filesystem::copy(shippedResourcesDir / "icons", appPath / "resources" / "icons", std::filesystem::copy_options::recursive | std::filesystem::copy_options::skip_existing);
        }
    }

    void saveWorkspace(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        std::filesystem::path appPath = Presets::getAppPath();
        //backup old files
        if (std::filesystem::is_regular_file(appPath / "settings.txt")) {
            std::ifstream oldSetFile(appPath / "settings.txt", std::ios::binary);
            std::ofstream bkpSetFile(appPath / "settings_backup.txt", std::ios::binary);
            bkpSetFile << oldSetFile.rdbuf();
            oldSetFile.close();
            bkpSetFile.close();
        }
        if (std::filesystem::is_regular_file(appPath / "workspace.txt")) {
            std::ifstream oldWorkFile(appPath / "workspace.txt", std::ios::binary);
            std::ofstream bkpWorkFile(appPath / "workscape_backup.txt", std::ios::binary);
            bkpWorkFile << oldWorkFile.rdbuf();
            oldWorkFile.close();
            bkpWorkFile.close();
        }

        std::string settings = "";
        std::string workspace = "";
        // save settings
        //TTMNOTE: settings can't have ":" char in them, messes with loadWorkspace
        switch (uiData.style) {
        case Presets::STYLES::COMPANY_GREY: {settings += "STYLE=COMPANY_GREY\n"; }break;
        case Presets::STYLES::PATH_OF_TALENT_TREE: {settings += "STYLE=PATH_OF_TALENT_TREE\n"; }break;
        case Presets::STYLES::LIGHT_MODE: {settings += "STYLE=LIGHT_MODE\n"; }break;
        default: {settings += "STYLE=COMPANY_GREY\n"; }break;
        }
        if (uiData.enableGlow) {
            settings += "GLOW=1\n";
        }
        else {
            settings += "GLOW=0\n";
        }
        switch (uiData.fontsize) {
        case Presets::FONTSIZE::MINI: {settings += "FONTSIZE=MINI\n"; } break;
        case Presets::FONTSIZE::SMALL: {settings += "FONTSIZE=SMALL\n"; } break;
        case Presets::FONTSIZE::DEFAULT: {settings += "FONTSIZE=DEFAULT\n"; } break;
        case Presets::FONTSIZE::LARGE: {settings += "FONTSIZE=LARGE\n"; } break;
        case Presets::FONTSIZE::HUGE: {settings += "FONTSIZE=HUGE\n"; } break;
        default: {settings += "FONTSIZE=DEFAULT\n"; } break;
        }
        WINDOWPLACEMENT wp = { 0 };
        wp.length = sizeof(wp);
        if (GetWindowPlacement(*uiData.hwnd, &wp))
        {
            // save wp values somewhere...
            settings += "WINDOWPLACEMENT="
                + std::to_string(wp.rcNormalPosition.left)
                + "," + std::to_string(wp.rcNormalPosition.top)
                + "," + std::to_string(wp.rcNormalPosition.right)
                + "," + std::to_string(wp.rcNormalPosition.bottom)
                + "," + std::to_string(wp.showCmd)
                + "\n";
        }

        ImGuiDockNode* leftWindowNode = ImGui::DockBuilderGetNode(uiData.leftWindowDockId);
        ImGuiDockNode* rightWindowNode = ImGui::DockBuilderGetNode(uiData.rightWindowDockId);
        float ratio = rightWindowNode->Size.x / (leftWindowNode->Size.x + rightWindowNode->Size.x);
        settings += "DIVIDERRATIO=" + std::to_string(ratio) + "\n";

        // save talent tree collection
        for (TalentTreeData tree : talentTreeCollection.trees) {
            workspace += Engine::createTreeStringRepresentation(tree.tree) + "\n";
        }
        workspace += "ACTIVETREE=" + std::to_string(talentTreeCollection.activeTreeIndex) + "\n";
        if (talentTreeCollection.activeTreeIndex >= 0 && talentTreeCollection.activeTreeIndex < talentTreeCollection.trees.size()) {
            workspace += "ACTIVESKILLSET=" + std::to_string(talentTreeCollection.activeTree().activeSkillsetIndex) + "\n";
        }

        std::ofstream setFile(appPath / "settings.txt");
        setFile << settings;
        std::ofstream workFile(appPath / "workspace.txt");
        workFile << workspace;
    }

    TalentTreeCollection loadWorkspace(UIData& uiData) {
        std::filesystem::path appPath = Presets::getAppPath();
        //restore settings
        useDefaultSettings(uiData);
        bool settingsFileExists = std::filesystem::is_regular_file(appPath / "settings.txt");
        if (settingsFileExists) {
            std::ifstream setFile(appPath / "settings.txt");
            std::string line;
            while (std::getline(setFile, line)) {
                if (line.find("STYLE") != std::string::npos) {
                    if (line.find("COMPANY_GREY") != std::string::npos) {
                        Presets::SET_GUI_STYLE_COMPANY_GREY();
                        uiData.style = Presets::STYLES::COMPANY_GREY;
                    }
                    else if (line.find("PATH_OF_TALENT_TREE") != std::string::npos) {
                        Presets::SET_GUI_STYLE_PATH_OF_TALENT_TREE();
                        uiData.style = Presets::STYLES::PATH_OF_TALENT_TREE;
                    }
                    else if (line.find("LIGHT_MODE") != std::string::npos) {
                        Presets::SET_GUI_STYLE_LIGHT_MODE();
                        uiData.style = Presets::STYLES::LIGHT_MODE;
                    }
                }
                if (line.find("GLOW") != std::string::npos) {
                    uiData.enableGlow = (line == "GLOW=1");
                }
                if (line.find("FONTSIZE") != std::string::npos) {
                    if (line.find("MINI") != std::string::npos) {
                        uiData.fontsize = Presets::FONTSIZE::MINI;
                    }
                    else if (line.find("SMALL") != std::string::npos) {
                        uiData.fontsize = Presets::FONTSIZE::SMALL;
                    }
                    else if (line.find("DEFAULT") != std::string::npos) {
                        uiData.fontsize = Presets::FONTSIZE::DEFAULT;
                    }
                    else if (line.find("LARGE") != std::string::npos) {
                        uiData.fontsize = Presets::FONTSIZE::LARGE;
                    }
                    else if (line.find("HUGE") != std::string::npos) {
                        uiData.fontsize = Presets::FONTSIZE::HUGE;
                    }
                }
                //WINDOWPLACEMENT is loaded in a separate function cause it needs to happen earlier
                if (line.find("DIVIDERRATIO") != std::string::npos) {
                    std::string ratioStr = line.substr(line.find("=") + 1);
                    uiData.dockWindowRatio = std::stof(ratioStr);
                }
            }
        }
        //init talent tree collection
        TalentTreeCollection col;
        if (!std::filesystem::is_regular_file(appPath / "workspace.txt")) {
            return col;
        }
        std::ifstream workFile(appPath / "workspace.txt");
        std::string line;
        int tempActiveTree = -1;
        int tempActiveSkillset = -1;
        //TTMTODO: might need major reworks down the line
        while (std::getline(workFile, line))
        {
            if (line == "") {
                continue;
            }
            //TTMNOTE: for backwards compatibility try to restore settings from workspace file
            if (!settingsFileExists) {
                if (line.find("STYLE") != std::string::npos) {
                    if (line.find("COMPANY_GREY") != std::string::npos) {
                        Presets::SET_GUI_STYLE_COMPANY_GREY();
                        uiData.style = Presets::STYLES::COMPANY_GREY;
                    }
                    else if (line.find("PATH_OF_TALENT_TREE") != std::string::npos) {
                        Presets::SET_GUI_STYLE_PATH_OF_TALENT_TREE();
                        uiData.style = Presets::STYLES::PATH_OF_TALENT_TREE;
                    }
                    else if (line.find("LIGHT_MODE") != std::string::npos) {
                        Presets::SET_GUI_STYLE_LIGHT_MODE();
                        uiData.style = Presets::STYLES::LIGHT_MODE;
                    }
                }
                if (line.find("GLOW") != std::string::npos) {
                    uiData.enableGlow = (line == "GLOW=1");
                }
                //TTMNOTE: This couldn't ever be in the workspace file but just to be unnecessarily safe
                if (line.find("FONTSIZE") != std::string::npos) {
                    if (line.find("MINI") != std::string::npos) {
                        uiData.fontsize = Presets::FONTSIZE::MINI;
                    }
                    else if (line.find("SMALL") != std::string::npos) {
                        uiData.fontsize = Presets::FONTSIZE::SMALL;
                    }
                    else if (line.find("DEFAULT") != std::string::npos) {
                        uiData.fontsize = Presets::FONTSIZE::DEFAULT;
                    }
                    else if (line.find("LARGE") != std::string::npos) {
                        uiData.fontsize = Presets::FONTSIZE::LARGE;
                    }
                    else if (line.find("HUGE") != std::string::npos) {
                        uiData.fontsize = Presets::FONTSIZE::HUGE;
                    }
                }
            }
            TalentTreeData data;
            if (Engine::validateAndRepairTreeStringFormat(line)) {
                try {
                    data.tree = Engine::parseTree(line);
                    col.trees.push_back(data);
                    col.activeTreeIndex = static_cast<int>(col.trees.size() - 1);
                }
                catch (std::logic_error& e) {
                    ImGui::LogText(e.what());
                }
            }

            if (line.find("ACTIVETREE") != std::string::npos) {
                tempActiveTree = std::stoi(line.substr(line.find("=") + 1));
            }

            if (line.find("ACTIVESKILLSET") != std::string::npos) {
                tempActiveSkillset = std::stoi(line.substr(line.find("=") + 1));
            }
        }
        if (tempActiveTree >= 0 && tempActiveTree < col.trees.size()) {
            col.activeTreeIndex = tempActiveTree;
        }
        if (col.activeTreeIndex >= 0) {
            if (tempActiveSkillset >= 0 && tempActiveSkillset < col.trees[col.activeTreeIndex].tree.loadout.size()) {
                col.trees[col.activeTreeIndex].tree.activeSkillsetIndex = tempActiveSkillset;
            }
        }
        return col;
    }

    void resetWorkspaceAndTrees() {
        std::filesystem::path appPath = Presets::getAppPath();
        std::filesystem::remove_all(appPath);
    }

    WINDOWPLACEMENT loadWindowPlacement() {
        WINDOWPLACEMENT wp = { 0 };
        wp.length = sizeof(wp);
        wp.showCmd = 1;
        wp.rcNormalPosition = { 100, 100, 1700, 1000 };

        std::filesystem::path appPath = Presets::getAppPath();
        bool settingsFileExists = std::filesystem::is_regular_file(appPath / "settings.txt");
        if (settingsFileExists) {
            std::ifstream setFile(appPath / "settings.txt");
            std::string line;
            while (std::getline(setFile, line)) {
                if (line.find("WINDOWPLACEMENT=") != std::string::npos) {
                    std::vector<std::string> wpSaved = Engine::splitString(line.substr(16), ",");
                    wp.showCmd = static_cast<UINT>(std::stoi(wpSaved[4]));
                    wp.rcNormalPosition = {
                        std::stoi(wpSaved[0]),
                        std::stoi(wpSaved[1]),
                        std::stoi(wpSaved[2]),
                        std::stoi(wpSaved[3]),
                    };
                }
            }
        }
        return wp;
    }

    bool loadChangeLogData(std::vector<std::string>& changeLogData) {
        std::filesystem::path changeLogPath{ "changelog.txt" };
        if (std::filesystem::is_regular_file(changeLogPath)) {
            try {
                std::ifstream changeLogFileStream;
                changeLogFileStream.open(changeLogPath);
                if (changeLogFileStream.is_open())
                {
                    if (!changeLogFileStream.eof())
                    {
                        std::string line;
                        while (std::getline(changeLogFileStream, line)) {
                            changeLogData.push_back(line);
                        }
                    }
                    else {
                        return false;
                    }
                }
                else {
                    return false;
                }
            }
            catch (const std::ifstream::failure& e) {
                ImGui::LogText(e.what());
                return false;
            }
        }
        else {
            return false;
        }
        return true;
    }

    void useDefaultSettings(UIData& uiData) {
        Presets::SET_GUI_STYLE_LIGHT_MODE();
        uiData.style = Presets::STYLES::LIGHT_MODE;
        uiData.enableGlow = false;
        uiData.fontsize = Presets::FONTSIZE::DEFAULT;
    }
}