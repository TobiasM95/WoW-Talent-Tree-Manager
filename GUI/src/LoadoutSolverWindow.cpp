#include "LoadoutSolverWindow.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"

namespace TTM {
    static void AttachLoadoutSolverTooltip(Engine::Talent talent)
    {
        if (ImGui::IsItemHovered())
        {
            std::string idLabel = "Id: " + std::to_string(talent.index) + ", Pos: (" + std::to_string(talent.row) + ", " + std::to_string(talent.column) + ")";
            if (talent.type != Engine::TalentType::SWITCH) {
                ImGui::BeginTooltip();
                ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
                ImGui::Text(talent.getName().c_str());
                ImGui::PopFont();
                //ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(idLabel.c_str()).x);
                ImGui::Text(idLabel.c_str());
                std::string talentTypeString;
                switch (talent.type) {
                case Engine::TalentType::ACTIVE: {talentTypeString = "(active)"; }break;
                case Engine::TalentType::PASSIVE: {talentTypeString = "(passive)"; }break;
                }
                ImGui::TextColored(ImVec4(0.92f, 0.44f, 0.44f, 1.0f), talentTypeString.c_str());
                ImGui::Text(("Points: " + std::to_string(talent.points) + "/" + std::to_string(talent.maxPoints) + ", points required: " + std::to_string(talent.pointsRequired)).c_str());
                ImGui::Spacing();
                ImGui::Spacing();

                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 15.0f);
                ImGui::TextColored(ImVec4(0.533f, 0.533f, 1.0f, 1.0f), talent.getDescription().c_str());
                ImGui::PopTextWrapPos();

                ImGui::EndTooltip();
            }
            else {
                ImGui::BeginTooltip();
                ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
                ImGui::Text(talent.getName().c_str());
                ImGui::PopFont();
                //ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(idLabel.c_str()).x);
                ImGui::Text(idLabel.c_str());
                ImGui::TextColored(ImVec4(0.92f, 0.44f, 0.44f, 1.0f), "(switch, ctrl+click)");
                ImGui::Text(("Points: " + std::to_string(talent.points) + "/" + std::to_string(talent.maxPoints) + ", points required: " + std::to_string(talent.pointsRequired)).c_str());
                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.533f, 0.533f, 1.0f, 1.0f), talent.getDescription().c_str());

                ImGui::EndTooltip();
            }
        }
    }

    void RenderLoadoutSolverWindow(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
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

            placeLoadoutSolverTreeElements(uiData, talentTreeCollection);

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
                        talentTreeCollection.activeTree().name.c_str(), uiData.allCombinationsAdded, talentTreeCollection.activeTree().maxTalentPoints);
                    ImGui::Text("Hint: Include/Exclude talents on the left, then press filter to view all valid combinations. Green/Yellow = must have, Red = must not have.");
                    ImGui::PopTextWrapPos();
                    if (ImGui::Button("Filter")) {

                    }
                    ImGui::Separator();
                    ImGui::Text("Number of talent points");
                    if (ImGui::BeginListBox("##loadoutSolverTalentPointsListbox", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
                    {
                        for (int n = 0; n < talentTreeCollection.activeTree().maxTalentPoints; n++)
                        {
                            const bool is_selected = (uiData.loadoutSolverTalentPointSelection == n);
                            if (ImGui::Selectable(std::to_string(n + 1).c_str(), is_selected)) {
                                uiData.loadoutSolverTalentPointSelection = n;
                            }

                            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                            if (is_selected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndListBox();
                    }
                }break;
                }
            ImGui::End();
        }
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Talent tree too large", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Talent tree has too many possible talent points!\nMaximum number of possible talent points spent is 64.");

            ImGui::SetItemDefaultFocus();
            if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }
    }

    void placeLoadoutSolverTreeElements(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        Engine::TalentTree& tree = talentTreeCollection.activeTree();

        if (!talentTreeCollection.activeTreeData().isTreeSolveProcessed) {
            ImGui::SetCursorPos(ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 75.0f, ImGui::GetContentRegionAvail().y * 0.5f - 12.5f));
            if (ImGui::Button("Process tree", ImVec2(150, 25))) {
                clearSolvingProcess(uiData, talentTreeCollection);
                if (talentTreeCollection.activeTree().maxTalentPoints > 64) {
                    ImGui::OpenPopup("Talent tree too large");
                    return;
                }
                talentTreeCollection.activeTreeData().skillsetFilter = std::make_shared<Engine::TalentSkillset>();
                talentTreeCollection.activeTreeData().skillsetFilter->name = "SolverSkillset";
                for (auto& talent : tree.orderedTalents) {
                    talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.first] = 0;
                }
                Engine::clearTree(tree);
                talentTreeCollection.activeTreeData().treeDAGInfo = Engine::countConfigurationsParallel(tree);
                talentTreeCollection.activeTreeData().isTreeSolveProcessed = true;
                for (auto& talentPointsCombinbations : talentTreeCollection.activeTreeData().treeDAGInfo->allCombinations) {
                    for (auto& combinations : talentPointsCombinbations) {
                        uiData.allCombinationsAdded += combinations.second;
                    }
                }
            }
            return;
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
            float posX = talentWindowPaddingX + talent.second->column * 2 * talentHalfSpacing + talentPadding;
            float posY = talentWindowPaddingY + talent.second->row * 2 * talentHalfSpacing + talentPadding;
            bool changedColor = false;
            if (talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.first] > 0 
                && talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.first] < talent.second->maxPoints) {
                ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.2f, 0.9f, 0.2f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor(0.25f, 1.0f, 0.25f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(0.4f, 1.0f, 0.4f));
                ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(0, 0, 0));
                changedColor = true;
            }
            else if (talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.first] == talent.second->maxPoints) {
                ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.9f, 0.73f, 0.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor(1.0f, 0.82f, 0.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(1.0f, 0.95f, 0.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(0, 0, 0));
                changedColor = true;
            }
            else if (talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.first] < 0) {
                ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.95f, 0.23f, 0.23f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor(1.0f, 0.33f, 0.33f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(1.0f, 0.43f, 0.43f));
                ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(0, 0, 0));
                changedColor = true;
            }
            ImGui::SetCursorPos(ImVec2(posX, posY));
            ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f + (talent.second->type == Engine::TalentType::SWITCH) * 9.0f * uiData.treeEditorZoomFactor + (talent.second->type == Engine::TalentType::PASSIVE) * 15.0f * uiData.treeEditorZoomFactor);
            ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 0.0f + (talent.second->type == Engine::TalentType::SWITCH) * 9.0f * uiData.treeEditorZoomFactor + (talent.second->type == Engine::TalentType::PASSIVE) * 15.0f * uiData.treeEditorZoomFactor);
            if (ImGui::Button((std::to_string(talent.second->index) + "##" + talent.second->name).c_str(), ImVec2(static_cast<float>(talentSize), static_cast<float>(talentSize)))) {
                talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.first] += 1;
                if (talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.first] > talent.second->maxPoints) {
                    talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.first] = -1;
                }

            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                uiData.loadoutEditorRightClickIndex = talent.first;
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right) && talent.first == uiData.loadoutEditorRightClickIndex) {
                talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.first] -= 1;
                if (talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.first] < -1) {
                    talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent.first] = talent.second->maxPoints;
                }
            }
            ImGui::PopStyleVar(2);
            ImGui::PopFont();
            if (changedColor) {
                ImGui::PopStyleColor(4);
            }
            AttachLoadoutSolverTooltip(*talent.second);
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