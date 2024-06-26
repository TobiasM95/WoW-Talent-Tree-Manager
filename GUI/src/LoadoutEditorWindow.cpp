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

#include "LoadoutEditorWindow.h"
#include "TalentTreeEditorWindow.h" // for screenshot TTMTODO: put screenshot function somewhere else
#include "TTMGUIPresets.h"

namespace TTM {
    static void AttachLoadoutEditTooltip(const UIData& uiData, Engine::Talent_s talent)
    {
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !ImGui::IsKeyDown(ImGuiKey_LeftAlt))
        {
            std::string idLabel = "Id: " + std::to_string(talent->index) + ", Pos: (" + std::to_string(talent->row) + ", " + std::to_string(talent->column) + ")";
            if (talent->type != Engine::TalentType::SWITCH) {
                ImGui::BeginTooltip();
                Presets::PUSH_FONT(uiData.fontsize, 1);
                ImGui::Text(talent->getName().c_str());
                Presets::POP_FONT();
                if (uiData.iconIndexMap.count(talent->index)) {
                    ImGui::Image(
                        uiData.iconIndexMap.at(talent->index).first->texture,
                        ImVec2(static_cast<float>(uiData.treeEditorBaseTalentSize), static_cast<float>(uiData.treeEditorBaseTalentSize))
                    );
                }
                //ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(idLabel.c_str()).x);
                ImGui::Text(idLabel.c_str());
                std::string talentTypeString;
                switch (talent->type) {
                case Engine::TalentType::ACTIVE: {talentTypeString = "(active)"; }break;
                case Engine::TalentType::PASSIVE: {talentTypeString = "(passive)"; }break;
                }
                ImGui::TextUnformattedColored(Presets::GET_TOOLTIP_TALENT_TYPE_COLOR(uiData.style), talentTypeString.c_str());
                if (talent->preFilled) {
                    ImGui::TextUnformattedColored(Presets::GET_TOOLTIP_TALENT_TYPE_COLOR(uiData.style), "(preselected)");
                }
                ImGui::Text(("Points: " + std::to_string(talent->points) + "/" + std::to_string(talent->maxPoints) + ", points required: " + std::to_string(talent->pointsRequired)).c_str());
                ImGui::Spacing();
                ImGui::Spacing();

                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 15.0f);
                ImGui::TextUnformattedColored(Presets::GET_TOOLTIP_TALENT_DESC_COLOR(uiData.style), talent->getDescription().c_str());
                ImGui::PopTextWrapPos();

                ImGui::EndTooltip();
            }
            else {
                if (talent->points > 0) {
                    ImGui::BeginTooltip();
                    Presets::PUSH_FONT(uiData.fontsize, 1);
                    ImGui::Text(talent->getName().c_str());
                    Presets::POP_FONT();
                    if (uiData.iconIndexMap.count(talent->index)) {
                        if (talent->talentSwitch == 1) {
                            ImGui::Image(
                                uiData.iconIndexMap.at(talent->index).first->texture, 
                                ImVec2(static_cast<float>(uiData.treeEditorBaseTalentSize), static_cast<float>(uiData.treeEditorBaseTalentSize))
                            );
                        }
                        else if (talent->talentSwitch == 2) {
                            ImGui::Image(
                                uiData.iconIndexMap.at(talent->index).second->texture, 
                                ImVec2(static_cast<float>(uiData.treeEditorBaseTalentSize), static_cast<float>(uiData.treeEditorBaseTalentSize))
                            );
                        }
                    }
                    //ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(idLabel.c_str()).x);
                    ImGui::Text(idLabel.c_str());
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 15.0f);
                    ImGui::TextUnformattedColored(Presets::GET_TOOLTIP_TALENT_TYPE_COLOR(uiData.style), "(switch, ctrl+click:");
                    ImGui::TextUnformattedColored(Presets::GET_TOOLTIP_TALENT_TYPE_COLOR(uiData.style), (talent->getNameSwitch() + ")").c_str());
                    ImGui::PopTextWrapPos();
                    if (talent->preFilled) {
                        ImGui::TextUnformattedColored(Presets::GET_TOOLTIP_TALENT_TYPE_COLOR(uiData.style), "(preselected)");
                    }
                    ImGui::Text(("Points: " + std::to_string(talent->points) + "/" + std::to_string(talent->maxPoints) + ", points required: " + std::to_string(talent->pointsRequired)).c_str());
                    ImGui::Spacing();
                    ImGui::Spacing();
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 15.0f);
                    ImGui::TextUnformattedColored(Presets::GET_TOOLTIP_TALENT_DESC_COLOR(uiData.style), talent->getDescription().c_str());
                    ImGui::PopTextWrapPos();
                }
                else {
                    ImGui::BeginTooltip();
                    Presets::PUSH_FONT(uiData.fontsize, 1);
                    ImGui::Text(talent->name.c_str());
                    Presets::POP_FONT();
                    if (uiData.iconIndexMap.count(talent->index)) {
                        ImGui::Image(
                            uiData.iconIndexMap.at(talent->index).first->texture, 
                            ImVec2(static_cast<float>(uiData.treeEditorBaseTalentSize), static_cast<float>(uiData.treeEditorBaseTalentSize))
                        );
                    }
                    //ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(idLabel.c_str()).x);
                    ImGui::Text(idLabel.c_str());
                    ImGui::TextColored(Presets::GET_TOOLTIP_TALENT_TYPE_COLOR(uiData.style), "(switch)");
                    if (talent->preFilled) {
                        ImGui::TextUnformattedColored(Presets::GET_TOOLTIP_TALENT_TYPE_COLOR(uiData.style), "(preselected)");
                    }
                    ImGui::Text(("Points: " + std::to_string(talent->points) + "/" + std::to_string(talent->maxPoints) + ", points required: " + std::to_string(talent->pointsRequired)).c_str());
                    
                    ImGui::Spacing();
                    ImGui::Spacing();
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 15.0f);
                    ImGui::TextUnformattedColored(Presets::GET_TOOLTIP_TALENT_DESC_COLOR(uiData.style), talent->descriptions[0].c_str());
                    ImGui::PopTextWrapPos();
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                    Presets::PUSH_FONT(uiData.fontsize, 1);
                    ImGui::Text(talent->nameSwitch.c_str());
                    Presets::POP_FONT();
                    if (uiData.iconIndexMap.count(talent->index)) {
                        ImGui::Image(
                            uiData.iconIndexMap.at(talent->index).second->texture,
                            ImVec2(static_cast<float>(uiData.treeEditorBaseTalentSize), static_cast<float>(uiData.treeEditorBaseTalentSize))
                        );
                    }
                    //ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(idLabel.c_str()).x);
                    ImGui::Text(idLabel.c_str());
                    ImGui::TextColored(Presets::GET_TOOLTIP_TALENT_TYPE_COLOR(uiData.style), "(switch)");
                    if (talent->preFilled) {
                        ImGui::TextUnformattedColored(Presets::GET_TOOLTIP_TALENT_TYPE_COLOR(uiData.style), "(preselected)");
                    }
                    ImGui::Text(("Points: " + std::to_string(talent->points) + "/" + std::to_string(talent->maxPoints) + ", points required: " + std::to_string(talent->pointsRequired)).c_str());
                    
                    ImGui::Spacing();
                    ImGui::Spacing();
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 15.0f);
                    ImGui::TextUnformattedColored(Presets::GET_TOOLTIP_TALENT_DESC_COLOR(uiData.style), talent->descriptions[1].c_str());
                    ImGui::PopTextWrapPos();
                }

                ImGui::EndTooltip();
            }
        }
    }

    void RenderLoadoutEditorWindow(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
        if (ImGui::Begin("TreeWindow", nullptr, ImGuiWindowFlags_NoDecoration)) {
            ImGuiWindowFlags treeWindowChildFlags = ImGuiWindowFlags_NoScrollbar;
            bool ctrlPressed = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
            if (ctrlPressed) {
                treeWindowChildFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
            }
            ImGui::BeginChild("TreeWindowChild", ImVec2(0, 0), true, treeWindowChildFlags);
            if (ImGui::IsItemActive())
            {
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                    ImGui::SetScrollX(ImGui::GetScrollX() - ImGui::GetIO().MouseDelta.x);
                    ImGui::SetScrollY(ImGui::GetScrollY() - ImGui::GetIO().MouseDelta.y);
                }
            }
            if (ImGui::IsWindowHovered() && ctrlPressed) {
                float mouseWheel = ImGui::GetIO().MouseWheel;
                if (mouseWheel != 0) {
                    float oldZoomFactor = uiData.treeEditorZoomFactor;
                    uiData.treeEditorZoomFactor += 0.2f * mouseWheel;
                    uiData.treeEditorZoomFactor = std::clamp(uiData.treeEditorZoomFactor, 0.5f, 3.0f);
                    if (oldZoomFactor != uiData.treeEditorZoomFactor) {
                        uiData.treeEditorWindowPos = ImGui::GetWindowPos();
                        uiData.treeEditorWindowSize = ImGui::GetWindowSize();
                        uiData.treeEditorMousePos = ImGui::GetMousePos();
                    }
                }
            }
            
            if (uiData.hoveredEditorSkillset) {
                drawSkillsetPreview(uiData, talentTreeCollection, uiData.hoveredEditorSkillset);
            }
            else if (uiData.hoveredBlizzHashCombo) {
                loadActiveIcons(uiData, uiData.hoveredBlizzHashCombo->first, -1, true);
                drawSkillsetPreview(uiData, uiData.hoveredBlizzHashCombo->first, uiData.hoveredBlizzHashCombo->second);
                loadActiveIcons(uiData, talentTreeCollection, true);
            }
            else {
                placeLoadoutEditorTreeElements(uiData, talentTreeCollection);
            }
            ImGui::EndChild();
        }
        ImGui::End();
        ImGui::PopStyleVar();
        if (ImGui::Begin("SettingsWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
            if (ImGui::Button("Loadout Information", ImVec2(ImGui::GetContentRegionAvail().x / 2.f, 25))) {
                uiData.loadoutEditPage = LoadoutEditPage::LoadoutInformation;
            }
            ImGui::SameLine();
            if (ImGui::Button("Import/Export Skillsets", ImVec2(ImGui::GetContentRegionAvail().x, 25))) {
                uiData.loadoutEditPage = LoadoutEditPage::SkillsetEdit;
            }
            ImGui::Spacing();
            switch (uiData.loadoutEditPage) {
            case LoadoutEditPage::LoadoutInformation: {
                ImGui::Text("Tree name: %s", talentTreeCollection.activeTree().name.c_str());

                ImGui::Text("Skillset count: %d", talentTreeCollection.activeTree().loadout.size());

                if (talentTreeCollection.activeTree().loadout.size() > 0) {
                    ImGui::Text("Skillset name:");
                    ImGui::InputText("##loadoutEditorSkillsetNameInput", &talentTreeCollection.activeSkillset()->name, 
                        ImGuiInputTextFlags_CallbackCharFilter, TextFilters::FilterNameLetters);
                    ImGui::Text("Set level cap:");
                    ImGui::SliderInt("##loadoutEditorLevelCapSlider", &talentTreeCollection.activeSkillset()->levelCap, 11, 70, "Level %d", ImGuiSliderFlags_AlwaysClamp);
                    bool disableLevelCapCheckbox = false;
                    if (Engine::getLevelRequirement(*talentTreeCollection.activeSkillset(), talentTreeCollection.activeTree())
                        > talentTreeCollection.activeSkillset()->levelCap) {
                        talentTreeCollection.activeSkillset()->useLevelCap = false;
                        disableLevelCapCheckbox = true;
                        ImGui::BeginDisabled();
                    }
                    ImGui::Checkbox("Activate level cap##loadoutEditorActivateLevelCapCheckbox", &talentTreeCollection.activeSkillset()->useLevelCap);
                    if (disableLevelCapCheckbox) {
                        ImGui::EndDisabled();
                    }
                    ImGui::Text("Skillset points spent: %d (+%d)", talentTreeCollection.activeSkillset()->talentPointsSpent - talentTreeCollection.activeTree().preFilledTalentPoints, talentTreeCollection.activeTree().preFilledTalentPoints);
                    int minLevel = Engine::getLevelRequirement(*talentTreeCollection.activeSkillset(), talentTreeCollection.activeTree());
                    ImGui::Text("Required level: %d", minLevel);
                    if (minLevel > 70) {
                        ImGui::Text("Required level is greater than current max level (70)!");
                    }
                    if (ImGui::Button("Reset skillset##loadoutEditorResetSkillsetButton")) {
                        talentTreeCollection.activeSkillset()->talentPointsSpent = 0;
                        for (auto& p : talentTreeCollection.activeSkillset()->assignedSkillPoints) {
                            p.second = 0;
                        }
                        for (auto& t : talentTreeCollection.activeTree().orderedTalents) {
                            t.second->points = 0;
                            t.second->talentSwitch = 0;
                        }
                    }
                }
                else {
                    ImGui::Text("Skillset name: [none]");
                    ImGui::Text("Skillset points spent: [none]");
                }

                ImGui::Text("Skillsets:");
                float boxHeight = ImGui::CalcTextSize("@").y;
                uiData.hoveredEditorSkillset = nullptr;
                if (ImGui::BeginListBox("##loadoutEditorSkillsetsListbox", ImVec2(ImGui::GetContentRegionAvail().x, 20 * boxHeight)))
                {
                    for (int n = 0; n < talentTreeCollection.activeTree().loadout.size(); n++)
                    {
                        const bool is_selected = (talentTreeCollection.activeTree().activeSkillsetIndex == n);
                        if (ImGui::Selectable((talentTreeCollection.activeTree().loadout[n]->name + "##" + std::to_string(n)).c_str(), is_selected)) {
                            talentTreeCollection.activeTree().activeSkillsetIndex = n;
                            Engine::activateSkillset(talentTreeCollection.activeTree(), n);
                        }

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();

                        if (ImGui::IsItemHovered()) {
                            uiData.hoveredEditorSkillset = std::make_shared<Engine::TalentSkillset>(*talentTreeCollection.activeTree().loadout[n]);
                        }
                    }
                    ImGui::EndListBox();
                }
                if (ImGui::Button("Add skillset##loadoutEditorAddSkillsetButton", ImVec2(ImGui::GetContentRegionAvail().x / 4.0f, 0))) {
                    Engine::createSkillset(talentTreeCollection.activeTree());
                    Engine::activateSkillset(talentTreeCollection.activeTree(), static_cast<int>(talentTreeCollection.activeTree().loadout.size() - 1));
                }
                ImGui::SameLine();
                if (ImGui::Button("Copy skillset##loadoutEditorAddSkillsetButton", ImVec2(ImGui::GetContentRegionAvail().x / 3.0f, 0))
                    && talentTreeCollection.activeTree().loadout.size() > 0 && talentTreeCollection.activeTree().activeSkillsetIndex >= 0) {
                    Engine::copySkillset(talentTreeCollection.activeTree(), talentTreeCollection.activeSkillset());
                    Engine::activateSkillset(talentTreeCollection.activeTree(), static_cast<int>(talentTreeCollection.activeTree().loadout.size() - 1));
                }
                ImGui::SameLine();
                if (ImGui::Button("Delete skillset##loadoutEditorDeleteSkillsetButton", ImVec2(ImGui::GetContentRegionAvail().x / 2.0f, 0))
                    && talentTreeCollection.activeTree().loadout.size() > 0 && talentTreeCollection.activeTree().activeSkillsetIndex >= 0) {
                    talentTreeCollection.activeTree().loadout.erase(talentTreeCollection.activeTree().loadout.begin() + talentTreeCollection.activeTree().activeSkillsetIndex);
                    if (talentTreeCollection.activeTree().loadout.size() == 0) {
                        talentTreeCollection.activeTree().activeSkillsetIndex = -1;
                    }
                    else {
                        talentTreeCollection.activeTree().activeSkillsetIndex = std::clamp(
                            talentTreeCollection.activeTree().activeSkillsetIndex,
                            0,
                            static_cast<int>(talentTreeCollection.activeTree().loadout.size() - 1));
                        Engine::activateSkillset(talentTreeCollection.activeTree(), talentTreeCollection.activeTree().activeSkillsetIndex);
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Delete all skillsets##loadoutEditorDeleteAllSkillsetsButton", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                    talentTreeCollection.activeTree().loadout.clear();
                    talentTreeCollection.activeTree().activeSkillsetIndex = -1;
                }

                ImGui::Text("Loadout description:");
                ImGui::InputTextMultiline("##LoadoutDescriptionInputText", &talentTreeCollection.activeTree().loadoutDescription,
                    ImVec2(ImGui::GetContentRegionAvail().x, 500.0f));
            }break;
            case LoadoutEditPage::SkillsetEdit: {
                if (ImGui::CollapsingHeader("Talent Tree Manager imports/exports"))
                {
                    ImGui::Text("Import skillsets:");
                    ImGui::InputText("##loadoutEditorImportSkillsetsInput", &uiData.loadoutEditorImportSkillsetsString);
                    ImGui::SameLine();
                    if (ImGui::Button("Import##treeEditorImportTalentTreeButton")) {
                        if (uiData.loadoutEditorImportSkillsetsString != "") {
                            uiData.loadoutEditorImportSkillsetsResult = Engine::importSkillsets(talentTreeCollection.activeTree(), uiData.loadoutEditorImportSkillsetsString);
                            ImGui::OpenPopup("Import skillsets result");
                        }
                    }
                    ImGui::Text("Export active skillset:");
                    ImGui::InputText("##loadoutEditorExportActiveSkillsetInput", &uiData.loadoutEditorExportActiveSkillsetString, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AutoSelectAll);
                    ImGui::SameLine();
                    if (ImGui::Button("Export##loadoutEditorExportActiveSkillsetButton")) {
                        if (talentTreeCollection.activeTree().activeSkillsetIndex >= 0) {
                            uiData.loadoutEditorExportActiveSkillsetString = Engine::createActiveSkillsetStringRepresentation(talentTreeCollection.activeTree());
                        }
                    }
                    ImGui::Text("Export all skillsets:");
                    ImGui::InputText("##loadoutEditorExportAllSkillsetsInput", &uiData.loadoutEditorExportAllSkillsetsString, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AutoSelectAll);
                    ImGui::SameLine();
                    if (ImGui::Button("Export##loadoutEditorExportAllSkillsetsButton")) {
                        if (talentTreeCollection.activeTree().loadout.size() > 0) {
                            uiData.loadoutEditorExportAllSkillsetsString = Engine::createAllSkillsetsStringRepresentation(talentTreeCollection.activeTree());
                        }
                    }

                    ImGui::Text("Screenshot of loadout:");
                    if (ImGui::Button("To clipboard##treeEditorScreenshotExportTalentTreeButton")) {
                        createScreenshotToClipboard(ImGui::FindWindowByName("TreeWindow")->WorkRect);
                    }
                }

                if (ImGui::CollapsingHeader("In-game Skillsets imports/exports"))
                {
                    ImGui::Text("Complementary tree/skillset:");
                    bool empty = (talentTreeCollection.activeTree().complementaryTreeIndex < 0
                        && talentTreeCollection.activeTree().complementarySkillsetIndex < 0);
                    uiData.hoveredBlizzHashCombo = nullptr;
                    if (ImGui::BeginCombo("##talentEditIconNameSwitchCombo", empty ? "Empty skillset" : uiData.loadoutEditorComplementarySelection.c_str()))
                    {
                        const bool empty_selected = talentTreeCollection.activeTree().complementaryTreeIndex < 0;
                        if (ImGui::Selectable("Empty skillset", empty_selected)) {
                            talentTreeCollection.activeTree().complementaryTreeIndex = -1;
                            talentTreeCollection.activeTree().complementarySkillsetIndex = -1;
                            uiData.loadoutEditorComplementarySelection = "Empty skillset";
                        }

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (empty_selected)
                            ImGui::SetItemDefaultFocus();

                        for (int i = 0; i < talentTreeCollection.trees.size(); i++)
                        {
                            auto& talentTreeData = talentTreeCollection.trees[i];
                            if (talentTreeData.tree.classID != talentTreeCollection.activeTree().classID
                                || talentTreeData.tree.type == talentTreeCollection.activeTree().type) {
                                continue;
                            }
                            for (int j = 0; j < talentTreeCollection.trees[i].tree.loadout.size(); j++) {
                                const bool is_selected = (
                                    talentTreeCollection.activeTree().complementaryTreeIndex == i &&
                                    talentTreeCollection.activeTree().complementarySkillsetIndex == j
                                    );
                                std::string displayText = talentTreeCollection.trees[i].tree.name + ": " + talentTreeCollection.trees[i].tree.loadout[j]->name;
                                if (ImGui::Selectable(displayText.c_str(), is_selected)) {
                                    talentTreeCollection.activeTree().complementaryTreeIndex = i;
                                    talentTreeCollection.activeTree().complementarySkillsetIndex = j;
                                    uiData.loadoutEditorComplementarySelection = displayText;
                                }

                                if (ImGui::IsItemHovered()) {
                                    uiData.hoveredBlizzHashCombo = std::make_shared<std::pair<Engine::TalentTree*, std::shared_ptr<Engine::TalentSkillset>>>(
                                        &talentTreeCollection.trees[i].tree,
                                        talentTreeCollection.trees[i].tree.loadout[j]
                                        );
                                }

                                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                                if (is_selected)
                                    ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }

                    ImGui::Text("Import ingame skillset string:");
                    if (talentTreeCollection.activeTree().type == Engine::TreeType::CLASS) {
                        ImGui::Checkbox("Import spec tree into selected tree too", &uiData.loadoutEditorImportBlizzardOtherTreeCheckbox);
                    }
                    else {
                        ImGui::Checkbox("Import class tree into selected tree too", &uiData.loadoutEditorImportBlizzardOtherTreeCheckbox);
                    }
                    ImGui::InputText("##loadoutEditorImportBlizzardHashInput", &uiData.loadoutEditorImportBlizzardHashString, ImGuiInputTextFlags_AutoSelectAll);
                    ImGui::SameLine();
                    if (ImGui::Button("Import##loadoutEditorImportBlizzardHashButton")) {
                        bool correctClassSpec = verifyTreeIDWithBlizzHash(talentTreeCollection.activeTree(), uiData.loadoutEditorImportBlizzardHashString);
                        if (correctClassSpec) {
                            Engine::TalentTree* compTreePtr = nullptr;
                            if (talentTreeCollection.activeTree().complementaryTreeIndex >= 0
                                && talentTreeCollection.activeTree().complementaryTreeIndex < talentTreeCollection.trees.size()) {
                                compTreePtr = &talentTreeCollection.trees[talentTreeCollection.activeTree().complementaryTreeIndex].tree;
                            }
                            bool success = Engine::importBlizzardHash(
                                talentTreeCollection.activeTree(),
                                compTreePtr,
                                uiData.loadoutEditorImportBlizzardHashString,
                                uiData.loadoutEditorImportBlizzardOtherTreeCheckbox
                            );
                            uiData.isLoadoutInitValidated = false;
                            if (success) {
                                uiData.loadoutEditPage = LoadoutEditPage::LoadoutInformation;
                                ImGui::OpenPopup("Import ingame skillset result");
                            }
                            else {
                                uiData.loadoutEditorImportBlizzardHashString = "Invalid import string!";
                            }
                        }
                        else {
                            uiData.loadoutEditorImportBlizzardHashString = "Wrong class/spec or invalid import string!";
                        }
                    }

                    ImGui::Text("Export ingame skillset string:");
                    ImGui::InputText("##loadoutEditorExportBlizzardHashInput", &uiData.loadoutEditorExportBlizzardHashString, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
                    ImGui::SameLine();
                    if (ImGui::Button("Export##loadoutEditorExportBlizzardHashButton")) {
                        Engine::TalentTree* compTreePtr = nullptr;
                        std::shared_ptr<Engine::TalentSkillset> compSsPtr = nullptr;
                        if (talentTreeCollection.activeTree().complementaryTreeIndex >= 0
                            && talentTreeCollection.activeTree().complementaryTreeIndex < talentTreeCollection.trees.size()) {
                            compTreePtr = &talentTreeCollection.trees[talentTreeCollection.activeTree().complementaryTreeIndex].tree;
                            if (talentTreeCollection.activeTree().complementarySkillsetIndex >= 0
                                && talentTreeCollection.activeTree().complementarySkillsetIndex < compTreePtr->loadout.size()) {
                                compSsPtr = compTreePtr->loadout[talentTreeCollection.activeTree().complementarySkillsetIndex];
                            }
                        }
                        Engine::exportBlizzardHash(
                            talentTreeCollection.activeTree(),
                            compTreePtr,
                            compSsPtr,
                            uiData.loadoutEditorExportBlizzardHashString
                        );
                    }
                }

                if (ImGui::CollapsingHeader("SimulationCraft exports"))
                {
                    ImGui::Text("Export active skillset to SimC:");
                    ImGui::Checkbox("Create profileset", &uiData.loadoutEditorExportActiveSkillsetSimcProfilesetCheckbox);
                    ImGui::InputText("##loadoutEditorExportActiveSkillsetSimcInput", &uiData.loadoutEditorExportActiveSkillsetSimcString, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AutoSelectAll);
                    ImGui::SameLine();
                    if (ImGui::Button("Export##loadoutEditorExportActiveSkillsetSimcButton")) {
                        if (talentTreeCollection.activeTree().activeSkillsetIndex >= 0) {
                            uiData.loadoutEditorExportActiveSkillsetSimcString = Engine::createActiveSkillsetSimcStringRepresentation(
                                talentTreeCollection.activeTree(), uiData.loadoutEditorExportActiveSkillsetSimcProfilesetCheckbox
                            );
                        }
                    }
                    ImGui::Text("Export all skillsets to SimC:");
                    ImGui::InputText("##loadoutEditorExportAllSkillsetsSimcInput", &uiData.loadoutEditorExportAllSkillsetsSimcString, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AutoSelectAll);
                    ImGui::SameLine();
                    if (ImGui::Button("Export##loadoutEditorExportAllSkillsetsSimcButton")) {
                        if (talentTreeCollection.activeTree().loadout.size() > 0) {
                            uiData.loadoutEditorExportAllSkillsetsSimcString = Engine::createAllSkillsetsSimcStringRepresentation(talentTreeCollection.activeTree());
                        }
                    }

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::Text("Create single talent comparison profilesets:");
                    ImGui::InputText("##loadoutEditorExportSingleTalentSkillsetsSimcInput", &uiData.loadoutEditorExportSingleTalentSkillsetsSimcString, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AutoSelectAll);
                    if (ImGui::Button("Export##loadoutEditorExportSingleTalentSkillsetsSimcButton")) {
                        uiData.loadoutEditorExportSingleTalentSkillsetsSimcString = Engine::createSingleTalentComparisonSimcString(talentTreeCollection.activeTree());
                    }
                }
            }break;
            }
            if (ImGui::BeginPopupModal("Import skillsets result", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Imported %d skillsets!\n(%d skillsets might have been discarded due to mismatched trees\nor corrupted import strings.)", 
                    uiData.loadoutEditorImportSkillsetsResult.first, uiData.loadoutEditorImportSkillsetsResult.second);
                if (uiData.loadoutEditorImportSkillsetsResult.first > 0) {
                    uiData.loadoutEditPage = LoadoutEditPage::LoadoutInformation;
                }
                ImGui::SetItemDefaultFocus();
                if (ImGui::Button("OK", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            if (ImGui::BeginPopupModal("Import ingame skillset result", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                if (uiData.loadoutEditorImportBlizzardOtherTreeCheckbox) {
                    ImGui::Text("Imported class and spec skillset from ingame export string.\nIf they're valid skillsets, you can view them in the respective loadout editor.");
                }
                else {
                    ImGui::Text("Imported skillset from ingame export string.\nIf it was a valid skillset, it was activated right now.");
                }

                ImGui::SetItemDefaultFocus();
                if (ImGui::Button("OK", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        ImGui::End();
        if (ImGui::Begin("SearchWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar)) {
            ImGui::Text("Search:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
            if (ImGui::InputText("##loadoutSolverSearchInput", &uiData.talentSearchString, ImGuiInputTextFlags_CallbackCharFilter, TextFilters::FilterNameLetters)) {
                //update the filteredTalentList
                uiData.searchedTalents.clear();

                Engine::filterTalentSearch(uiData.talentSearchString, uiData.searchedTalents, talentTreeCollection.activeTree());
            }
            ImGui::SameLine();
            ImGui::TextUnformatted("(also accepts \"active\", \"passive\", \"switch\")");
        }
        ImGui::End();
    }

    void placeLoadoutEditorTreeElements(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        Engine::TalentTree& tree = talentTreeCollection.activeTree();

        if (tree.loadout.size() == 0) {
            ImGui::SetCursorPos(ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 75.0f, ImGui::GetContentRegionAvail().y * 0.5f - 12.5f));
            if (ImGui::Button("Create first skillset", ImVec2(150, 25))) {
                Engine::createSkillset(tree);
                Engine::activateSkillset(tree, static_cast<int>(tree.loadout.size() - 1));
            }
            return;
        }
        else if (tree.activeSkillsetIndex < 0) {
            //this should in theory not happen but failsave never hurts
            Engine::activateSkillset(tree, 0);
        }

        int talentHalfSpacing = static_cast<int>(uiData.treeEditorBaseTalentHalfSpacing * uiData.treeEditorZoomFactor);
        int talentSize = static_cast<int>(uiData.treeEditorBaseTalentSize * uiData.treeEditorZoomFactor);
        float talentWindowPaddingX = static_cast<float>(uiData.treeEditorTalentWindowPaddingX);
        float talentWindowPaddingY = static_cast<float>(uiData.treeEditorTalentWindowPaddingY);
        ImVec2 origin = ImVec2(talentWindowPaddingX, talentWindowPaddingY);
        //calculate full tree width and if that is < window width center tree
        float fullTreeWidth = (tree.maxCol - 1) * 2.0f * talentHalfSpacing;
        float windowWidth = ImGui::GetContentRegionAvail().x;
        if (fullTreeWidth + 2 * origin.x < windowWidth) {
            origin.x = 0.5f * (windowWidth - fullTreeWidth);
        }

        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 scrollOffset = ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY());

        int maxRow = 0;

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImGuiStyle& imStyle = ImGui::GetStyle();


        for (auto& talent : tree.orderedTalents) {
            for (auto& child : talent.second->children) {
                bool talentDisabled = false;
                if (talent.second->pointsRequired > talentTreeCollection.activeSkillset()->talentPointsSpent - talentTreeCollection.activeTree().preFilledTalentPoints
                    || talent.second->preFilled
                    || talent.second->points != talent.second->maxPoints
                    || (talentTreeCollection.activeSkillset()->useLevelCap
                        && Engine::getLevelRequirement(*talentTreeCollection.activeSkillset(), talentTreeCollection.activeTree(), 1) > talentTreeCollection.activeSkillset()->levelCap
                        && talent.second->points == 0)) {
                    talentDisabled = true;
                }
                if (child->pointsRequired > talentTreeCollection.activeSkillset()->talentPointsSpent - talentTreeCollection.activeTree().preFilledTalentPoints
                    || child->preFilled
                    || (talentTreeCollection.activeSkillset()->useLevelCap
                        && Engine::getLevelRequirement(*talentTreeCollection.activeSkillset(), talentTreeCollection.activeTree(), 1) > talentTreeCollection.activeSkillset()->levelCap
                        && child->points == 0)) {
                    talentDisabled = true;
                }
                drawArrowBetweenTalents(
                    talent.second,
                    child,
                    drawList,
                    windowPos,
                    scrollOffset,
                    origin,
                    talentHalfSpacing,
                    talentSize,
                    0.0f,
                    uiData,
                    true,
                    talentDisabled ? 0.2f : 1.0f);
            }
        }
        for (auto& reqInfo : tree.requirementSeparatorInfo) {
            ImVec4 separatorColor = Presets::GET_TOOLTIP_TALENT_DESC_COLOR(uiData.style);
            if (talentTreeCollection.activeSkillset()->talentPointsSpent - tree.preFilledTalentPoints >= reqInfo.first) {
                separatorColor.w = 0.4f;
            }
            drawList->AddLine(
                ImVec2(windowPos.x - scrollOffset.x + origin.x - 2 * talentSize, windowPos.y - scrollOffset.y + talentWindowPaddingY + (reqInfo.second - 1) * talentSize),
                ImVec2(windowPos.x - scrollOffset.x + origin.x + (tree.maxCol + 1) * talentSize, windowPos.y - scrollOffset.y + talentWindowPaddingY + (reqInfo.second - 1) * talentSize),
                ImColor(separatorColor),
                2.0f
            );
            drawList->AddText(
                ImVec2(windowPos.x - scrollOffset.x + origin.x - 2 * talentSize, windowPos.y - scrollOffset.y + talentWindowPaddingY + (reqInfo.second - 1) * talentSize),
                ImColor(separatorColor),
                (std::to_string(talentTreeCollection.activeSkillset()->talentPointsSpent - tree.preFilledTalentPoints) + " / " + std::to_string(reqInfo.first) + " points").c_str()
            );
        }

        for (auto& talent : tree.orderedTalents) {
            maxRow = talent.second->row > maxRow ? talent.second->row : maxRow;
            float posX = origin.x + (talent.second->column - 1) * 2 * talentHalfSpacing;
            float posY = origin.y + (talent.second->row - 1) * 2 * talentHalfSpacing;
            bool talentIsSearchedFor = false;
            bool searchActive = uiData.talentSearchString != "";
            bool talentDisabled = false;
            bool isParentFilled = talent.second->parents.size() == 0;
            for (auto& parent : talent.second->parents) {
                if (parent->points == parent->maxPoints) {
                    isParentFilled = true;
                    break;
                }
            }
            if (talent.second->pointsRequired > talentTreeCollection.activeSkillset()->talentPointsSpent - talentTreeCollection.activeTree().preFilledTalentPoints 
                || !isParentFilled || talent.second->preFilled
                || (talentTreeCollection.activeSkillset()->useLevelCap 
                    && Engine::getLevelRequirement(*talentTreeCollection.activeSkillset(), talentTreeCollection.activeTree(), 1) > talentTreeCollection.activeSkillset()->levelCap
                    && talent.second->points == 0)) {
                talentDisabled = true;
            }
            if (talent.second->preFilled) {
                int assignedPoints = talent.second->maxPoints - talent.second->points;
                talent.second->points += assignedPoints;
                talentTreeCollection.activeSkillset()->assignedSkillPoints[talent.first] += assignedPoints;
                talentTreeCollection.activeSkillset()->talentPointsSpent += assignedPoints;
                if (talent.second->type == Engine::TalentType::SWITCH) {
                    talent.second->talentSwitch = 1;
                }
            }
            ImGui::SetCursorPos(ImVec2(posX, posY));
            Presets::PUSH_FONT(uiData.fontsize, 1);
            if (talentDisabled) {
                ImGui::GetCurrentContext()->Style.DisabledAlpha = 0.20f;
                ImGui::BeginDisabled();
            }

            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
            ImGui::PushID((std::to_string(talent.second->points) + "/" + std::to_string(talent.second->maxPoints) + "##" + std::to_string(talent.second->index)).c_str());
            TextureInfo* iconContent = nullptr;
            TextureInfo* iconContentChoice = nullptr;
            if (talent.second->type == Engine::TalentType::SWITCH) {
                if (talentTreeCollection.activeSkillset()->assignedSkillPoints[talent.first] > 0) {
                    if (talent.second->talentSwitch == 2) {
                        iconContent = uiData.iconIndexMap[talent.second->index].second;
                    }
                    else {
                        iconContent = uiData.iconIndexMap[talent.second->index].first;
                    }
                }
                else {
                    iconContent = uiData.iconIndexMapGrayed[talent.second->index].first;
                    iconContentChoice = uiData.iconIndexMapGrayed[talent.second->index].second;
                }
            }
            else {
                if (talentTreeCollection.activeSkillset()->assignedSkillPoints[talent.first] > 0) {
                    iconContent = uiData.iconIndexMap[talent.second->index].first;
                }
                else {
                    iconContent = uiData.iconIndexMapGrayed[talent.second->index].first;
                }
            }
            if (ImGui::InvisibleButton(("##invisTalentButton" + std::to_string(talent.first)).c_str(),
                ImVec2(static_cast<float>(talentSize), static_cast<float>(talentSize)))) {
                if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)) {
                    if (talent.second->type == Engine::TalentType::SWITCH) {
                        talent.second->talentSwitch = talent.second->talentSwitch < 2 ? 2 : 1;
                        if (talent.second->points > 0) {
                            talentTreeCollection.activeSkillset()->assignedSkillPoints[talent.first] = talent.second->talentSwitch;
                        }
                    }
                }
                else {
                    if (isParentFilled 
                        && talentTreeCollection.activeSkillset()->talentPointsSpent - talentTreeCollection.activeTree().preFilledTalentPoints >= talent.second->pointsRequired 
                        && talent.second->points < talent.second->maxPoints
                        && (!talentTreeCollection.activeSkillset()->useLevelCap
                        || Engine::getLevelRequirement(*talentTreeCollection.activeSkillset(), talentTreeCollection.activeTree(), 1) <= talentTreeCollection.activeSkillset()->levelCap)) {
                        talent.second->points += 1;
                        if (talent.second->type == Engine::TalentType::SWITCH) {
                            if (talent.second->talentSwitch == 0) {
                                talent.second->talentSwitch = 1;
                            }
                            talentTreeCollection.activeSkillset()->assignedSkillPoints[talent.first] = talent.second->talentSwitch;
                        }
                        else {
                            talentTreeCollection.activeSkillset()->assignedSkillPoints[talent.first] += 1;
                        }
                        talentTreeCollection.activeSkillset()->talentPointsSpent += 1;
                    }
                }
            }
            ImGui::PopID();
            ImGui::PopStyleColor(5);
            Presets::POP_FONT();
            if (talentDisabled) {
                ImGui::GetCurrentContext()->Style.DisabledAlpha = 0.6f;
                ImGui::EndDisabled();
            }
            AttachLoadoutEditTooltip(uiData, talent.second);
            if (talentDisabled) {
                ImGui::GetCurrentContext()->Style.DisabledAlpha = 0.20f;
                ImGui::BeginDisabled();
            }
            if (talent.second->type != Engine::TalentType::SWITCH || talentTreeCollection.activeSkillset()->assignedSkillPoints[talent.first] > 0) {
                ImGui::SetCursorPos(ImVec2(posX, posY));
                ImGui::Image(
                    iconContent->texture,
                    ImVec2(static_cast<float>(talentSize), static_cast<float>(talentSize)), ImVec2(0, 0), ImVec2(1, 1)
                );
            }
            else {
                float separatorWidth = 0.05f;
                ImGui::SetCursorPos(ImVec2(posX, posY));
                ImGui::Image(
                    iconContent->texture,
                    ImVec2(talentSize * (1.0f - separatorWidth) / 2.0f, static_cast<float>(talentSize)), ImVec2(0, 0), ImVec2((1.0f - separatorWidth) / 2.0f, 1)
                );

                ImGui::SetCursorPos(ImVec2(posX + talentSize * (1.0f + separatorWidth) / 2.0f, posY));
                ImGui::Image(
                    iconContentChoice->texture,
                    ImVec2(talentSize* (1.0f - separatorWidth) / 2.0f, static_cast<float>(talentSize)), ImVec2((1.0f + separatorWidth) / 2.0f, 0), ImVec2(1, 1)
                );
            }

            //mask has to be displayed without alpha
            ImGui::SetCursorPos(ImVec2(posX, posY));
            if (talentDisabled) {
                ImGui::GetCurrentContext()->Style.DisabledAlpha = 0.6f;
                ImGui::EndDisabled();
            }
            ImGui::Image(
                uiData.talentIconMasks[static_cast<int>(uiData.style)][static_cast<int>(talent.second->type)].texture,
                ImVec2(static_cast<float>(talentSize), static_cast<float>(talentSize)), ImVec2(0, 0), ImVec2(1, 1)
            );
            ImGui::SetCursorPos(ImVec2(
                posX - 0.5f * (uiData.treeEditorZoomFactor * uiData.redIconGlow[static_cast<int>(talent.second->type)].width - talentSize),
                posY - 0.5f * (uiData.treeEditorZoomFactor * uiData.redIconGlow[static_cast<int>(talent.second->type)].height - talentSize)));
            if (uiData.enableGlow && !searchActive) {
                if (talent.second->points == talent.second->maxPoints) {
                    ImGui::Image(
                        uiData.goldIconGlow[static_cast<int>(talent.second->type)].texture,
                        ImVec2(
                            uiData.treeEditorZoomFactor * uiData.goldIconGlow[static_cast<int>(talent.second->type)].width,
                            uiData.treeEditorZoomFactor * uiData.goldIconGlow[static_cast<int>(talent.second->type)].height),
                        ImVec2(0, 0), ImVec2(1, 1),
                        ImVec4(1, 1, 1, 1.0f - 0.5f * (uiData.style == Presets::STYLES::COMPANY_GREY))
                    );
                }
                else if (talent.second->points > 0) {
                    ImGui::Image(
                        uiData.greenIconGlow[static_cast<int>(talent.second->type)].texture,
                        ImVec2(
                            uiData.treeEditorZoomFactor * uiData.greenIconGlow[static_cast<int>(talent.second->type)].width,
                            uiData.treeEditorZoomFactor * uiData.greenIconGlow[static_cast<int>(talent.second->type)].height),
                        ImVec2(0, 0), ImVec2(1, 1),
                        ImVec4(1, 1, 1, 1.0f - 0.5f * (uiData.style == Presets::STYLES::COMPANY_GREY))
                    );
                }
            }
            else if (uiData.talentSearchString != "" && std::find(uiData.searchedTalents.begin(), uiData.searchedTalents.end(), talent.second) != uiData.searchedTalents.end()) {
                talentIsSearchedFor = true;
                ImGui::Image(
                    uiData.blueIconGlow[static_cast<int>(talent.second->type)].texture,
                    ImVec2(
                        uiData.treeEditorZoomFactor * uiData.blueIconGlow[static_cast<int>(talent.second->type)].width,
                        uiData.treeEditorZoomFactor * uiData.blueIconGlow[static_cast<int>(talent.second->type)].height),
                    ImVec2(0, 0), ImVec2(1, 1),
                    ImVec4(1, 1, 1, 1.0f - 0.5f * (uiData.style == Presets::STYLES::COMPANY_GREY))
                );
            }
            if (talentDisabled) {
                ImGui::GetCurrentContext()->Style.DisabledAlpha = 0.20f;
                ImGui::BeginDisabled();
            }

            drawLoadoutEditorShapeAroundTalent(
                talent.second, 
                drawList,
                imStyle.Colors,
                ImVec2(posX, posY), 
                talentSize, 
                ImGui::GetWindowPos(), 
                ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY()), 
                uiData, 
                talentTreeCollection,
                1.0f - 0.8f * talentDisabled,
                searchActive,
                talentIsSearchedFor);
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                uiData.loadoutEditorRightClickIndex = talent.first;
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Middle)) {
                uiData.loadoutEditorMiddleClickIndex = talent.first;
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right) && talent.first == uiData.loadoutEditorRightClickIndex) {
                if (talent.second->points > 0) {
                    talent.second->points -= 1;
                    if (talent.second->type == Engine::TalentType::SWITCH) {
                        talentTreeCollection.activeSkillset()->assignedSkillPoints[talent.first] = 0;
                    }
                    else {
                        talentTreeCollection.activeSkillset()->assignedSkillPoints[talent.first] -= 1;
                    }
                    talentTreeCollection.activeSkillset()->talentPointsSpent -= 1;
                    bool allTalentsValid = Engine::checkTalentValidity(talentTreeCollection.activeTree());
                    if (!allTalentsValid) {
                        talent.second->points += 1;
                        if (talent.second->type == Engine::TalentType::SWITCH) {
                            talentTreeCollection.activeSkillset()->assignedSkillPoints[talent.first] = talent.second->talentSwitch;
                        }
                        else {
                            talentTreeCollection.activeSkillset()->assignedSkillPoints[talent.first] += 1;
                        }
                        talentTreeCollection.activeSkillset()->talentPointsSpent += 1;
                    }
                }
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Middle) && talent.first == uiData.loadoutEditorMiddleClickIndex) {
                if (talent.second->type == Engine::TalentType::SWITCH) {
                    talent.second->talentSwitch = talent.second->talentSwitch < 2 ? 2 : 1;
                    if (talent.second->points > 0) {
                        talentTreeCollection.activeSkillset()->assignedSkillPoints[talent.first] = talent.second->talentSwitch;
                    }
                }
            }
            if (talentDisabled) {
                ImGui::GetCurrentContext()->Style.DisabledAlpha = 0.6f;
                ImGui::EndDisabled();
            }
        }
        if (ImGui::IsWindowHovered() && (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift))
            && !(ImGui::IsKeyDown(ImGuiKey_LeftSuper) || ImGui::IsKeyDown(ImGuiKey_RightSuper))) {
            for (auto& talent : tree.orderedTalents) {
                float posX = origin.x + (talent.second->column - 1) * 2 * talentHalfSpacing;
                float posY = origin.y + (talent.second->row - 1) * 2 * talentHalfSpacing;
                ImVec2 textBoxPos = ImVec2(
                    posX - 0.5f * talentSize + ImGui::GetWindowPos().x - ImGui::GetScrollX(),
                    posY - 0.5f * talentSize + ImGui::GetWindowPos().y - ImGui::GetScrollY()
                );
                ImVec2 bounds = ImVec2(textBoxPos.x + 2.0f * talentSize, textBoxPos.y + 2.0f * talentSize);
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                draw_list->AddRectFilled(textBoxPos, bounds, IM_COL32(0, 0, 0, 255));
                draw_list->AddRect(textBoxPos, bounds, IM_COL32(255, 255, 255, 255), 0, 0, 2.0f);
                std::string infoText = talent.second->type == Engine::TalentType::SWITCH ? talent.second->getName() + " / " + talent.second->getNameSwitch() : talent.second->getName();
                Presets::PUSH_FONT(uiData.fontsize, 3);
                AddWrappedText(infoText, textBoxPos, 5.0f, ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 2.0f * talentSize, 2.0f * talentSize, ImGui::GetWindowDrawList());
                Presets::POP_FONT();
            }
        }

        ImGui::SetCursorPos(ImVec2(origin.x, 1.5f * origin.y + maxRow * 2 * talentHalfSpacing));
        ImGui::InvisibleButton(
            "##invisbuttonedit",
            ImVec2(
                origin.x + (tree.maxCol - 1) * 2 * talentHalfSpacing,
                0.5f * origin.y
            )
        );

        if ((uiData.maxScrollBuffer.x != ImGui::GetScrollMaxX() || uiData.maxScrollBuffer.y != ImGui::GetScrollMaxY())
            && uiData.treeEditorWindowSize.x != 0 && uiData.treeEditorWindowSize.y != 0) {
            uiData.treeEditorRelWorldMousePos = ImVec2(
                (uiData.scrollBuffer.x + (uiData.treeEditorMousePos.x - uiData.treeEditorWindowPos.x)) / (uiData.maxScrollBuffer.x + uiData.treeEditorWindowSize.x),
                (uiData.scrollBuffer.y + (uiData.treeEditorMousePos.y - uiData.treeEditorWindowPos.y)) / (uiData.maxScrollBuffer.y + uiData.treeEditorWindowSize.y)
            );
            ImVec2 targetPosAbs = ImVec2(
                uiData.treeEditorRelWorldMousePos.x * (ImGui::GetScrollMaxX() + uiData.treeEditorWindowSize.x),
                uiData.treeEditorRelWorldMousePos.y * (ImGui::GetScrollMaxY() + uiData.treeEditorWindowSize.y)
            );
            ImVec2 targetScreenPosAbs = ImVec2(
                targetPosAbs.x - (uiData.treeEditorMousePos.x - uiData.treeEditorWindowPos.x),
                targetPosAbs.y - (uiData.treeEditorMousePos.y - uiData.treeEditorWindowPos.y)
            );
            ImGui::SetScrollX(targetScreenPosAbs.x);
            ImGui::SetScrollY(targetScreenPosAbs.y);

            uiData.maxScrollBuffer = ImVec2(ImGui::GetScrollMaxX(), ImGui::GetScrollMaxY());
            uiData.scrollBuffer.x = std::clamp(targetScreenPosAbs.x, 0.0f, uiData.maxScrollBuffer.x);
            uiData.scrollBuffer.y = std::clamp(targetScreenPosAbs.y, 0.0f, uiData.maxScrollBuffer.y);
        }
    }
}