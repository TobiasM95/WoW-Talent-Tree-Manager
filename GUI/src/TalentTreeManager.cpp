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

#include "TalentTreeEditorWindow.h"
#include "LoadoutEditorWindow.h"
#include "LoadoutSolverWindow.h"

//TTMTODO: place different components into their own files, lots of cleanup
//TTMTODO: Replace all talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree with talentTreeCollection.activeTree()
namespace TTM {
    

	void RenderMainWindow(UIData& uiData, TalentTreeCollection& talentTreeCollection, bool& done) {

		RenderMenuBar(uiData, talentTreeCollection, done);
        RenderStatusBar(uiData, talentTreeCollection);
        RenderWorkArea(uiData, talentTreeCollection);
        //ImGui::ShowMetricsWindow(nullptr);
	}

	void RenderMenuBar(UIData& uiData, TalentTreeCollection& talentTreeCollection, bool& done) {
		if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Save")) {
                    saveWorkspace(uiData, talentTreeCollection);
                }
                if (ImGui::MenuItem("Close")) {
                    done = true;
                }
                ImGui::EndMenu();
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
                if (ImGui::MenuItem("Light Mode (yuck)")) {
                    Presets::SET_GUI_STYLE(Presets::STYLES::LIGHT_MODE);
                    uiData.style = Presets::STYLES::LIGHT_MODE;
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
                ImGui::EndMenu();
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
        if (ImGui::BeginPopupModal("About##Popup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Feedback and suggestions: BuffMePls#2973 (Discord)");
            ImGui::Text("Github: https://github.com/TobiasM95/WoW-Talent-Tree-Manager");
            ImGui::Text("Dear ImGui: https://github.com/ocornut/imgui");

            ImGui::SetItemDefaultFocus();
            if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }
        static bool* close = new bool();
        *close = true;
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
            ImGui::Text("The left side shows the tree and all currently selected talents (and for switch talents the selected talent). Dragging the window around as well as zooming works the same way as the talent tree editor. You can select talents just like ingame by clicking on the talent. By pressing left Ctrl and clicking on a switch talent you can change the variant.");
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
            ImGui::PopTextWrapPos();

            ImGui::SetItemDefaultFocus();
            if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
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

    void saveWorkspace(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        std::filesystem::path appPath = getAppPath();
        std::string workspace = "";
        switch (uiData.style) {
        case Presets::STYLES::COMPANY_GREY: {workspace += "COMPANY_GREY\n"; }break;
        case Presets::STYLES::PATH_OF_TALENT_TREE: {workspace += "PATH_OF_TALENT_TREE\n"; }break;
        case Presets::STYLES::LIGHT_MODE: {workspace += "LIGHT_MODE\n"; }break;
        }
        for (TalentTreeData tree : talentTreeCollection.trees) {
            workspace += Engine::createTreeStringRepresentation(tree.tree) + "\n";
        }
        std::ofstream workFile(appPath / "workspace.txt");
        workFile << workspace;
    }

    TalentTreeCollection loadWorkspace(UIData& uiData) {
        std::filesystem::path appPath = getAppPath();
        TalentTreeCollection col;
        if (!std::filesystem::is_regular_file(appPath / "workspace.txt")) {
            return col;
        }
        std::ifstream workFile(appPath / "workspace.txt");
        std::string line;
        bool firstLine = false;
        while (std::getline(workFile, line))
        {
            if (!firstLine) {
                firstLine = true;
                if (line == "COMPANY_GREY") {
                    Presets::SET_GUI_STYLE_COMPANY_GREY();
                    uiData.style = Presets::STYLES::COMPANY_GREY;
                }
                else if (line == "PATH_OF_TALENT_TREE") {
                    Presets::SET_GUI_STYLE_PATH_OF_TALENT_TREE();
                    uiData.style = Presets::STYLES::PATH_OF_TALENT_TREE;
                }
                else if (line == "LIGHT_MODE") {
                    Presets::SET_GUI_STYLE_LIGHT_MODE();
                    uiData.style = Presets::STYLES::LIGHT_MODE;
                }
                continue;
            }
            if (line == "") {
                break;
            }
            TalentTreeData data;
            data.tree = Engine::parseTree(line);
            col.trees.push_back(data);
            col.activeTreeIndex = static_cast<int>(col.trees.size() - 1);
        }
        return col;
    }
}