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

#include "LoadoutEditorWindow.h"
#include "TTMGUIPresets.h"

namespace TTM {
    static void AttachLoadoutEditTooltip(const UIData& uiData, Engine::Talent_s talent)
    {
        if (ImGui::IsItemHovered())
        {
            std::string idLabel = "Id: " + std::to_string(talent->index) + ", Pos: (" + std::to_string(talent->row) + ", " + std::to_string(talent->column) + ")";
            if (talent->type != Engine::TalentType::SWITCH) {
                ImGui::BeginTooltip();
                ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
                ImGui::Text(talent->getName().c_str());
                ImGui::PopFont();
                //ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(idLabel.c_str()).x);
                ImGui::Text(idLabel.c_str());
                std::string talentTypeString;
                switch (talent->type) {
                case Engine::TalentType::ACTIVE: {talentTypeString = "(active)"; }break;
                case Engine::TalentType::PASSIVE: {talentTypeString = "(passive)"; }break;
                }
                ImGui::TextUnformattedColored(Presets::GET_TOOLTIP_TALENT_TYPE_COLOR(uiData.style), talentTypeString.c_str());
                ImGui::Text(("Points: " + std::to_string(talent->points) + "/" + std::to_string(talent->maxPoints) + ", points required: " + std::to_string(talent->pointsRequired)).c_str());
                ImGui::Spacing();
                ImGui::Spacing();

                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 15.0f);
                ImGui::TextUnformattedColored(Presets::GET_TOOLTIP_TALENT_DESC_COLOR(uiData.style), talent->getDescription().c_str());
                ImGui::PopTextWrapPos();

                ImGui::EndTooltip();
            }
            else {
                ImGui::BeginTooltip();
                ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
                ImGui::Text(talent->getName().c_str());
                ImGui::PopFont();
                //ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(idLabel.c_str()).x);
                ImGui::Text(idLabel.c_str());
                ImGui::TextColored(Presets::GET_TOOLTIP_TALENT_TYPE_COLOR(uiData.style), "(switch, ctrl+click)");
                ImGui::Text(("Points: " + std::to_string(talent->points) + "/" + std::to_string(talent->maxPoints) + ", points required: " + std::to_string(talent->pointsRequired)).c_str());
                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::TextUnformattedColored(Presets::GET_TOOLTIP_TALENT_DESC_COLOR(uiData.style), talent->getDescription().c_str());

                ImGui::EndTooltip();
            }
        }
    }

    void RenderLoadoutEditorWindow(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
        if (ImGui::Begin("TreeWindow", nullptr, ImGuiWindowFlags_NoDecoration)) {
            ImGui::BeginChild("TreeWindowChild", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            if (ImGui::IsItemActive())
            {
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                    ImGui::SetScrollX(ImGui::GetScrollX() - ImGui::GetIO().MouseDelta.x);
                    ImGui::SetScrollY(ImGui::GetScrollY() - ImGui::GetIO().MouseDelta.y);
                }
            }
            if (ImGui::IsWindowHovered()) {
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
                    ImGui::Text("Skillset points spent: %d", talentTreeCollection.activeSkillset()->talentPointsSpent);
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
                if (ImGui::Button("Add skillset##loadoutEditorAddSkillsetButton", ImVec2(ImGui::GetContentRegionAvail().x / 3.0f, 0))) {
                    Engine::createSkillset(talentTreeCollection.activeTree());
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
                    uiData.loadoutEditorImportSkillsetsResult = Engine::importSkillsets(talentTreeCollection.activeTree(), uiData.loadoutEditorImportSkillsetsString);
                    ImGui::OpenPopup("Import skillsets result");
                }
                ImGui::Text("Export active skillset:");
                ImGui::InputText("##loadoutEditorExportActiveSkillsetInput", &uiData.loadoutEditorExportActiveSkillsetString, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AutoSelectAll);
                ImGui::SameLine();
                if (ImGui::Button("Export##loadoutEditorExportActiveSkillsetButton")) {
                    uiData.loadoutEditorExportActiveSkillsetString = Engine::createActiveSkillsetStringRepresentation(talentTreeCollection.activeTree());
                }
                ImGui::Text("Export all skillsets:");
                ImGui::InputText("##loadoutEditorExportAllSkillsetsInput", &uiData.loadoutEditorExportAllSkillsetsString, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AutoSelectAll);
                ImGui::SameLine();
                if (ImGui::Button("Export##loadoutEditorExportAllSkillsetsButton")) {
                    uiData.loadoutEditorExportAllSkillsetsString = Engine::createAllSkillsetsStringRepresentation(talentTreeCollection.activeTree());
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
            tree.activeSkillsetIndex = 0;
        }

        //TTMTODO: Change button style layout to render base layout? This is messy af
        int talentHalfSpacing = static_cast<int>(uiData.treeEditorBaseTalentHalfSpacing * uiData.treeEditorZoomFactor);
        int talentSize = static_cast<int>(uiData.treeEditorBaseTalentSize * uiData.treeEditorZoomFactor);
        float talentWindowPaddingY = static_cast<float>(uiData.treeEditorTalentWindowPaddingY);

        float talentWindowPaddingX = 0.5f * (ImGui::GetWindowWidth() - tree.maxCol * 2 * talentHalfSpacing);
        float minXPadding = ImGui::GetCursorPosX();
        float minYPadding = ImGui::GetCursorPosY();
        if (talentWindowPaddingX < minXPadding)
            talentWindowPaddingX = minXPadding;
        float talentPadding = 0.5f * (2 * talentHalfSpacing - talentSize);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        for (auto& talent : tree.orderedTalents) {
            float posX = talentWindowPaddingX + (talent.second->column - 1) * 2 * talentHalfSpacing + talentPadding;
            float posY = talentWindowPaddingY + (talent.second->row - 1) * 2 * talentHalfSpacing + talentPadding;
            bool changedColor = false;
            bool talentDisabled = false;
            bool isParentFilled = talent.second->parents.size() == 0;
            for (auto& parent : talent.second->parents) {
                if (parent->points == parent->maxPoints) {
                    isParentFilled = true;
                    break;
                }
            }
            if (talent.second->pointsRequired > talentTreeCollection.activeSkillset()->talentPointsSpent || !isParentFilled) {
                talentDisabled = true;
            }
            if (talent.second->points > 0 && talent.second->points < talent.second->maxPoints) {
                ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.2f, 0.9f, 0.2f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor(0.25f, 1.0f, 0.25f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(0.4f, 1.0f, 0.4f));
                ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(0, 0, 0));
                changedColor = true;
            }
            else if (talent.second->points == talent.second->maxPoints) {
                if (talent.second->type == Engine::TalentType::SWITCH) {
                    if (talent.second->talentSwitch == 1) {
                        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.9f, 0.43f, 0.43f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor(1.0f, 0.53f, 0.53f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(1.0f, 0.63f, 0.63f));
                        ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(0, 0, 0));
                    }
                    else {
                        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.43f, 0.43f, 0.9f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor(0.53f, 0.53f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(.63f, 0.63f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(0, 0, 0));
                    }
                }
                else {
                    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.9f, 0.73f, 0.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor(1.0f, 0.82f, 0.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(1.0f, 0.95f, 0.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(0, 0, 0));
                }
                changedColor = true;
            }
            ImGui::SetCursorPos(ImVec2(posX, posY));
            ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f + (talent.second->type == Engine::TalentType::SWITCH) * 9.0f * uiData.treeEditorZoomFactor + (talent.second->type == Engine::TalentType::PASSIVE) * 15.0f * uiData.treeEditorZoomFactor);
            ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 0.0f + (talent.second->type == Engine::TalentType::SWITCH) * 9.0f * uiData.treeEditorZoomFactor + (talent.second->type == Engine::TalentType::PASSIVE) * 15.0f * uiData.treeEditorZoomFactor);
            if (talentDisabled) {
                ImGui::GetCurrentContext()->Style.DisabledAlpha = 0.35f;
                ImGui::BeginDisabled();
            }
            if (ImGui::Button((std::to_string(talent.second->points) + "##" + std::to_string(talent.second->index)).c_str(), ImVec2(static_cast<float>(talentSize), static_cast<float>(talentSize)))) {
                //TTMTODO: loadout editor talent selection
                if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)) {
                    if (talent.second->type == Engine::TalentType::SWITCH && talent.second->points > 0) {
                        talent.second->talentSwitch = talent.second->talentSwitch == 1 ? 2 : 1;
                        talentTreeCollection.activeSkillset()->assignedSkillPoints[talent.first] = talent.second->talentSwitch;
                    }
                }
                else {
                    if (isParentFilled && talentTreeCollection.activeSkillset()->talentPointsSpent >= talent.second->pointsRequired && talent.second->points < talent.second->maxPoints) {
                        talent.second->points += 1;
                        talentTreeCollection.activeSkillset()->assignedSkillPoints[talent.first] += 1;
                        talentTreeCollection.activeSkillset()->talentPointsSpent += 1;
                        if (talent.second->type == Engine::TalentType::SWITCH) {
                            talent.second->talentSwitch = 1;
                        }
                    }
                }
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                uiData.loadoutEditorRightClickIndex = talent.first;
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
                        talentTreeCollection.activeSkillset()->assignedSkillPoints[talent.first] += 1;
                        talentTreeCollection.activeSkillset()->talentPointsSpent += 1;
                    }
                    if (talent.second->points == 0) {
                        talent.second->talentSwitch = 0;
                    }
                }
            }
            if (talentDisabled) {
                ImGui::GetCurrentContext()->Style.DisabledAlpha = 0.6f;
                ImGui::EndDisabled();
            }
            ImGui::PopStyleVar(2);
            ImGui::PopFont();
            if (changedColor) {
                ImGui::PopStyleColor(4);
            }
            AttachLoadoutEditTooltip(uiData, talent.second);
            for (auto& child : talent.second->children) {
                drawArrowBetweenTalents(
                    talent.second,
                    child,
                    drawList,
                    ImGui::GetWindowPos(),
                    ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY()),
                    ImVec2(talentWindowPaddingX, talentWindowPaddingY),
                    talentHalfSpacing,
                    talentSize,
                    talentPadding,
                    uiData);
            }
        }
        //add an invisible button to get scrollspace padding correctly, factor 1.5 is due to 1.0 min padding at the borders and 0.5 auto padding between rows
        ImGui::InvisibleButton("##invisbuttonedit", ImVec2(tree.maxCol * 2.0f * talentHalfSpacing + 1.0f, talentHalfSpacing - 0.5f * talentSize + talentWindowPaddingY - 1.5f * minYPadding));

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