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

#include "LoadoutSolverWindow.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"

namespace TTM {
    static void AttachLoadoutSolverTooltip(const UIData& uiData, Engine::Talent_s talent, int assignedPointsTarget)
    {
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
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
                if (talent->preFilled) {
                    ImGui::TextUnformattedColored(Presets::GET_TOOLTIP_TALENT_TYPE_COLOR(uiData.style), "(preselected)");
                }
                if (assignedPointsTarget >= 0) {
                    ImGui::Text(("Points: " + std::to_string(assignedPointsTarget) + "/" + std::to_string(talent->maxPoints) + ", points required: " + std::to_string(talent->pointsRequired)).c_str());
                }
                else {
                    ImGui::Text(("Points: exclude, points required: " + std::to_string(talent->pointsRequired)).c_str());
                }
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
                ImGui::Text(talent->name.c_str());
                ImGui::PopFont();
                //ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(idLabel.c_str()).x);
                ImGui::Text(idLabel.c_str());
                ImGui::TextColored(Presets::GET_TOOLTIP_TALENT_TYPE_COLOR(uiData.style), "(switch)");
                if (talent->preFilled) {
                    ImGui::TextUnformattedColored(Presets::GET_TOOLTIP_TALENT_TYPE_COLOR(uiData.style), "(preselected)");
                }
                if (assignedPointsTarget >= 0) {
                    ImGui::Text(("Points: " + std::to_string(assignedPointsTarget) + "/" + std::to_string(talent->maxPoints) + ", points required: " + std::to_string(talent->pointsRequired)).c_str());
                }
                else {
                    ImGui::Text(("Points: exclude, points required: " + std::to_string(talent->pointsRequired)).c_str());
                }
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
                //ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(idLabel.c_str()).x);
                ImGui::Text(idLabel.c_str());
                ImGui::TextColored(Presets::GET_TOOLTIP_TALENT_TYPE_COLOR(uiData.style), "(switch)");
                if (talent->preFilled) {
                    ImGui::TextUnformattedColored(Presets::GET_TOOLTIP_TALENT_TYPE_COLOR(uiData.style), "(preselected)");
                }
                if (assignedPointsTarget >= 0) {
                    ImGui::Text(("Points: " + std::to_string(assignedPointsTarget) + "/" + std::to_string(talent->maxPoints) + ", points required: " + std::to_string(talent->pointsRequired)).c_str());
                }
                else {
                    ImGui::Text(("Points: exclude, points required: " + std::to_string(talent->pointsRequired)).c_str());
                }
                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 15.0f);
                ImGui::TextUnformattedColored(Presets::GET_TOOLTIP_TALENT_DESC_COLOR(uiData.style), talent->descriptions[1].c_str());
                ImGui::PopTextWrapPos();

                ImGui::EndTooltip();
            }
        }
    }

    void RenderLoadoutSolverWindow(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
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

            placeLoadoutSolverTreeElements(uiData, talentTreeCollection);
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            if (ImGui::BeginPopupModal("Talent tree too large", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Talent tree has too many possible talent points!\nMaximum number of possible talent points spent is 64.");

                ImGui::SetItemDefaultFocus();
                if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
                ImGui::EndPopup();
            }

            ImGui::EndChild();
        }
        ImGui::End();
        ImGui::PopStyleVar();
        if (talentTreeCollection.activeTreeData().isTreeSolveProcessed) {
            if (ImGui::Begin("SettingsWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
                switch (uiData.loadoutSolverPage) {
                case LoadoutSolverPage::SolutionResults: {
                    ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
                    ImGui::Text("%s has %d different skillset combinations with 1 to %d talent points.",
                        talentTreeCollection.activeTree().name.c_str(), uiData.loadoutSolverAllCombinationsAdded, uiData.loadoutSolverTalentPointLimit);
                    if (ImGui::Button("Reset solutions")) {
                        clearSolvingProcess(uiData, talentTreeCollection);
                    }
                    ImGui::Text("Hint: Include/Exclude talents on the left, then press filter to view all valid combinations. Green/Yellow = must have, Red = must not have.");
                    ImGui::PopTextWrapPos();
                    if (ImGui::Checkbox("Auto apply filter", &uiData.loadoutSolverAutoApplyFilter)) {
                        if (uiData.loadoutSolverAutoApplyFilter) {
                            Engine::filterSolvedSkillsets(talentTreeCollection.activeTree(), talentTreeCollection.activeTreeData().treeDAGInfo, talentTreeCollection.activeTreeData().skillsetFilter);
                            talentTreeCollection.activeTreeData().isTreeSolveFiltered = true;
                            uiData.loadoutSolverTalentPointSelection = -1;
                            uiData.loadoutSolverSkillsetResultPage = -1;
                            uiData.loadoutSolverBufferedPage = -1;
                        }
                    }
                    if (uiData.loadoutSolverAutoApplyFilter) {
                        ImGui::BeginDisabled();
                    }
                    if (ImGui::Button("Filter")) {
                        Engine::filterSolvedSkillsets(talentTreeCollection.activeTree(), talentTreeCollection.activeTreeData().treeDAGInfo, talentTreeCollection.activeTreeData().skillsetFilter);
                        talentTreeCollection.activeTreeData().isTreeSolveFiltered = true;
                        uiData.loadoutSolverTalentPointSelection = -1;
                        uiData.loadoutSolverSkillsetResultPage = -1;
                        uiData.loadoutSolverBufferedPage = -1;
                    }
                    if (uiData.loadoutSolverAutoApplyFilter) {
                        ImGui::EndDisabled();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Clear filter")) {
                        for (auto& indexPointPair : talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints) {
                            indexPointPair.second = 0;
                        }
                        Engine::filterSolvedSkillsets(talentTreeCollection.activeTree(), talentTreeCollection.activeTreeData().treeDAGInfo, talentTreeCollection.activeTreeData().skillsetFilter);
                        talentTreeCollection.activeTreeData().isTreeSolveFiltered = true;
                        uiData.loadoutSolverTalentPointSelection = -1;
                        uiData.loadoutSolverSkillsetResultPage = -1;
                        uiData.loadoutSolverBufferedPage = -1;
                    }
                    if (talentTreeCollection.activeTreeData().isTreeSolveFiltered) {
                        ImGui::Separator();
                        ImGui::Text("Number of talent points (number of combinations):");
                        ImGui::Checkbox("##loadoutSolverRestrictTalentPointsCheckbox", &talentTreeCollection.activeTreeData().restrictTalentPoints);
                        if (!talentTreeCollection.activeTreeData().restrictTalentPoints) {
                            ImGui::BeginDisabled();
                        }
                        ImGui::SameLine();
                        ImGui::Text("Restrict talent points to:");
                        ImGui::SameLine();
                        if (ImGui::BeginCombo("##loadoutSolverRestrictTalentPointsCombo", std::to_string(talentTreeCollection.activeTreeData().restrictedTalentPoints + 1).c_str(), ImGuiComboFlags_None))
                        {
                            for (int n = 0; n < uiData.loadoutSolverTalentPointLimit; n++)
                            {
                                const bool is_selected = (talentTreeCollection.activeTreeData().restrictedTalentPoints == n);
                                if (ImGui::Selectable(std::to_string(n + 1).c_str(), is_selected))
                                    talentTreeCollection.activeTreeData().restrictedTalentPoints = n;

                                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                                if (is_selected)
                                    ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }
                        if (!talentTreeCollection.activeTreeData().restrictTalentPoints) {
                            ImGui::EndDisabled();
                        }
                        if (ImGui::BeginListBox("##loadoutSolverTalentPointsListbox", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
                        {
                            if (talentTreeCollection.activeTreeData().restrictTalentPoints && talentTreeCollection.activeTree().maxTalentPoints > 0) {
                                int rtp = talentTreeCollection.activeTreeData().restrictedTalentPoints;
                                const bool is_selected = (uiData.loadoutSolverTalentPointSelection == rtp);
                                if (ImGui::Selectable((std::to_string(rtp + 1) + " (" + std::to_string(talentTreeCollection.activeTreeData().treeDAGInfo->filteredCombinations[rtp].size()) + ")").c_str(), is_selected)) {
                                    uiData.loadoutSolverTalentPointSelection = rtp;
                                    uiData.loadoutSolverSkillsetResultPage = 0;
                                    uiData.loadoutSolverBufferedPage = -1;
                                }

                                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                                if (is_selected)
                                    ImGui::SetItemDefaultFocus();
                            }
                            else {
                                for (int n = 0; n < uiData.loadoutSolverTalentPointLimit; n++)
                                {
                                    if (talentTreeCollection.activeTreeData().treeDAGInfo->filteredCombinations[n].size() > 0) {
                                        const bool is_selected = (uiData.loadoutSolverTalentPointSelection == n);
                                        if (ImGui::Selectable((std::to_string(n + 1) + " (" + std::to_string(talentTreeCollection.activeTreeData().treeDAGInfo->filteredCombinations[n].size()) + ")").c_str(), is_selected)) {
                                            uiData.loadoutSolverTalentPointSelection = n;
                                            uiData.loadoutSolverSkillsetResultPage = 0;
                                            uiData.loadoutSolverBufferedPage = -1;
                                        }

                                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                                        if (is_selected)
                                            ImGui::SetItemDefaultFocus();
                                    }
                                }
                            }
                            ImGui::EndListBox();
                        }
                        if (uiData.loadoutSolverTalentPointSelection > -1) {
                            displayFilteredSkillsetSelector(uiData, talentTreeCollection);
                        }
                    }
                }break;
                }
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            if (ImGui::BeginPopupModal("Add to loadout successfull", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Skillset/s has/have been successfully added to the loadout!");

                ImGui::SetItemDefaultFocus();
                if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
                ImGui::EndPopup();
            }
            ImGui::End();
        }
    }

    void placeLoadoutSolverTreeElements(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        Engine::TalentTree& tree = talentTreeCollection.activeTree();

        if (!talentTreeCollection.activeTreeData().isTreeSolveProcessed && !uiData.loadoutSolveInitiated) {
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
            ImGui::Text("Select the number of talent points to spend at most (higher count means longer runtime). Maximum value is 64 for now (should be enough for all retail talent trees).");
            ImVec2 l1TopLeft = ImGui::GetItemRectMin();
            ImVec2 l1BottomRight = ImGui::GetItemRectMax();
            l1TopLeft.x -= boxPadding;
            l1TopLeft.y -= boxPadding;
            l1BottomRight.x += boxPadding;
            l1BottomRight.y += boxPadding;
            ImGui::SetCursorPosX(pos.x);
            ImGui::TextColored(ImVec4(1.f, 0.2f, 0.2f, 1.0f), "The solver does not yet support classic talent trees (3 subtrees in one big class tree with independent point requirements)!");

            //ask for solver max talent points
            ImGui::SetCursorPosX(pos.x);
            ImGui::Text("Talent points limit:");
            ImGui::SetCursorPosX(pos.x);
            ImGui::PushItemWidth(wrapWidth);
            ImGui::SliderInt("##loadoutSolverTalentPointsLimitSlider", &uiData.loadoutSolverTalentPointLimit, 1, uiData.loadoutSolverMaxTalentPoints, "%d", ImGuiSliderFlags_NoInput);
            ImGui::PopItemWidth();

            ImVec2 l2BottomRight = ImGui::GetItemRectMax();
            l2BottomRight.x += boxPadding;
            l2BottomRight.y += boxPadding;

            // Draw actual text bounding box, following by marker of our expected limit (should not overlap!)
            draw_list->AddRect(l1TopLeft, ImVec2(l1TopLeft.x + wrapWidth + 2 * boxPadding, l2BottomRight.y), IM_COL32(200, 200, 200, 255));
            ImGui::PopTextWrapPos();

            //center a button
            ImGui::SetCursorPos(ImVec2(centerX - 0.5f * wrapWidth - boxPadding, ImGui::GetCursorPosY() + boxPadding));
            if (ImGui::Button("Process tree", ImVec2(wrapWidth + 2 * boxPadding, 25))) {
                uiData.loadoutSolveInitiated = true;
            }
            return;
        }
        else if (uiData.loadoutSolveInitiated && !uiData.loadoutSolveInProgress) {
            uiData.loadoutSolveInProgress = true;
            ImVec2 textSize = ImGui::CalcTextSize("Processing...");
            ImVec2 contentRegion = ImGui::GetContentRegionAvail();
            ImGui::SetCursorPos(ImVec2(0.5f * contentRegion.x - 0.5f * textSize.x, 0.5f * contentRegion.y - 0.5 * textSize.y));
            ImGui::Text("Processing...");
            return;
        }
        else if (uiData.loadoutSolveInitiated && uiData.loadoutSolveInProgress) {
            clearSolvingProcess(uiData, talentTreeCollection);
            if (uiData.loadoutSolverTalentPointLimit > talentTreeCollection.activeTree().maxTalentPoints - talentTreeCollection.activeTree().preFilledTalentPoints) {
                uiData.loadoutSolverTalentPointLimit = talentTreeCollection.activeTree().maxTalentPoints - talentTreeCollection.activeTree().preFilledTalentPoints;
            }
            if (uiData.loadoutSolverTalentPointLimit > uiData.loadoutSolverMaxTalentPoints
                // This check prevents trees with more than 64 spendable talent points from being handled
                // In a world where indexing would work with infite spendable talent poins (simple switch from uint64 to bitsets
                // this condition can be removed and everything should work fine
                || talentTreeCollection.activeTree().maxTalentPoints > uiData.loadoutSolverMaxTalentPoints) {
                ImGui::OpenPopup("Talent tree too large");
                uiData.loadoutSolveInitiated = false;
                uiData.loadoutSolveInProgress = false;
                return;
            }
            talentTreeCollection.activeTreeData().skillsetFilter = std::make_shared<Engine::TalentSkillset>();
            talentTreeCollection.activeTreeData().skillsetFilter->name = "SolverSkillset";
            for (auto& talent : tree.orderedTalents) {
                talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.first] = 0;
            }
            Engine::clearTree(tree);
            talentTreeCollection.activeTreeData().treeDAGInfo = Engine::countConfigurationsParallel(tree, uiData.loadoutSolverTalentPointLimit);
            talentTreeCollection.activeTreeData().isTreeSolveProcessed = true;
            for (auto& talentPointsCombinbations : talentTreeCollection.activeTreeData().treeDAGInfo->allCombinations) {
                for (auto& combinations : talentPointsCombinbations) {
                    uiData.loadoutSolverAllCombinationsAdded += combinations.second;
                }
            }
            uiData.loadoutSolveInitiated = false;
            uiData.loadoutSolveInProgress = false;
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
        int maxRow = 0;

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImGuiStyle& imStyle = ImGui::GetStyle();
        for (auto& talent : tree.orderedTalents) {
            maxRow = talent.second->row > maxRow ? talent.second->row : maxRow;
            float posX = talentWindowPaddingX + (talent.second->column - 1) * 2 * talentHalfSpacing + talentPadding;
            float posY = talentWindowPaddingY + (talent.second->row - 1) * 2 * talentHalfSpacing + talentPadding;
            ImGui::SetCursorPos(ImVec2(posX, posY));
            ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
            if (talent.second->preFilled) {
                ImGui::GetCurrentContext()->Style.DisabledAlpha = 0.35f;
                ImGui::BeginDisabled();
            }

            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0, 0, 0, 0));
            ImGui::PushID((std::to_string(talent.second->index)).c_str());
            TextureInfo iconContent;
            //TTMTODO: Implement dual icon textures for switch talents
            if (talent.second->type == Engine::TalentType::SWITCH) {
                if (talent.second->talentSwitch) {
                    iconContent = uiData.iconIndexMap[talent.second->index].first;
                }
                else {
                    iconContent = uiData.iconIndexMap[talent.second->index].first;
                }
            }
            else {
                iconContent = uiData.iconIndexMap[talent.second->index].first;
            }
            if (ImGui::ImageButton(iconContent.texture,
                ImVec2(static_cast<float>(talentSize), static_cast<float>(talentSize)), ImVec2(0, 0), ImVec2(1, 1), 0
            )) {
                talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.first] += 1;
                if (talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.first] > talent.second->maxPoints) {
                    talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.first] = -1;
                }
                if (uiData.loadoutSolverAutoApplyFilter) {
                    Engine::filterSolvedSkillsets(talentTreeCollection.activeTree(), talentTreeCollection.activeTreeData().treeDAGInfo, talentTreeCollection.activeTreeData().skillsetFilter);
                    talentTreeCollection.activeTreeData().isTreeSolveFiltered = true;
                    uiData.loadoutSolverTalentPointSelection = -1;
                    uiData.loadoutSolverSkillsetResultPage = -1;
                    uiData.loadoutSolverBufferedPage = -1;
                }
            }
            ImGui::PopID();
            ImGui::PopStyleColor(2);
            drawLoadoutSolverShapeAroundTalent(
                talent.second,
                drawList,
                imStyle.Colors,
                ImVec2(posX, posY),
                talentSize,
                ImGui::GetWindowPos(),
                ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY()),
                uiData,
                talentTreeCollection,
                talentTreeCollection.activeTreeData().skillsetFilter);
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                uiData.loadoutEditorRightClickIndex = talent.first;
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right) && talent.first == uiData.loadoutEditorRightClickIndex) {
                talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.first] -= 1;
                if (talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.first] < -1) {
                    talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.first] = talent.second->maxPoints;
                }
                if (uiData.loadoutSolverAutoApplyFilter) {
                    Engine::filterSolvedSkillsets(talentTreeCollection.activeTree(), talentTreeCollection.activeTreeData().treeDAGInfo, talentTreeCollection.activeTreeData().skillsetFilter);
                    talentTreeCollection.activeTreeData().isTreeSolveFiltered = true;
                    uiData.loadoutSolverTalentPointSelection = -1;
                    uiData.loadoutSolverSkillsetResultPage = -1;
                    uiData.loadoutSolverBufferedPage = -1;
                }
            }
            if (talent.second->preFilled) {
                ImGui::GetCurrentContext()->Style.DisabledAlpha = 0.6f;
                ImGui::EndDisabled();
            }
            ImGui::PopFont();
            AttachLoadoutSolverTooltip(uiData, talent.second, talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.first]);
        }
        for (auto& talent : tree.orderedTalents) {
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
        ImGui::SetCursorPos(ImVec2(0, talentWindowPaddingY + maxRow * 2 * talentHalfSpacing));
        ImGui::InvisibleButton(
            "##invisbuttonedit",
            ImVec2(
                2.0f * talentWindowPaddingX + (tree.maxCol - 2) * 2 * talentHalfSpacing + 2.0f * talentPadding + talentSize,
                talentHalfSpacing - 0.5f * talentSize + talentWindowPaddingY - 1.5f * minYPadding
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

    void displayFilteredSkillsetSelector(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        //get buffered skillset page results
        int maxPage = getResultsPage(uiData, talentTreeCollection, uiData.loadoutSolverSkillsetResultPage);
        if (uiData.loadoutSolverPageResults.size() == 0) {
            return;
        }
        ImGui::Text("Filtered skillsets:");

        if (ImGui::BeginListBox("##loadoutSolverFilteredSkillsetPageListbox", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
        {
            for (int n = 0; n < uiData.loadoutSolverPageResults.size(); n++)
            {
                const bool is_selected = (uiData.selectedFilteredSkillsetIndex == n);
                if (ImGui::Selectable(("Id: " + std::to_string(uiData.loadoutSolverPageResults[n].first)).c_str(), is_selected)) {
                    uiData.selectedFilteredSkillsetIndex = n;
                    uiData.selectedFilteredSkillset = uiData.loadoutSolverPageResults[n].first;
                }

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndListBox();
        }

        float padding = ImGui::GetCursorPosX();
        if (ImGui::Button("<<##loadoutSolverFilterSkipLeftButton")) {
            uiData.loadoutSolverSkillsetResultPage = 0;
            uiData.selectedFilteredSkillsetIndex = 0;
        }
        ImGui::SameLine();
        if (ImGui::Button("<##loadoutSolverFilterLeftButton")) {
            if (uiData.loadoutSolverSkillsetResultPage - 1 > 0) {
                uiData.loadoutSolverSkillsetResultPage -= 1;
            }
            else {
                uiData.loadoutSolverSkillsetResultPage = 0;
            }
            uiData.selectedFilteredSkillsetIndex = 0;
        }
        ImGui::SameLine();
        float width = ImGui::GetCursorPosX() - padding;
        ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x * 0.5f);
        ImGui::Text("%d/%d", uiData.loadoutSolverSkillsetResultPage + 1, maxPage + 1);
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - width + padding);
        if (ImGui::Button(">##loadoutSolverFilterRightButton")) {
            if (uiData.loadoutSolverSkillsetResultPage + 1 > maxPage) {
                uiData.loadoutSolverSkillsetResultPage = maxPage;
            }
            else {
                uiData.loadoutSolverSkillsetResultPage += 1;
            }
            uiData.selectedFilteredSkillsetIndex = 0;
        }
        ImGui::SameLine();
        if (ImGui::Button(">>##loadoutSolverFilterSkipRightButton")) {
            uiData.loadoutSolverSkillsetResultPage = maxPage;
            uiData.selectedFilteredSkillsetIndex = 0;
        }
        if (ImGui::Button("Add to loadout##loadoutSolverAddToLoadoutButton") && uiData.selectedFilteredSkillsetIndex >= 0) {
            std::shared_ptr<Engine::TalentSkillset> sk = Engine::skillsetIndexToSkillset(
                talentTreeCollection.activeTree(),
                talentTreeCollection.activeTreeData().treeDAGInfo,
                uiData.selectedFilteredSkillset
            );
            sk->name = "Solved loadout " + std::to_string(uiData.selectedFilteredSkillset);
            Engine::applyPreselectedTalentsToSkillset(talentTreeCollection.activeTree(), sk);
            talentTreeCollection.activeTree().loadout.push_back(sk);
            ImGui::OpenPopup("Add to loadout successfull");
        }
        if (ImGui::Button("Add all in page to loadout##loadoutSolverAddAllToLoadoutButton")) {
            for (auto& skillsetIndexPair : uiData.loadoutSolverPageResults) {
                std::shared_ptr<Engine::TalentSkillset> sk = Engine::skillsetIndexToSkillset(
                    talentTreeCollection.activeTree(),
                    talentTreeCollection.activeTreeData().treeDAGInfo,
                    skillsetIndexPair.first
                );
                sk->name = "Solved loadout " + std::to_string(skillsetIndexPair.first);
                Engine::applyPreselectedTalentsToSkillset(talentTreeCollection.activeTree(), sk);
                talentTreeCollection.activeTree().loadout.push_back(sk);
            }
            ImGui::OpenPopup("Add to loadout successfull");
        }
    }

    int getResultsPage(UIData& uiData, TalentTreeCollection& talentTreeCollection, int pageNumber) {
        std::vector<std::pair<Engine::SIND, int>>& filteredResults = talentTreeCollection.activeTreeData().treeDAGInfo->filteredCombinations[uiData.loadoutSolverTalentPointSelection];
        int maxPage = static_cast<int>((filteredResults.size() - 1) / uiData.loadoutSolverResultsPerPage);
        if (pageNumber == uiData.loadoutSolverBufferedPage) {
            return maxPage;
        }
        uiData.loadoutSolverBufferedPage = pageNumber;
        uiData.loadoutSolverPageResults.clear();
        int maxIndex = (pageNumber + 1) * uiData.loadoutSolverResultsPerPage;
        maxIndex = maxIndex > filteredResults.size() ? static_cast<int>(filteredResults.size()) : maxIndex;
        if (maxIndex == 0) {
            return maxPage;
        }
        for (int i = pageNumber * uiData.loadoutSolverResultsPerPage; i < maxIndex; i++) {
            uiData.loadoutSolverPageResults.push_back(filteredResults[i]);
        }
        if (uiData.loadoutSolverPageResults.size() > 0) {
            uiData.selectedFilteredSkillset = uiData.loadoutSolverPageResults[0].first;
        }
        return maxPage;
    }
}