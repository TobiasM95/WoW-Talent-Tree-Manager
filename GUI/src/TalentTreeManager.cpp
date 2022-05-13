#include "TalentTreeManager.h"

#include <windows.h>
#include <shlobj.h>

#include <algorithm>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"

#include "TTMGUIPresets.h"
#include "TTMEnginePresets.h"

#include "TalentTreeEditorWindow.h"
#include "LoadoutEditorWindow.h"
#include "LoadoutSolverWindow.h"

//TTMTODO: place different components into their own files, lots of cleanup
//TTMTODO: Replace all talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree with talentTreeCollection.activeTree()
namespace TTM {
    

	void RenderMainWindow(UIData& uiData, TalentTreeCollection& talentTreeCollection) {

		RenderMenuBar(uiData);
        RenderStatusBar(uiData, talentTreeCollection);
        RenderWorkArea(uiData, talentTreeCollection);
        //ImGui::ShowMetricsWindow(nullptr);
	}

	void RenderMenuBar(UIData& uiData) {
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("Styles")) {
                if (ImGui::MenuItem("Company Grey")) {
                    Presets::SET_GUI_STYLE(Presets::STYLES::COMPANY_GREY);
                }
                if (ImGui::MenuItem("Path of Talent Tree")) {
                    Presets::SET_GUI_STYLE(Presets::STYLES::PATH_OF_TALENT_TREE);
                }
                if (ImGui::MenuItem("Light Mode (yuck)")) {
                    Presets::SET_GUI_STYLE(Presets::STYLES::LIGHT_MODE);
                }
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
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
            if (ImGui::TabItemButton("X", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip))
                ImGui::OpenPopup("CloseAllConfirmation");

            if (ImGui::BeginPopupModal("Create new tree", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                if (ImGui::Button("New custom tree", ImVec2(-0.01f, 0))) {
                    TalentTreeData tree;
                    tree.tree = Engine::loadTreePreset(Presets::RETURN_PRESET_BY_NAME("new_custom_tree"));

                    talentTreeCollection.trees.push_back(tree);
                    talentTreeCollection.activeTreeIndex = static_cast<int>(talentTreeCollection.trees.size() - 1);
                    ImGui::CloseCurrentPopup();
                }
                //ImGui::SameLine();
                ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("or").x) * 0.5f);
                ImGui::Text("or");
                //ImGui::SameLine();
                ImGui::Combo("##treeEditorCreationPresetClassCombo", &uiData.treeEditorPresetClassCombo, Presets::CLASSES, IM_ARRAYSIZE(Presets::CLASSES));
                //ImGui::SameLine();
                ImGui::Combo(
                    "##treeEditorCreationPresetSpecCombo",
                    &uiData.treeEditorPresetSpecCombo,
                    Presets::RETURN_SPECS(uiData.treeEditorPresetClassCombo),
                    IM_ARRAYSIZE(Presets::RETURN_SPECS(uiData.treeEditorPresetClassCombo))
                );
                //ImGui::SameLine();
                if (ImGui::Button("Load preset", ImVec2(-0.01f, 0))) { 
                    TalentTreeData tree;
                    tree.tree = Engine::loadTreePreset(Presets::RETURN_PRESET(uiData.treeEditorPresetClassCombo, uiData.treeEditorPresetSpecCombo));
                    talentTreeCollection.trees.push_back(tree);
                    talentTreeCollection.activeTreeIndex = static_cast<int>(talentTreeCollection.trees.size() - 1);

                    ImGui::CloseCurrentPopup(); 
                }
                ImGui::EndPopup();
            }
            if (ImGui::BeginPopup("CloseAllConfirmation"))
            {
                ImGui::Text("Close all tabs?");
                ImGui::SameLine();
                if (ImGui::Button("Yes")) {
                    talentTreeCollection.trees.clear();
                    talentTreeCollection.activeTreeIndex = -1;
                    uiData.editorView = EditorView::None;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            // Submit our regular tabs
            size_t deleteTreeIndex = talentTreeCollection.trees.size();
            for (int n = 0; n < talentTreeCollection.trees.size(); )
            {
                bool open = true;
                ImGuiTabItemFlags flag = ImGuiTabItemFlags_None;
                if (ImGui::BeginTabItem((talentTreeCollection.trees[n].tree.name + "##" + std::to_string(n)).c_str(), &open, flag))
                {
                    talentTreeCollection.activeTreeIndex = n;
                    RenderTreeViewTabs(uiData, talentTreeCollection);
                    ImGui::EndTabItem();
                }

                if (!open) {
                    deleteTreeIndex = n;
                }
                else
                    n++;
            }
            if (deleteTreeIndex < talentTreeCollection.trees.size()) {
                talentTreeCollection.trees.erase(talentTreeCollection.trees.begin() + deleteTreeIndex);
                talentTreeCollection.activeTreeIndex = static_cast<int>(talentTreeCollection.trees.size() - 1);
            }
            if (talentTreeCollection.trees.size() == 0) {
                uiData.editorView = EditorView::None;
            }

            ImGui::EndTabBar();
        }
    }

    void RenderTreeViewTabs(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        // Expose some other flags which are useful to showcase how they interact with Leading/Trailing tabs
        static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_FittingPolicyScroll;

        if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
        {
            if (ImGui::BeginTabItem("Talent Tree Editor", nullptr, ImGuiTabItemFlags_None))
            {
                uiData.editorView = EditorView::TreeEdit;
                uiData.isLoadoutInitValidated = false;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Talent Loadout Editor", nullptr, ImGuiTabItemFlags_None))
            {
                uiData.editorView = EditorView::LoadoutEdit;
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
            if (ImGui::BeginTabItem("Talent Loadout Solver", nullptr, ImGuiTabItemFlags_None))
            {
                uiData.editorView = EditorView::LoadoutSolver;
                uiData.isLoadoutInitValidated = false;
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }

    void SubmitDockSpace() {
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoResizeX;

        ImGuiID dockspace_id = ImGui::GetID("WorkAreaDockspace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

        static bool first = true;
        if (first) {
            first = false;
            ImGui::DockBuilderRemoveNode(dockspace_id); // Clear out existing layout
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace); // Add empty node
            ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);//ImGui::GetCurrentWindow()->Size);

            ImGuiID dock_main_id = dockspace_id; // This variable will track the document node, however we are not using it here as we aren't docking anything into it.
            ImGuiID dock_id_left, dock_id_right;
            ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.40f, &dock_id_right, &dock_id_left);

            ImGui::DockBuilderDockWindow("TreeWindow", dock_id_left);
            ImGui::DockBuilderDockWindow("SettingsWindow", dock_id_right);
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
        //TTMTODO: Implement different statusbar texts for different views
		if (ImGui::BeginViewportSideBar("MainStatusBar", NULL, ImGuiDir_Down, ImGui::GetFrameHeight(), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar)) {
			if (ImGui::BeginMenuBar()) {
                std::string text = "Currently active tree: ";
                text += talentTreeCollection.activeTreeIndex < 0 ? "none" : talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.name;
                if (talentTreeCollection.activeTreeIndex >= 0) {
                    text += " - node count: " + std::to_string(talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.nodeCount);
                    text += " - maximum skill points : " + std::to_string(talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.maxTalentPoints);
                }
				ImGui::Text(text.c_str());
				ImGui::EndMenuBar();
			}
		}
        ImGui::End();
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
}