#include "LoadoutEditorWindow.h"

namespace TTM {
    static void AttachLoadoutEditTooltip(Engine::Talent talent)
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
                ImGui::TextColored(ImVec4(0.92f, 0.24f, 0.24f, 1.0f), talentTypeString.c_str());
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
                ImGui::TextColored(ImVec4(0.92f, 0.24f, 0.24f, 1.0f), "(switch)");
                ImGui::Text(("Points: " + std::to_string(talent.points) + "/" + std::to_string(talent.maxPoints) + ", points required: " + std::to_string(talent.pointsRequired)).c_str());
                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.533f, 0.533f, 1.0f, 1.0f), talent.getDescription().c_str());

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
                    ImGui::Text("Skillset name: %s", talentTreeCollection.activeSkillset()->name.c_str());
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
            }
        }
        ImGui::End();
    }

    void placeLoadoutEditorTreeElements(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        Engine::TalentTree& tree = talentTreeCollection.activeTree();

        if (tree.loadout.size() == 0) {
            if (ImGui::Button("Create first skillset")) {
                Engine::createSkillset(tree);
                Engine::activateSkillset(tree, tree.loadout.size() - 1);
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
            float posX = talentWindowPaddingX + talent.second->column * 2 * talentHalfSpacing + talentPadding;
            float posY = talentWindowPaddingY + talent.second->row * 2 * talentHalfSpacing + talentPadding;
            bool changedColor = false;
            if (talent.second->points > 0 && talent.second->points < talent.second->maxPoints) {
                ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.2f, 0.9f, 0.2f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor(0.25f, 1.0f, 0.25f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(0.4f, 1.0f, 0.4f));
                ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(0, 0, 0));
                changedColor = true;
            }
            else if (talent.second->points == talent.second->maxPoints) {
                ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.9f, 0.73f, 0.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor(1.0f, 0.82f, 0.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(1.0f, 0.95f, 0.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(0, 0, 0));
                changedColor = true;
            }
            ImGui::SetCursorPos(ImVec2(posX, posY));
            ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f + (talent.second->type == Engine::TalentType::SWITCH) * 8.0f * uiData.treeEditorZoomFactor + (talent.second->type == Engine::TalentType::PASSIVE) * 15.0f * uiData.treeEditorZoomFactor);
            ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 0.0f + (talent.second->type == Engine::TalentType::SWITCH) * 8.0f * uiData.treeEditorZoomFactor + (talent.second->type == Engine::TalentType::PASSIVE) * 15.0f * uiData.treeEditorZoomFactor);
            if (ImGui::Button(std::to_string(talent.second->points).c_str(), ImVec2(static_cast<float>(talentSize), static_cast<float>(talentSize)))) {
                //TTMTODO: loadout editor talent selection
            }
            ImGui::PopStyleVar(2);
            ImGui::PopFont();
            if (changedColor) {
                ImGui::PopStyleColor(4);
            }
            AttachLoadoutEditTooltip(*talent.second);
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