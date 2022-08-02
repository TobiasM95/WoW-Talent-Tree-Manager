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
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            std::string idLabel = "Id: " + std::to_string(talent->index) + ", Pos: (" + std::to_string(talent->row) + ", " + std::to_string(talent->column) + ")";
            if (talent->type != Engine::TalentType::SWITCH) {
                ImGui::BeginTooltip();
                ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
                ImGui::Text(talent->getName().c_str());
                ImGui::PopFont();
                if (uiData.iconIndexMap.count(talent->index)) {
                    ImGui::Image(
                        uiData.iconIndexMap.at(talent->index).first.texture, 
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
                    ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
                    ImGui::Text(talent->getName().c_str());
                    ImGui::PopFont();
                    if (uiData.iconIndexMap.count(talent->index)) {
                        if (talent->talentSwitch == 1) {
                            ImGui::Image(
                                uiData.iconIndexMap.at(talent->index).first.texture, 
                                ImVec2(static_cast<float>(uiData.treeEditorBaseTalentSize), static_cast<float>(uiData.treeEditorBaseTalentSize))
                            );
                        }
                        else if (talent->talentSwitch == 2) {
                            ImGui::Image(
                                uiData.iconIndexMap.at(talent->index).second.texture, 
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
                    ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
                    ImGui::Text(talent->name.c_str());
                    ImGui::PopFont();
                    if (uiData.iconIndexMap.count(talent->index)) {
                        ImGui::Image(
                            uiData.iconIndexMap.at(talent->index).first.texture, 
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
                    ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
                    ImGui::Text(talent->nameSwitch.c_str());
                    ImGui::PopFont();
                    if (uiData.iconIndexMap.count(talent->index)) {
                        ImGui::Image(
                            uiData.iconIndexMap.at(talent->index).second.texture, 
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
                    uiData.treeEditorZoomFactor = std::clamp(uiData.treeEditorZoomFactor, 1.0f, 3.0f);
                    if (oldZoomFactor != uiData.treeEditorZoomFactor) {
                        uiData.treeEditorWindowPos = ImGui::GetWindowPos();
                        uiData.treeEditorWindowSize = ImGui::GetWindowSize();
                        uiData.treeEditorMousePos = ImGui::GetMousePos();
                    }
                }
            }
            
            placeLoadoutEditorTreeElements(uiData, talentTreeCollection);

            ImGui::EndChild();
        }
        ImGui::End();
        ImGui::PopStyleVar();
        if (ImGui::Begin("SettingsWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
            if (ImGui::Button("Loadout Information", ImVec2(ImGui::GetContentRegionAvail().x / 2.f, 25))) {
                uiData.loadoutEditPage = LoadoutEditPage::LoadoutInformation;
            }
            ImGui::SameLine();
            if (ImGui::Button("Edit Skillsets", ImVec2(ImGui::GetContentRegionAvail().x, 25))) {
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
                    ImGui::Text("Skillset points spent: %d (+%d)", talentTreeCollection.activeSkillset()->talentPointsSpent - talentTreeCollection.activeTree().preFilledTalentPoints, talentTreeCollection.activeTree().preFilledTalentPoints);
                    int minLevel = 0;
                    if (talentTreeCollection.activeSkillset()->talentPointsSpent > talentTreeCollection.activeTree().preFilledTalentPoints) {
                        minLevel += 10;
                        if (talentTreeCollection.activeTree().type == Engine::TreeType::CLASS) {
                            minLevel += (talentTreeCollection.activeSkillset()->talentPointsSpent - talentTreeCollection.activeTree().preFilledTalentPoints) * 2 - 2;
                        }
                        else {
                            minLevel += (talentTreeCollection.activeSkillset()->talentPointsSpent - talentTreeCollection.activeTree().preFilledTalentPoints)* 2 - 1;
                        }
                    }
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

                ImGui::Text("Loadout description:");
                ImGui::InputTextMultiline("##LoadoutDescriptionInputText", &talentTreeCollection.activeTree().loadoutDescription,
                    ImGui::GetContentRegionAvail());
            }break;
            case LoadoutEditPage::SkillsetEdit: {
                ImGui::Text("Skillsets:");
                if (ImGui::BeginListBox("##loadoutEditorSkillsetsListbox", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
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
                ImGui::Separator();
                ImGui::Text("Hint: Loadouts are stored in trees. If you save a tree, this will include the loadout!");
            }break;
            }
            if (ImGui::BeginPopupModal("Import skillsets result", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Imported %d skillsets!\n(%d skillsets might have been discarded due to mismatched trees\nor corrupted import strings.)", 
                    uiData.loadoutEditorImportSkillsetsResult.first, uiData.loadoutEditorImportSkillsetsResult.second);

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

        int maxRow = 0;

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImGuiStyle& imStyle = ImGui::GetStyle();
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
            if (talent.second->pointsRequired > talentTreeCollection.activeSkillset()->talentPointsSpent - talentTreeCollection.activeTree().preFilledTalentPoints || !isParentFilled || talent.second->preFilled) {
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
            ImGui::SetCursorPos(ImVec2(posX - 0.5f * (uiData.treeEditorZoomFactor * uiData.redIconGlow.width - talentSize), posY - 0.5f * (uiData.treeEditorZoomFactor * uiData.redIconGlow.height - talentSize)));
            if (uiData.enableGlow && !searchActive) {
                if (talent.second->points == talent.second->maxPoints) {
                    ImGui::Image(
                        uiData.goldIconGlow.texture,
                        ImVec2(uiData.treeEditorZoomFactor * uiData.goldIconGlow.width, uiData.treeEditorZoomFactor * uiData.goldIconGlow.height),
                        ImVec2(0, 0), ImVec2(1, 1),
                        ImVec4(1, 1, 1, 1.0f - 0.5f * (uiData.style == Presets::STYLES::COMPANY_GREY))
                    );
                }
                else if (talent.second->points > 0) {
                    ImGui::Image(
                        uiData.greenIconGlow.texture,
                        ImVec2(uiData.treeEditorZoomFactor * uiData.greenIconGlow.width, uiData.treeEditorZoomFactor * uiData.greenIconGlow.height),
                        ImVec2(0, 0), ImVec2(1, 1),
                        ImVec4(1, 1, 1, 1.0f - 0.5f * (uiData.style == Presets::STYLES::COMPANY_GREY))
                    );
                }
            }
            else if (uiData.talentSearchString != "" && std::find(uiData.searchedTalents.begin(), uiData.searchedTalents.end(), talent.second) != uiData.searchedTalents.end()) {
                talentIsSearchedFor = true;
                ImGui::Image(
                    uiData.blueIconGlow.texture,
                    ImVec2(uiData.treeEditorZoomFactor * uiData.blueIconGlow.width, uiData.treeEditorZoomFactor * uiData.blueIconGlow.height),
                    ImVec2(0, 0), ImVec2(1, 1),
                    ImVec4(1, 1, 1, 1.0f - 0.5f * (uiData.style == Presets::STYLES::COMPANY_GREY))
                );
            }
            ImGui::SetCursorPos(ImVec2(posX, posY));
            ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
            if (talentDisabled) {
                ImGui::GetCurrentContext()->Style.DisabledAlpha = 0.35f;
                ImGui::BeginDisabled();
            }

            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
            ImGui::PushID((std::to_string(talent.second->points) + "/" + std::to_string(talent.second->maxPoints) + "##" + std::to_string(talent.second->index)).c_str());
            TextureInfo iconContent;
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
                    iconContent = uiData.splitIconIndexMap[talent.second->index];
                }
            }
            else {
                iconContent = uiData.iconIndexMap[talent.second->index].first;
            }
            if (ImGui::ImageButton(iconContent.texture,
                ImVec2(static_cast<float>(talentSize), static_cast<float>(talentSize)), ImVec2(0, 0), ImVec2(1, 1), 0
            )) {
                if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)) {
                    if (talent.second->type == Engine::TalentType::SWITCH) {
                        talent.second->talentSwitch = talent.second->talentSwitch < 2 ? 2 : 1;
                        if (talent.second->points > 0) {
                            talentTreeCollection.activeSkillset()->assignedSkillPoints[talent.first] = talent.second->talentSwitch;
                        }
                    }
                }
                else {
                    if (isParentFilled && talentTreeCollection.activeSkillset()->talentPointsSpent - talentTreeCollection.activeTree().preFilledTalentPoints >= talent.second->pointsRequired && talent.second->points < talent.second->maxPoints) {
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
                1.0f - 0.65f * talentDisabled,
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
            ImGui::PopFont();
            AttachLoadoutEditTooltip(uiData, talent.second);
        }
        for (auto& talent : tree.orderedTalents) {
            for (auto& child : talent.second->children) {
                drawArrowBetweenTalents(
                    talent.second,
                    child,
                    drawList,
                    ImGui::GetWindowPos(),
                    ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY()),
                    origin,
                    talentHalfSpacing,
                    talentSize,
                    0.0f,
                    uiData,
                    true);
            }
        }
        if (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift)) {
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
                ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[3]);
                AddWrappedText(infoText, textBoxPos, 5.0f, ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 2.0f * talentSize, 2.0f * talentSize, ImGui::GetWindowDrawList());
                ImGui::PopFont();
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