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

#include <windows.h>
#include <shlobj.h>

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
				ImGui::EndMenu();
			}
            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("Controls & Tips")) {
                    uiData.showHelpPopup = true;
                }
                if (ImGui::MenuItem("About TTM")) {
                    uiData.showAboutPopup = true;
                }
                if (ImGui::MenuItem("Check Updates")) {
                    uiData.updateStatus = UpdateStatus::NOTCHECKED;
                    uiData.renderedOnce = false;
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

            ImGui::SetItemDefaultFocus();
            if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }
        if (ImGui::BeginPopupModal("Controls & Tips##Popup", close, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::PushTextWrapPos(1000);
            ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[2]);
            ImGui::Spacing();
            ImGui::Text("Controls");
            ImGui::PopFont();
            ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
            ImGui::Text("Tree editor");
            ImGui::PopFont();
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
            ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
            ImGui::Text("Loadout editor");
            ImGui::PopFont();
            ImGui::Separator();
            ImGui::Bullet();
            ImGui::Text("The loadout editor allows you to create one or multiple skillsets inside the loadout, give it a description and import/export one or all skillsets.");
            ImGui::Bullet();
            ImGui::Text("The left side shows the tree and all currently selected talents (and for switch talents the selected talent). Dragging the window around as well as zooming works the same way as the talent tree editor. You can select talents just like ingame by clicking on the talent. By either pressing left Ctrl and clicking on a switch talent or using the middle mouse button you can change the variant.");
            ImGui::Bullet();
            ImGui::Text("In the \nEdit Skillsets\n page you can switch between skillsets, add/delete skillsets and importing/exporting a single skillset or all skillsets. This will only share the skillset, i.e. the assigned talent points, but not the loadout description. For this, you can share the full talent tree.");
            ImGui::Spacing();
            ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
            ImGui::Text("Loadout solver");
            ImGui::PopFont();
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
            ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[2]);
            ImGui::Spacing();
            ImGui::Text("Tips");
            ImGui::PopFont();
            ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
            ImGui::Text("Simulationcraft");
            ImGui::PopFont();
            ImGui::Separator();
            ImGui::Bullet();
            ImGui::Text("To quickly sim different skillsets you can manually add multiple skillsets to your loadout and export to simc (to be implemented in the future) or you can use the loadout solver -> process tree -> filter down to a suitable number of skillsets -> add all to loadout -> export all skillsets to simc -> sim.");
            ImGui::Spacing();
            ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
            ImGui::Text("Custom icons");
            ImGui::PopFont();
            ImGui::Separator();
            ImGui::Bullet();
            ImGui::Text("To include custom icons in TTM, put .png files with size 40x40 in the resources/icons/custom/ directory (create it if it doesn't exist). Restart the application and you should see your custom icons being available. If they have the same name as currently existing icons, they will overwrite the original. You can use this to create your own versions of existing icons without changing original icons.");
            ImGui::Spacing();
            ImGui::PopTextWrapPos();

            ImGui::SetItemDefaultFocus();
            if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
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
            RenderTalentTreeTabs(uiData, talentTreeCollection);
            SubmitDockSpace();
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
                    uiData.treeEditorImportTreeString = "";
                    uiData.treeEditorExportTreeString = "";
                    loadActiveIcons(uiData, talentTreeCollection, true);
                    uiData.treeEditorSelectedTalent = nullptr;
                    saveWorkspace(uiData, talentTreeCollection);
                    ImGui::CloseCurrentPopup(); 
                }
                ImGui::Text("or");
                ImGui::InputText("##treeEditorCreationPopupImportText", &uiData.treeEditorImportTreeString);
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

            // Submit our regular tabs
            for (int n = 0; n < talentTreeCollection.trees.size(); )
            {
                Presets::SET_TAB_ITEM_COLOR(uiData.style, talentTreeCollection.trees[n].tree.presetName);
                bool isReset = false;
                bool open = true;
                ImGuiTabItemFlags flag = ImGuiTabItemFlags_None;
                std::string displayTag = talentTreeCollection.trees[n].tree.name;
                if (n == talentTreeCollection.activeTreeIndex) {
                    displayTag = "> " + displayTag;
                }
                if (ImGui::BeginTabItem((displayTag + "###treetab" + std::to_string(n)).c_str(), &open, flag))
                {
                    if (open) {
                        if (talentTreeCollection.activeTreeIndex != n) {
                            talentTreeCollection.activeTreeIndex = n;
                            uiData.editorView = EditorView::None;
                            uiData.isLoadoutInitValidated = false;
                            uiData.treeEditorSelectedTalent = nullptr;
                            loadActiveIcons(uiData, talentTreeCollection);

                            //clear the talent search field and results
                            uiData.talentSearchString = "";
                            uiData.searchedTalents.clear();
                        }
                    }
                    RenderTreeViewTabs(uiData, talentTreeCollection);
                    ImGui::EndTabItem();
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
            if (ImGui::BeginTabItem((loadoutEditDisplayTag + "###LoadoutEditTabID").c_str(), nullptr, ImGuiTabItemFlags_None))
            {
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
            if (ImGui::BeginTabItem((loadoutSolverDisplayTag + "###LoadoutSolverTabID").c_str(), nullptr, ImGuiTabItemFlags_None))
            {
                if (uiData.editorView != EditorView::LoadoutSolver) {
                    uiData.editorView = EditorView::LoadoutSolver;
                    //saveWorkspace(uiData, talentTreeCollection);
                }
                uiData.isLoadoutInitValidated = false;
                ImGui::EndTabItem();
            }
            std::string treeEditDisplayTag = "Talent Tree Editor";
            if (uiData.editorView == EditorView::TreeEdit) {
                treeEditDisplayTag = "> " + treeEditDisplayTag;
            }
            if (ImGui::BeginTabItem((treeEditDisplayTag + "###TreeEditTabID").c_str(), nullptr, ImGuiTabItemFlags_None))
            {
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

    void SubmitDockSpace() {
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
            ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.40f, &dock_id_right, &dock_id_left);
            ImGui::DockBuilderSplitNode(dock_id_left, ImGuiDir_Down, 0.045f, &dock_id_bottom_left, &dock_id_left);

            ImGui::DockBuilderDockWindow("TreeWindow", dock_id_left);
            ImGui::DockBuilderDockWindow("SettingsWindow", dock_id_right);
            ImGui::DockBuilderDockWindow("SearchWindow", dock_id_bottom_left);
            ImGui::DockBuilderFinish(dockspace_id);
        }
    }

    void RenderWorkAreaWindow(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        switch (uiData.editorView) {
        case EditorView::TreeEdit: {RenderTalentTreeEditorWindow(uiData, talentTreeCollection); } break;
        case EditorView::LoadoutEdit: {RenderLoadoutEditorWindow(uiData, talentTreeCollection); } break;
        case EditorView::LoadoutSolver: {RenderLoadoutSolverWindow(uiData, talentTreeCollection); } break;
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
                        int minLevel = 0;
                        if (talentTreeCollection.activeSkillset()->talentPointsSpent > talentTreeCollection.activeTree().preFilledTalentPoints) {
                            minLevel += 10;
                            if (talentTreeCollection.activeTree().type == Engine::TreeType::CLASS) {
                                minLevel += (talentTreeCollection.activeSkillset()->talentPointsSpent - talentTreeCollection.activeTree().preFilledTalentPoints) * 2 - 2;
                            }
                            else {
                                minLevel += (talentTreeCollection.activeSkillset()->talentPointsSpent - talentTreeCollection.activeTree().preFilledTalentPoints) * 2 - 1;
                            }
                        }
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

    std::filesystem::path getAppPath() {
        std::filesystem::path path;
        PWSTR path_tmp;

        /* Attempt to get user's AppData folder
         *
         * This breaks Windows XP and earlier support!
         *
         * Microsoft Docs:
         * https://docs.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-shgetknownfolderpath
         * https://docs.microsoft.com/en-us/windows/win32/shell/knownfolderid
         */
        auto get_folder_path_ret = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path_tmp);

        /* Error check */
        if (get_folder_path_ret != S_OK) {
            CoTaskMemFree(path_tmp);
            throw std::system_error::exception("Could not open/find Roaming App Data path!");
        }

        /* Convert the Windows path type to a C++ path */
        path = path_tmp;
        std::filesystem::path appPath = path / "WoWTalentTreeManager";

        /* Free memory :) */
        CoTaskMemFree(path_tmp);

        //create app folder if it doesn't exist
        if (!std::filesystem::is_directory(appPath)) {
            std::filesystem::create_directory(appPath);
        }

        return appPath;
    }

    std::filesystem::path getCustomTreePath() {
        std::filesystem::path treePath = getAppPath() / "CustomTrees";

        //create custom tree folder if it doesn't exist
        if (!std::filesystem::is_directory(treePath)) {
            std::filesystem::create_directory(treePath);
        }

        return treePath;
    }

    void saveWorkspace(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        std::filesystem::path appPath = getAppPath();
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
        //TTMNOTE: settings can't have ":" char in them, messes with loadWorkspace
        switch (uiData.style) {
        case Presets::STYLES::COMPANY_GREY: {settings += "STYLE=COMPANY_GREY\n"; }break;
        case Presets::STYLES::PATH_OF_TALENT_TREE: {settings += "STYLE=PATH_OF_TALENT_TREE\n"; }break;
        case Presets::STYLES::LIGHT_MODE: {settings += "STYLE=LIGHT_MODE\n"; }break;
        }
        if (uiData.enableGlow) {
            settings += "GLOW=1\n";
        }
        else {
            settings += "GLOW=0\n";
        }
        for (TalentTreeData tree : talentTreeCollection.trees) {
            workspace += Engine::createTreeStringRepresentation(tree.tree) + "\n";
        }
        std::ofstream setFile(appPath / "settings.txt");
        setFile << settings;
        std::ofstream workFile(appPath / "workspace.txt");
        workFile << workspace;
    }

    TalentTreeCollection loadWorkspace(UIData& uiData) {
        std::filesystem::path appPath = getAppPath();
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
            }
        }
        //init talent tree collection
        TalentTreeCollection col;
        if (!std::filesystem::is_regular_file(appPath / "workspace.txt")) {
            return col;
        }
        std::ifstream workFile(appPath / "workspace.txt");
        std::string line;
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
        }
        return col;
    }

    void resetWorkspaceAndTrees() {
        std::filesystem::path appPath = getAppPath();
        std::filesystem::remove_all(appPath);
    }

    void useDefaultSettings(UIData& uiData) {
        Presets::SET_GUI_STYLE_LIGHT_MODE();
        uiData.style = Presets::STYLES::LIGHT_MODE;
        uiData.enableGlow = false;
    }
}