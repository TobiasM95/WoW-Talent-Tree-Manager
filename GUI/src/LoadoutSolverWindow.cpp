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

#include <random>
#include <thread>

namespace TTM {
    static void AttachLoadoutSolverTooltip(const UIData& uiData, Engine::Talent_s talent, int assignedPointsTarget)
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
                if (assignedPointsTarget >= 0) {
                    ImGui::Text(("Points: " + std::to_string(assignedPointsTarget) + "/" + std::to_string(talent->maxPoints) + ", points required: " + std::to_string(talent->pointsRequired)).c_str());
                }
                else if (assignedPointsTarget == -1) {
                    ImGui::Text(("Points: exclude, points required: " + std::to_string(talent->pointsRequired)).c_str());
                }
                else if (assignedPointsTarget == -2) {
                    ImGui::Text(("Points: at least 1 point in group, points required: " + std::to_string(talent->pointsRequired)).c_str());
                }
                else if (assignedPointsTarget == -3) {
                    ImGui::Text(("Points: exactly 1 in group maxed, points required: " + std::to_string(talent->pointsRequired)).c_str());
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
                    uiData.treeEditorZoomFactor = std::clamp(uiData.treeEditorZoomFactor, 0.5f, 3.0f);
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
            if (ImGui::Begin("SettingsWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
                if (ImGui::Button("Solution Filter", ImVec2(ImGui::GetContentRegionAvail().x / 2.f, 25))) {
                    uiData.loadoutSolverPage = LoadoutSolverPage::SolutionResults;
                }
                ImGui::SameLine();
                if (ImGui::Button("Tree Solve Status", ImVec2(ImGui::GetContentRegionAvail().x, 25))) {
                    uiData.loadoutSolverPage = LoadoutSolverPage::TreeSolveStatus;
                }
                ImGui::Spacing();
                switch (uiData.loadoutSolverPage) {
                case LoadoutSolverPage::SolutionResults: {
                    ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
                    if (talentTreeCollection.activeTreeData().treeDAGInfo->safetyGuardTriggered) {
                        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Safety guard triggered! There were more than %d combinations in total or solve was canceled! Values below will not be accurate!", MAX_NUMBER_OF_SOLVED_COMBINATIONS);
                    }
                    ImGui::Text("%s has %d different skillset combinations with 1 to %d talent points (This does not include variations with different switch talent choices).",
                        talentTreeCollection.activeTree().name.c_str(), uiData.loadoutSolverAllCombinationsAdded, uiData.loadoutSolverTalentPointLimit);
                    ImGui::Text("Processing took %.3f seconds.", talentTreeCollection.activeTreeData().treeDAGInfo->elapsedTime);
                    if (ImGui::Button("Reset solutions")) {
                        clearSolvingProcess(uiData, talentTreeCollection);
                    }
                    ImGui::Text("Hint: Include/Exclude talents on the left, then press filter to view all valid combinations.");
                    ImGui::Text("Different colors mean different things.");
                    ImGui::SameLine();
                    TTM::HelperTooltip("(?)", "Green/Yellow: At least selected points spent in this talent.\n\nRed: No points in this talent.\n\nBlue: At least one of all blue talents must have at least 1 point.\n\nPurple: Exactly one talent must be maxed out.");
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
                case LoadoutSolverPage::TreeSolveStatus: {
                    //show loadout solver status
                    static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY
                        | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter
                        | ImGuiTableFlags_BordersV | ImGuiTableFlags_SizingStretchSame;
                    if (ImGui::BeginTable("loadoutSolverStatusTable", 3, flags))
                    {
                        ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
                        ImGui::TableSetupColumn("Tree name", ImGuiTableColumnFlags_None, 0.6f);
                        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_None, 0.2f);
                        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_None, 0.2f);
                        ImGui::TableHeadersRow();

                        for (auto& currSolver : uiData.currentSolvers) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("%s", currSolver.first.c_str());
                            ImGui::TableSetColumnIndex(1);
                            ImGui::TextColored(Presets::GET_TOOLTIP_TALENT_TYPE_COLOR(uiData.style), "solving...");
                            ImGui::TableSetColumnIndex(2);
                            if (ImGui::Button(("cancel###" + currSolver.first).c_str())) {
                                currSolver.second->safetyGuardTriggered = true;
                                updateSolverStatus(uiData, talentTreeCollection, true);
                            }
                        }
                        for (auto& solvedTree : uiData.solvedTrees) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("%s", solvedTree.first.c_str());
                            ImGui::TableSetColumnIndex(1);
                            if (solvedTree.second->safetyGuardTriggered) {
                                ImGui::TextColored(Presets::TALENT_MAXED_BORDER_COLOR, "canceled");
                            }
                            else {
                                ImGui::TextColored(Presets::TALENT_PARTIAL_BORDER_COLOR, "solved");
                            }
                            ImGui::TableSetColumnIndex(2);
                            if (ImGui::Button(("reset###" + solvedTree.first).c_str())) {
                                clearSolvingProcess(uiData, *solvedTree.second);
                                updateSolverStatus(uiData, talentTreeCollection, true);
                            }
                        }
                        ImGui::EndTable();
                    }
                }break;
                }
            }
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
        updateSolverStatus(uiData, talentTreeCollection);
        Engine::TalentTree& tree = talentTreeCollection.activeTree();

        if (!talentTreeCollection.activeTreeData().isTreeSolveInProgress 
            && !talentTreeCollection.activeTreeData().isTreeSolveProcessed
            && !talentTreeCollection.activeTreeData().treeDAGInfo) {
            ImVec2 contentRegion = ImGui::GetContentRegionAvail();
            float centerX = 0.5f * contentRegion.x;
            float centerY = 0.5f * contentRegion.y;
            float wrapWidth = 250;
            float offsetY = -200;
            float boxPadding = 8;

            //show loadout solver status
            ImVec2 outer_size = ImVec2(centerX - 0.6f * wrapWidth, 500.0f);
            static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY 
                | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter 
                | ImGuiTableFlags_BordersV | ImGuiTableFlags_SizingStretchSame;
            if (ImGui::BeginTable("loadoutSolverStatusTable", 3, flags, outer_size))
            {
                ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
                ImGui::TableSetupColumn("Tree name", ImGuiTableColumnFlags_None, 0.6f);
                ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_None, 0.2f);
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_None, 0.2f);
                ImGui::TableHeadersRow();

                for (auto& currSolver : uiData.currentSolvers) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", currSolver.first.c_str());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(Presets::GET_TOOLTIP_TALENT_TYPE_COLOR(uiData.style), "solving...");
                    ImGui::TableSetColumnIndex(2);
                    if (ImGui::Button(("cancel###" + currSolver.first).c_str())) {
                        currSolver.second->safetyGuardTriggered = true;
                        updateSolverStatus(uiData, talentTreeCollection, true);
                    }
                }
                for (auto& solvedTree : uiData.solvedTrees) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", solvedTree.first.c_str());
                    ImGui::TableSetColumnIndex(1);
                    if (solvedTree.second->safetyGuardTriggered) {
                        ImGui::TextColored(Presets::TALENT_MAXED_BORDER_COLOR, "canceled");
                    }
                    else {
                        ImGui::TextColored(Presets::TALENT_PARTIAL_BORDER_COLOR, "solved");
                    }
                    ImGui::TableSetColumnIndex(2);
                    if (ImGui::Button(("reset###" + solvedTree.first).c_str())) {
                        clearSolvingProcess(uiData, *solvedTree.second);
                        updateSolverStatus(uiData, talentTreeCollection, true);
                    }
                }
                ImGui::EndTable();
            }

            //center an information rectangle
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 pos = ImVec2(centerX - 0.5f * wrapWidth, centerY + offsetY);
            ImGui::SetCursorPos(pos);
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + wrapWidth);
            ImGui::Text("Select the number of talent points to spend at most (higher count means longer runtime). Maximum value is 64 for now (should be enough for all retail talent trees).\n\nThe maximum limit of generated combinations is 500 million, this requires a system with at least 16 GB of RAM. If you are below that threshold it is recommended to be careful when using this tool and increment the talent points slowly from 20 upwards while checking your resources.");
            ImVec2 l1TopLeft = ImGui::GetItemRectMin();
            ImVec2 l1BottomRight = ImGui::GetItemRectMax();
            l1TopLeft.x -= boxPadding;
            l1TopLeft.y -= boxPadding;
            l1BottomRight.x += boxPadding;
            l1BottomRight.y += boxPadding;
            ImGui::SetCursorPosX(pos.x);
            if (talentTreeCollection.activeTree().presetName == "custom" || talentTreeCollection.activeTree().type == Engine::TreeType::CLASS) {
                ImGui::TextColored(ImVec4(1.f, 0.2f, 0.2f, 1.0f), "Solving class/custom trees is very RAM intensive, continue with care!\n\nThe solver does not yet support classic talent trees (3 subtrees in one big class tree with independent point requirements)!");
            }
            else {
                ImGui::TextColored(ImVec4(1.f, 0.2f, 0.2f, 1.0f), "The solver does not yet support classic talent trees (3 subtrees in one big class tree with independent point requirements)!");
            }

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
            bool allowNewSolver = uiData.currentSolvers.size() < uiData.maxConcurrentSolvers;
            if (!allowNewSolver) {
                ImGui::BeginDisabled();
            }
            if (ImGui::Button("Process tree (max 3 solvers)", ImVec2(wrapWidth + 2 * boxPadding, 25))) {
                if (uiData.currentSolvers.size() >= uiData.maxConcurrentSolvers) {
                    return;
                }
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
                    return;
                }
                talentTreeCollection.activeTreeData().skillsetFilter = std::make_shared<Engine::TalentSkillset>();
                talentTreeCollection.activeTreeData().skillsetFilter->name = "SolverSkillset";
                for (auto& talent : tree.orderedTalents) {
                    talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.first] = 0;
                }
                Engine::clearTree(tree);
                
                std::thread t(Engine::countConfigurationsParallel,
                    tree, 
                    uiData.loadoutSolverTalentPointLimit,
                    std::ref(talentTreeCollection.activeTreeData().treeDAGInfo),
                    std::ref(talentTreeCollection.activeTreeData().isTreeSolveInProgress),
                    std::ref(talentTreeCollection.activeTreeData().safetyGuardTriggered));
                t.detach();
                updateSolverStatus(uiData, talentTreeCollection, true);
            }
            if (!allowNewSolver) {
                ImGui::EndDisabled();
            }
            return;
        }
        else if (talentTreeCollection.activeTreeData().isTreeSolveInProgress) {
            ImVec2 contentRegion = ImGui::GetContentRegionAvail();
            float centerX = 0.5f * contentRegion.x;
            float centerY = 0.5f * contentRegion.y;
            float wrapWidth = 250;
            float offsetY = -200;
            float boxPadding = 8;

            //show loadout solver status
            ImVec2 outer_size = ImVec2(centerX - 0.6f * wrapWidth, 500.0f);
            static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY
                | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter
                | ImGuiTableFlags_BordersV | ImGuiTableFlags_SizingStretchSame;
            if (ImGui::BeginTable("loadoutSolverStatusTable", 3, flags, outer_size))
            {
                ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
                ImGui::TableSetupColumn("Tree name", ImGuiTableColumnFlags_None, 0.6f);
                ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_None, 0.2f);
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_None, 0.2f);
                ImGui::TableHeadersRow();

                for (auto& currSolver : uiData.currentSolvers) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", currSolver.first.c_str());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(Presets::GET_TOOLTIP_TALENT_TYPE_COLOR(uiData.style), "solving...");
                    ImGui::TableSetColumnIndex(2);
                    if (ImGui::Button(("cancel###" + currSolver.first).c_str())) {
                        currSolver.second->safetyGuardTriggered = true;
                        updateSolverStatus(uiData, talentTreeCollection, true);
                    }
                }
                for (auto& solvedTree : uiData.solvedTrees) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", solvedTree.first.c_str());
                    ImGui::TableSetColumnIndex(1);
                    if (solvedTree.second->safetyGuardTriggered) {
                        ImGui::TextColored(Presets::TALENT_MAXED_BORDER_COLOR, "canceled");
                    }
                    else {
                        ImGui::TextColored(Presets::TALENT_PARTIAL_BORDER_COLOR, "solved");
                    }
                    ImGui::TableSetColumnIndex(2);
                    if (ImGui::Button(("reset###" + solvedTree.first).c_str())) {
                        clearSolvingProcess(uiData, *solvedTree.second);
                        updateSolverStatus(uiData, talentTreeCollection, true);
                    }
                }
                ImGui::EndTable();
            }

            ImVec2 textSize = ImGui::CalcTextSize("Processing...");
            ImGui::SetCursorPos(ImVec2(0.5f * contentRegion.x - 0.5f * textSize.x, 0.5f * contentRegion.y - 0.5f * textSize.y));
            ImGui::Text("Processing...");
            ImGui::SetCursorPosX(0.5f * contentRegion.x - 0.5f * textSize.x);
            if (ImGui::Button("Cancel##loadoutSolverCancelSolveButton", ImVec2(textSize.x, 0))) {
                talentTreeCollection.activeTreeData().safetyGuardTriggered = true;
            }
            ImGui::SetCursorPosX(0.5f * contentRegion.x - 0.5f * 2 * textSize.x);
            if (ImGui::Button("Cancel all solves##loadoutSolverCancelAllButton", ImVec2(2 * textSize.x, 0))) {
                for (auto& treeData : talentTreeCollection.trees) {
                    treeData.safetyGuardTriggered = true;
                }
            }
            return;
        }
        else if (!talentTreeCollection.activeTreeData().isTreeSolveProcessed
            && talentTreeCollection.activeTreeData().treeDAGInfo) {
            //TTMTODO: is this complication even necessary?
            for (auto& talentPointsCombinations : talentTreeCollection.activeTreeData().treeDAGInfo->allCombinations) {
                uiData.loadoutSolverAllCombinationsAdded += static_cast<int>(talentPointsCombinations.size());
            }
            talentTreeCollection.activeTreeData().isTreeSolveProcessed = true;
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
                    uiData);
            }
        }
        for (auto& reqInfo : tree.requirementSeparatorInfo) {
            drawList->AddLine(
                ImVec2(windowPos.x - scrollOffset.x + origin.x - 2 * talentSize, windowPos.y - scrollOffset.y + talentWindowPaddingY + (reqInfo.second - 1) * talentSize),
                ImVec2(windowPos.x - scrollOffset.x + origin.x + (tree.maxCol + 1) * talentSize, windowPos.y - scrollOffset.y + talentWindowPaddingY + (reqInfo.second - 1) * talentSize),
                ImColor(Presets::GET_TOOLTIP_TALENT_DESC_COLOR(uiData.style)),
                2.0f
            );
            drawList->AddText(
                ImVec2(windowPos.x - scrollOffset.x + origin.x - 2 * talentSize, windowPos.y - scrollOffset.y + talentWindowPaddingY + (reqInfo.second - 1) * talentSize),
                ImColor(Presets::GET_TOOLTIP_TALENT_DESC_COLOR(uiData.style)),
                (std::to_string(reqInfo.first) + " points").c_str()
            );
        }

        for (auto& talent : tree.orderedTalents) {
            maxRow = talent.second->row > maxRow ? talent.second->row : maxRow;
            float posX = origin.x + (talent.second->column - 1) * 2 * talentHalfSpacing;
            float posY = origin.y + (talent.second->row - 1) * 2 * talentHalfSpacing;
            bool talentIsSearchedFor = false;
            bool searchActive = uiData.talentSearchString != "";
            ImGui::SetCursorPos(ImVec2(posX - 0.5f * (uiData.treeEditorZoomFactor * uiData.redIconGlow.width - talentSize), posY - 0.5f * (uiData.treeEditorZoomFactor * uiData.redIconGlow.height - talentSize)));
            if (uiData.enableGlow && !searchActive) {
                if (talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.second->index] == talent.second->maxPoints) {
                    ImGui::Image(
                        uiData.goldIconGlow.texture,
                        ImVec2(uiData.treeEditorZoomFactor * uiData.goldIconGlow.width, uiData.treeEditorZoomFactor * uiData.goldIconGlow.height),
                        ImVec2(0, 0), ImVec2(1, 1),
                        ImVec4(1, 1, 1, 1.0f - 0.5f * (uiData.style == Presets::STYLES::COMPANY_GREY))
                    );
                }
                else if (talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.second->index] > 0) {
                    ImGui::Image(
                        uiData.greenIconGlow.texture,
                        ImVec2(uiData.treeEditorZoomFactor * uiData.greenIconGlow.width, uiData.treeEditorZoomFactor * uiData.greenIconGlow.height),
                        ImVec2(0, 0), ImVec2(1, 1),
                        ImVec4(1, 1, 1, 1.0f - 0.5f * (uiData.style == Presets::STYLES::COMPANY_GREY))
                    );
                }
                else if (talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.second->index] == -1) {
                    ImGui::Image(
                        uiData.redIconGlow.texture,
                        ImVec2(uiData.treeEditorZoomFactor * uiData.redIconGlow.width, uiData.treeEditorZoomFactor * uiData.redIconGlow.height),
                        ImVec2(0, 0), ImVec2(1, 1),
                        ImVec4(1, 1, 1, 1.0f - 0.5f * (uiData.style == Presets::STYLES::COMPANY_GREY))
                    );
                }
                else if (talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.second->index] == -2) {
                    ImGui::Image(
                        uiData.blueIconGlow.texture,
                        ImVec2(uiData.treeEditorZoomFactor * uiData.redIconGlow.width, uiData.treeEditorZoomFactor * uiData.redIconGlow.height),
                        ImVec2(0, 0), ImVec2(1, 1),
                        ImVec4(1, 1, 1, 1.0f - 0.5f * (uiData.style == Presets::STYLES::COMPANY_GREY))
                    );
                }
                else if (talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.second->index] == -3) {
                    ImGui::Image(
                        uiData.purpleIconGlow.texture,
                        ImVec2(uiData.treeEditorZoomFactor * uiData.redIconGlow.width, uiData.treeEditorZoomFactor * uiData.redIconGlow.height),
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
            Presets::PUSH_FONT(uiData.fontsize, 1);
            if (talent.second->preFilled) {
                ImGui::GetCurrentContext()->Style.DisabledAlpha = 0.35f;
                ImGui::BeginDisabled();
            }

            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
            ImGui::PushID((std::to_string(talent.second->index)).c_str());
            TextureInfo* iconContent = nullptr;
            TextureInfo* iconContentChoice = nullptr;
            if (talent.second->type == Engine::TalentType::SWITCH) {
                iconContent = uiData.iconIndexMap[talent.second->index].first;
                iconContentChoice = uiData.iconIndexMap[talent.second->index].second;
            }
            else {
                iconContent = uiData.iconIndexMap[talent.second->index].first;
            }
            if (ImGui::InvisibleButton(("##invisTalentButton" + std::to_string(talent.first)).c_str(),
                ImVec2(static_cast<float>(talentSize), static_cast<float>(talentSize))
            )) {
                talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.first] += 1;
                if (talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.first] > talent.second->maxPoints) {
                    talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.first] = -3;
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
            ImGui::PopStyleColor(5);
            if (talent.second->type != Engine::TalentType::SWITCH) {
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
                    ImVec2(talentSize * (1.0f - separatorWidth) / 2.0f, static_cast<float>(talentSize)), ImVec2((1.0f + separatorWidth) / 2.0f, 0), ImVec2(1, 1)
                );
            }

            ImGui::SetCursorPos(ImVec2(posX, posY));
            ImGui::Image(
                uiData.talentIconMasks[static_cast<int>(uiData.style)][static_cast<int>(talent.second->type)].texture,
                ImVec2(static_cast<float>(talentSize), static_cast<float>(talentSize)), ImVec2(0, 0), ImVec2(1, 1)
            );
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
                talentTreeCollection.activeTreeData().skillsetFilter,
                searchActive,
                talentIsSearchedFor);
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                uiData.loadoutEditorRightClickIndex = talent.first;
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right) && talent.first == uiData.loadoutEditorRightClickIndex) {
                talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.first] -= 1;
                if (talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.first] < -3) {
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
            Presets::POP_FONT();
            AttachLoadoutSolverTooltip(uiData, talent.second, talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.first]);
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
                if (ImGui::Selectable(("Id: " + std::to_string(uiData.loadoutSolverPageResults[n])).c_str(), is_selected)) {
                    uiData.selectedFilteredSkillsetIndex = n;
                    uiData.selectedFilteredSkillset = uiData.loadoutSolverPageResults[n];
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
        ImGui::Text("Prefix:");
        ImGui::InputText("##loadoutSolverSkillsetPrefixInputText", &uiData.loadoutSolverSkillsetPrefix, 0, TextFilters::FilterNameLetters);
        if (ImGui::Button("Add selected to loadout##loadoutSolverAddToLoadoutButton") && uiData.selectedFilteredSkillsetIndex >= 0) {
            std::shared_ptr<Engine::TalentSkillset> sk = Engine::skillsetIndexToSkillset(
                talentTreeCollection.activeTree(),
                talentTreeCollection.activeTreeData().treeDAGInfo,
                uiData.selectedFilteredSkillset
            );
            std::string prefix = uiData.loadoutSolverSkillsetPrefix == "" ? "Solved loadout " : uiData.loadoutSolverSkillsetPrefix + " ";
            sk->name = prefix + std::to_string(uiData.selectedFilteredSkillset);
            Engine::applyPreselectedTalentsToSkillset(talentTreeCollection.activeTree(), sk);
            talentTreeCollection.activeTree().loadout.push_back(sk);
            ImGui::OpenPopup("Add to loadout successfull");
        }
        if (ImGui::Button("Add all in page to loadout##loadoutSolverAddAllInPageToLoadoutButton")) {
            for (uint64_t& skillsetIndex : uiData.loadoutSolverPageResults) {
                std::shared_ptr<Engine::TalentSkillset> sk = Engine::skillsetIndexToSkillset(
                    talentTreeCollection.activeTree(),
                    talentTreeCollection.activeTreeData().treeDAGInfo,
                    skillsetIndex
                );
                std::string prefix = uiData.loadoutSolverSkillsetPrefix == "" ? "Solved loadout " : uiData.loadoutSolverSkillsetPrefix + " ";
                sk->name = prefix + std::to_string(skillsetIndex);
                Engine::applyPreselectedTalentsToSkillset(talentTreeCollection.activeTree(), sk);
                talentTreeCollection.activeTree().loadout.push_back(sk);
            }
            ImGui::OpenPopup("Add to loadout successfull");
        }
        int maxAddRandomLimit = static_cast<int>(talentTreeCollection.activeTreeData().treeDAGInfo->filteredCombinations[uiData.loadoutSolverTalentPointSelection].size());
        ImGui::SliderInt("##loadoutSolverAddRandomToLoadoutSlider", &uiData.loadoutSolverAddRandomLoadoutCount, 1, uiData.loadoutSolverAddAllLimit > maxAddRandomLimit ? maxAddRandomLimit : uiData.loadoutSolverAddAllLimit, "%d", ImGuiSliderFlags_AlwaysClamp);
        if (ImGui::Button(("Add " + std::to_string(uiData.loadoutSolverAddRandomLoadoutCount) + " random skillsets to loadout").c_str())) {
            std::random_device rd;     // only used once to initialise (seed) engine
            std::mt19937 rng(rd());    // random-number engine used (Mersenne-Twister in this case)

            auto& SINDintPairs = talentTreeCollection.activeTreeData().treeDAGInfo->filteredCombinations[uiData.loadoutSolverTalentPointSelection];
            if (uiData.loadoutSolverAddRandomLoadoutCount > SINDintPairs.size()) {
                uiData.loadoutSolverAddRandomLoadoutCount = static_cast<int>(SINDintPairs.size());
            }
            std::vector<int> randomIndices;
            randomIndices.reserve(uiData.loadoutSolverAddRandomLoadoutCount);

            int startIndex = -1;
            while (randomIndices.size() < uiData.loadoutSolverAddRandomLoadoutCount) {
                //get random number between int(SINDintPairs.size() * 1.0f / uiData.loadoutSolverAddRandomLoadoutCount + 0.5f)
                int randomIncrementInterval = static_cast<int>((SINDintPairs.size() - startIndex - 1) * 1.0f / (uiData.loadoutSolverAddRandomLoadoutCount - randomIndices.size()) + 0.5f);
                std::uniform_int_distribution<int> uni(1, randomIncrementInterval); // guaranteed unbiased
                startIndex += uni(rng);
                randomIndices.emplace_back(startIndex);
            }

            for (int randomIndex : randomIndices) {
                std::shared_ptr<Engine::TalentSkillset> sk = Engine::skillsetIndexToSkillset(
                    talentTreeCollection.activeTree(),
                    talentTreeCollection.activeTreeData().treeDAGInfo,
                    talentTreeCollection.activeTreeData().treeDAGInfo->filteredCombinations[uiData.loadoutSolverTalentPointSelection][randomIndex]
                );
                std::string prefix = uiData.loadoutSolverSkillsetPrefix == "" ? "Solved loadout " : uiData.loadoutSolverSkillsetPrefix + " ";
                sk->name = prefix + std::to_string(talentTreeCollection.activeTreeData().treeDAGInfo->filteredCombinations[uiData.loadoutSolverTalentPointSelection][randomIndex]);
                Engine::applyPreselectedTalentsToSkillset(talentTreeCollection.activeTree(), sk);
                talentTreeCollection.activeTree().loadout.push_back(sk);
            }
            ImGui::OpenPopup("Add to loadout successfull");
        }
        if (ImGui::Button("Add all to loadout##loadoutSolverAddAllToLoadoutButton")) {
            size_t count = 0;
            for (uint64_t& skillsetIndex : talentTreeCollection.activeTreeData().treeDAGInfo->filteredCombinations[uiData.loadoutSolverTalentPointSelection]) {
                std::shared_ptr<Engine::TalentSkillset> sk = Engine::skillsetIndexToSkillset(
                    talentTreeCollection.activeTree(),
                    talentTreeCollection.activeTreeData().treeDAGInfo,
                    skillsetIndex
                );
                std::string prefix = uiData.loadoutSolverSkillsetPrefix == "" ? "Solved loadout " : uiData.loadoutSolverSkillsetPrefix + " ";
                sk->name = prefix + std::to_string(skillsetIndex);
                Engine::applyPreselectedTalentsToSkillset(talentTreeCollection.activeTree(), sk);
                talentTreeCollection.activeTree().loadout.push_back(sk);
                count++;
                if (count >= uiData.loadoutSolverAddAllLimit) {
                    break;
                }
            }
            ImGui::OpenPopup("Add to loadout successfull");
        }
        ImGui::SameLine();
        ImGui::Text("(Limited to %d)", uiData.loadoutSolverAddAllLimit);
    }

    int getResultsPage(UIData& uiData, TalentTreeCollection& talentTreeCollection, int pageNumber) {
        std::vector<Engine::SIND>& filteredResults = talentTreeCollection.activeTreeData().treeDAGInfo->filteredCombinations[uiData.loadoutSolverTalentPointSelection];
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
            uiData.selectedFilteredSkillset = uiData.loadoutSolverPageResults[0];
        }
        return maxPage;
    }
}