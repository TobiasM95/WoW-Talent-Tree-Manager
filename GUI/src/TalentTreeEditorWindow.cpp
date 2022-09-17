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

#include "TalentTreeEditorWindow.h"

#include <fstream>
#include <string>
#include <algorithm>
#include <tuple>

#include "TTMEnginePresets.h"
#include "Updater.h" //for pastebin share

namespace TTM {
    //Talent tooltip for the tree editor view
    static void AttachTalentEditTooltip(const UIData& uiData, Engine::Talent_s talent)
    {
        if (ImGui::IsItemHovered() && !ImGui::IsKeyDown(ImGuiKey_LeftAlt))
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
                ImGui::Text(("Max points: " + std::to_string(talent->maxPoints) + ", points required: " + std::to_string(talent->pointsRequired)).c_str());
                ImGui::Spacing();
                ImGui::Spacing();

                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 15.0f);
                for (int i = 0; i < talent->maxPoints - 1; i++) {
                    ImGui::TextUnformattedColored(Presets::GET_TOOLTIP_TALENT_DESC_COLOR(uiData.style), talent->descriptions[i].c_str());
                    ImGui::Separator();
                }
                ImGui::TextUnformattedColored(Presets::GET_TOOLTIP_TALENT_DESC_COLOR(uiData.style), talent->descriptions[talent->maxPoints - 1].c_str());
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
                ImGui::Text(("Max points: 1, points required: " + std::to_string(talent->pointsRequired)).c_str());
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
                ImGui::Text(("Max points: 1, points required: " + std::to_string(talent->pointsRequired)).c_str());
                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 15.0f);
                ImGui::TextUnformattedColored(Presets::GET_TOOLTIP_TALENT_DESC_COLOR(uiData.style), talent->descriptions[1].c_str());
                ImGui::PopTextWrapPos();

                ImGui::EndTooltip();
            }
        }
    }

    void RenderTalentTreeEditorWindow(UIData& uiData, TalentTreeCollection& talentTreeCollection) {

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

            placeTreeEditorTreeElements(uiData, talentTreeCollection);

            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            if (ImGui::BeginPopupModal("Cycle detected", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Talent will introduce a cycle to the tree and invalidate it! Check talent children and parents and remove talents that will create a loop.");

                ImGui::SetItemDefaultFocus();
                if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
                ImGui::EndPopup();
            }
            if (ImGui::BeginPopupModal("Pre filled talent error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Talent cannot be pre filled if it is not a root talent or if no parent is pre filled!");
                ImGui::Text("This warning could apply to child talents that are pre filled.");

                ImGui::SetItemDefaultFocus();
                if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
                ImGui::EndPopup();
            }

            ImGui::EndChild();
        }
        ImGui::End();
        ImGui::PopStyleVar();
        if (ImGui::Begin("SettingsWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
            if (ImGui::Button("Tree Information", ImVec2(ImGui::GetContentRegionAvail().x / 3.f, 25))) {
                uiData.treeEditPage = TreeEditPage::TreeInformation;
            }
            ImGui::SameLine();
            if (ImGui::Button("Tree Editor", ImVec2(ImGui::GetContentRegionAvail().x / 2.f, 25))) {
                uiData.treeEditPage = TreeEditPage::TreeEdit;
            }
            ImGui::SameLine();
            if (ImGui::Button("Save/Load Trees", ImVec2(ImGui::GetContentRegionAvail().x, 25))) {
                if (uiData.treeEditPage != TreeEditPage::SaveLoadTree) {
                    uiData.treeEditPage = TreeEditPage::SaveLoadTree;
                    uiData.treeEditorImportTreeString = "";
                    uiData.treeEditorExportTreeString = "";
                }
            }
            ImGui::Spacing();
            switch (uiData.treeEditPage) {
            case TreeEditPage::TreeInformation: {
                ImGui::Text("Tree name:");
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputText("##TreeNameInputText", &talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.name,
                    ImGuiInputTextFlags_CallbackCharFilter, TextFilters::FilterNameLetters);

                ImGui::Text("Preset: ");
                ImGui::SameLine(ImGui::GetContentRegionAvail().x / 3.f);
                ImGui::Text(talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.presetName.c_str());

                ImGui::Text("Tree type:");
                int currentTreeType = static_cast<int>(talentTreeCollection.activeTree().type);
                ImGui::Combo("##talentCreationTypeCombo", &currentTreeType, "Class tree\0Spec tree");
                if (static_cast<Engine::TreeType>(currentTreeType) != talentTreeCollection.activeTree().type) {
                    talentTreeCollection.activeTree().type = static_cast<Engine::TreeType>(currentTreeType);
                    talentTreeCollection.activeTree().presetName = "custom";
                    Engine::updateNodeCountAndMaxTalentPointsAndMaxID(talentTreeCollection.activeTree());
                    Engine::updateOrderedTalentList(talentTreeCollection.activeTree());
                    Engine::updateRequirementSeparatorInfo(talentTreeCollection.activeTree());
                    Engine::validateLoadout(talentTreeCollection.activeTree(), true);
                    clearSolvingProcess(uiData, talentTreeCollection);

                    uiData.treeEditorSelectedTalent = nullptr;
                }

                ImGui::Text("Node count: ");
                ImGui::SameLine(ImGui::GetContentRegionAvail().x / 3.f);
                ImGui::Text("%d", talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.nodeCount);

                ImGui::Text("Max talent points:");
                ImGui::SameLine(ImGui::GetContentRegionAvail().x / 3.f);
                ImGui::Text("%d", talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.maxTalentPoints);

                ImGui::Text("Tree description:");
                ImGui::InputTextMultiline("##TreeDescriptionInputText", &talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.treeDescription,
                    ImGui::GetContentRegionAvail());
            }break;
            case TreeEditPage::TreeEdit: {
                ImGui::Text("Note: Editing the tree can invalidate the loadout!");
                if (uiData.talentJustSelected)
                    ImGui::SetNextItemOpen(false, ImGuiCond_Always);
                //TTMTODO: At some point this Header content and the Edit/Delete header content can be put into a single function with uiData parameters
                if (ImGui::CollapsingHeader("Create Talent"))
                {
                    ImGui::Text("Talent type:");
                    int currentType = static_cast<int>(uiData.treeEditorCreationTalent->type);
                    ImGui::Combo("##talentCreationTypeCombo", &currentType, "Active\0Passive\0Switch");
                    uiData.treeEditorCreationTalent->type = static_cast<Engine::TalentType>(currentType);

                    ImGui::Text("Name:");
                    ImGui::InputText("##talentCreationNameInput", &uiData.treeEditorCreationTalent->name,
                        ImGuiInputTextFlags_CallbackCharFilter, TextFilters::FilterNameLetters);
                    float textInputWidth = ImGui::CalcItemWidth();

                    if (uiData.treeEditorCreationTalent->type != Engine::TalentType::SWITCH) ImGui::BeginDisabled();
                    ImGui::Text("Name (switch):");
                    ImGui::InputText("##talentCreationNameSwitchInput", &uiData.treeEditorCreationTalent->nameSwitch,
                        ImGuiInputTextFlags_CallbackCharFilter, TextFilters::FilterNameLetters);
                    if (uiData.treeEditorCreationTalent->type != Engine::TalentType::SWITCH) ImGui::EndDisabled();

                    ImGui::Text("Icon name:");
                    const char* iconNameComboPreviewValue = uiData.treeEditorCreationTalent->iconName.first.c_str();
                    if (ImGui::BeginCombo("##talentCreationIconNameCombo", iconNameComboPreviewValue))
                    {
                        for (auto& iconNameTexturePair : uiData.iconMap)
                        {
                            if (uiData.treeEditorCreationIconNameFilter != "" && Engine::simplifyString(iconNameTexturePair.first).find(Engine::simplifyString(uiData.treeEditorCreationIconNameFilter)) == std::string::npos) {
                                continue;
                            }
                            const bool is_selected = (iconNameComboPreviewValue == iconNameTexturePair.first);
                            if (ImGui::Selectable(iconNameTexturePair.first.c_str(), is_selected)) {
                                uiData.treeEditorCreationTalent->iconName.first = iconNameTexturePair.first;
                                uiData.treeEditorCreationTalentIcons.first = &uiData.iconMap[iconNameTexturePair.first].first;
                            }

                            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                            if (is_selected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    if(uiData.treeEditorCreationTalentIcons.first && uiData.treeEditorCreationTalentIcons.first->texture){
                        ImGui::Image(uiData.treeEditorCreationTalentIcons.first->texture, ImVec2(40, 40));
                    }
                    else {
                        uiData.treeEditorCreationTalentIcons.first = &uiData.defaultIcon;
                        ImGui::Image(uiData.treeEditorCreationTalentIcons.first->texture, ImVec2(40, 40));
                    }

                    if (uiData.treeEditorCreationTalent->type != Engine::TalentType::SWITCH) ImGui::BeginDisabled();
                    ImGui::Text("Icon name (switch):");
                    const char* iconNameSwitchComboPreviewValue = uiData.treeEditorCreationTalent->iconName.second.c_str();
                    if (ImGui::BeginCombo("##talentCreationIconNameSwitchCombo", iconNameSwitchComboPreviewValue))
                    {
                        for (auto& iconNameTexturePair : uiData.iconMap)
                        {
                            if (uiData.treeEditorCreationIconNameFilter != "" && Engine::simplifyString(iconNameTexturePair.first).find(Engine::simplifyString(uiData.treeEditorCreationIconNameFilter)) == std::string::npos) {
                                continue;
                            }
                            const bool is_selected = (iconNameSwitchComboPreviewValue == iconNameTexturePair.first);
                            if (ImGui::Selectable(iconNameTexturePair.first.c_str(), is_selected)) {
                                uiData.treeEditorCreationTalent->iconName.second = iconNameTexturePair.first;
                                uiData.treeEditorCreationTalentIcons.second = &uiData.iconMap[iconNameTexturePair.first].first;
                            }

                            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                            if (is_selected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    if (uiData.treeEditorCreationTalent->type == Engine::TalentType::SWITCH) {
                        if (uiData.treeEditorCreationTalentIcons.second && uiData.treeEditorCreationTalentIcons.second->texture) {
                            ImGui::Image(uiData.treeEditorCreationTalentIcons.second->texture, ImVec2(40, 40));
                        }
                        else {
                            uiData.treeEditorCreationTalentIcons.second = &uiData.defaultIcon;
                            ImGui::Image(uiData.treeEditorCreationTalentIcons.second->texture, ImVec2(40, 40));
                        }
                    }
                    if (uiData.treeEditorCreationTalent->type != Engine::TalentType::SWITCH) ImGui::EndDisabled();

                    ImGui::Text("Icon name filter:");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(textInputWidth - ImGui::GetCursorPosX() - 100);
                    ImGui::InputText("##talentCreationIconNameFilterInput", &uiData.treeEditorCreationIconNameFilter);
                    ImGui::SameLine();
                    if (ImGui::Button("Clear##talentCreationIconNameFilterClearButton", ImVec2(100, 0))) {
                        uiData.treeEditorCreationIconNameFilter = "";
                    }

                    if (uiData.treeEditorCreationTalent->type == Engine::TalentType::SWITCH || uiData.treeEditorCreationTalent->type == Engine::TalentType::ACTIVE) ImGui::BeginDisabled();
                    ImGui::Text("Maximum talent points:");
                    ImGui::SliderInt("##talentCreationMaxPointsSlider", &uiData.treeEditorCreationTalent->maxPoints, 1, 9, "%d", ImGuiSliderFlags_AlwaysClamp);
                    if (uiData.treeEditorCreationTalent->type == Engine::TalentType::SWITCH || uiData.treeEditorCreationTalent->type == Engine::TalentType::ACTIVE) {
                        ImGui::EndDisabled();
                        uiData.treeEditorCreationTalent->maxPoints = 1;
                    }

                    ImGui::Text("Points required:");
                    ImGui::SliderInt("##talentCreationPtsReqSlider", &uiData.treeEditorCreationTalent->pointsRequired, 0, 50, "%d", ImGuiSliderFlags_AlwaysClamp);

                    ImGui::Text("Is talent pre-filled:");
                    ImGui::Checkbox("##talentEditPreFilledCheckbox", &uiData.treeEditorCreationTalent->preFilled);

                    ImGui::Text("Descriptions:");
                    if (uiData.treeEditorCreationTalent->descriptions.size() != uiData.treeEditorCreationTalent->maxPoints + (uiData.treeEditorCreationTalent->type == Engine::TalentType::SWITCH)) {
                        uiData.treeEditorCreationTalent->descriptions.resize(uiData.treeEditorCreationTalent->maxPoints + (uiData.treeEditorCreationTalent->type == Engine::TalentType::SWITCH));
                    }
                    for (int i = 0; i < uiData.treeEditorCreationTalent->descriptions.size(); i++) {
                        ImGui::InputTextMultiline(("##talentCreationDescriptionInput" + std::to_string(i)).c_str(), &uiData.treeEditorCreationTalent->descriptions[i],
                            ImVec2(0, 5 * ImGui::CalcTextSize("@").y));
                    }

                    ImGui::Text("Talent row:");
                    ImGui::SliderInt("##talentCreationRowSlider", &uiData.treeEditorCreationTalent->row, 1, talentTreeCollection.activeTree().maxRowLimit, "%d", ImGuiSliderFlags_AlwaysClamp);

                    ImGui::Text("Talent column:");
                    ImGui::SliderInt("##talentCreationColumnSlider", &uiData.treeEditorCreationTalent->column, 1, talentTreeCollection.activeTree().maxColumnLimit, "%d", ImGuiSliderFlags_AlwaysClamp);

                    std::vector<std::string> talentComboList;
                    std::map<int, Engine::Talent_s> comboIndexTalentMap;
                    struct Funcs { static bool ItemGetter(void* data, int n, const char** out_str) { *out_str = ((*((std::vector<std::string>*)data))[n].c_str()); return true; } };
                    if (uiData.treeEditorCreationTalentParentsPlaceholder.size() > 0 || uiData.treeEditorCreationTalentChildrenPlaceholder.size() > 0) {
                        //create array of talentIndex: talentName strings to show in combo menu and create map<comboindex, talent> to select correct talent
                        //not sure if it's guaranteed to have talentIndex == comboindex, therefore map
                        int comboIndex = 0;
                        for (auto& talent : talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.orderedTalents) {
                            comboIndexTalentMap[comboIndex] = talent.second;
                            if (talent.second->type != Engine::TalentType::SWITCH) {
                                talentComboList.push_back(std::to_string(talent.first) + ": " + talent.second->name);
                            }
                            else {
                                talentComboList.push_back(std::to_string(talent.first) + ": " + talent.second->name + " / " + talent.second->nameSwitch);
                            }
                            comboIndex++;
                        }
                    }
                    ImGui::Text("Parents:");
                    int parentCount = static_cast<int>(uiData.treeEditorCreationTalentParentsPlaceholder.size());
                    ImGui::SliderInt("##talentCreationParentCountSlider", &parentCount, 0, talentTreeCollection.activeTree().nodeCount, "%d", ImGuiSliderFlags_AlwaysClamp);
                    if (parentCount != uiData.treeEditorCreationTalentParentsPlaceholder.size())
                        uiData.treeEditorCreationTalentParentsPlaceholder.resize(parentCount);

                    for (int i = 0; i < uiData.treeEditorCreationTalentParentsPlaceholder.size(); i++) {
                        // Simplified one-liner Combo() using an accessor function
                        ImGui::Combo(
                            ("##talentCreationParentCombo" + std::to_string(i)).c_str(),
                            &uiData.treeEditorCreationTalentParentsPlaceholder[i],
                            &Funcs::ItemGetter,
                            &talentComboList,
                            static_cast<int>(talentComboList.size()));
                    }
                    ImGui::Text("Children:");
                    int childrenCount = static_cast<int>(uiData.treeEditorCreationTalentChildrenPlaceholder.size());
                    ImGui::SliderInt("##talentCreationChildrenCountSlider", &childrenCount, 0, talentTreeCollection.activeTree().nodeCount, "%d", ImGuiSliderFlags_AlwaysClamp);
                    if (childrenCount != uiData.treeEditorCreationTalentChildrenPlaceholder.size())
                        uiData.treeEditorCreationTalentChildrenPlaceholder.resize(childrenCount);

                    for (int i = 0; i < uiData.treeEditorCreationTalentChildrenPlaceholder.size(); i++) {
                        // Simplified one-liner Combo() using an accessor function
                        ImGui::Combo(
                            ("##talentCreationChildrenCombo" + std::to_string(i)).c_str(),
                            &uiData.treeEditorCreationTalentChildrenPlaceholder[i],
                            &Funcs::ItemGetter,
                            &talentComboList,
                            static_cast<int>(talentComboList.size()));
                    }

                    if (ImGui::Button("Create Talent##talentCreationCreateButton")) {
                        //Validates talent and inserts it into tree, otherwise show modal popup
                        validateAndInsertTalent(uiData, talentTreeCollection, comboIndexTalentMap);
                        loadActiveIcons(uiData, talentTreeCollection, true);
                    }
                }
                if (uiData.talentJustSelected) {
                    ImGui::SetNextItemOpen(true, ImGuiCond_Always);
                }
                if (ImGui::CollapsingHeader("Edit/Delete Talent"))
                {
                    if (uiData.treeEditorSelectedTalent != nullptr) {

                        ImGui::Text("Talent type:");
                        int currentType = static_cast<int>(uiData.treeEditorSelectedTalent->type);
                        ImGui::Combo("##talentEditTypeCombo", &currentType, "Active\0Passive\0Switch");
                        uiData.treeEditorSelectedTalent->type = static_cast<Engine::TalentType>(currentType);

                        ImGui::Text("Name:");
                        ImGui::InputText("##talentEditNameInput", &uiData.treeEditorSelectedTalent->name, 
                            ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CallbackCharFilter, TextFilters::FilterNameLetters);
                        float textInputWidth = ImGui::CalcItemWidth();

                        if (uiData.treeEditorSelectedTalent->type != Engine::TalentType::SWITCH) ImGui::BeginDisabled();
                        ImGui::Text("Name (switch):");
                        ImGui::InputText("##talentEditNameSwitchInput", &uiData.treeEditorSelectedTalent->nameSwitch, 
                            ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CallbackCharFilter, TextFilters::FilterNameLetters);
                        if (uiData.treeEditorSelectedTalent->type != Engine::TalentType::SWITCH) ImGui::EndDisabled();

                        ImGui::Text("Icon name:");
                        const char* iconNameComboPreviewValue = uiData.treeEditorSelectedTalent->iconName.first.c_str();
                        if (ImGui::BeginCombo("##talentEditIconNameCombo", iconNameComboPreviewValue))
                        {
                            for (auto& iconNameTexturePair : uiData.iconMap)
                            {
                                if (uiData.treeEditorEditIconNameFilter != "" && Engine::simplifyString(iconNameTexturePair.first).find(Engine::simplifyString(uiData.treeEditorEditIconNameFilter)) == std::string::npos) {
                                    continue;
                                }
                                const bool is_selected = (iconNameComboPreviewValue == iconNameTexturePair.first);
                                if (ImGui::Selectable(iconNameTexturePair.first.c_str(), is_selected)) {
                                    uiData.treeEditorSelectedTalent->iconName.first = iconNameTexturePair.first;
                                    uiData.treeEditorSelectedTalentIcons.first = &uiData.iconMap[iconNameTexturePair.first].first;
                                }

                                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                                if (is_selected)
                                    ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }
                        if (uiData.treeEditorSelectedTalentIcons.first && uiData.treeEditorSelectedTalentIcons.first->texture) {
                            ImGui::Image(uiData.treeEditorSelectedTalentIcons.first->texture, ImVec2(40, 40));
                        }
                        else {
                            uiData.treeEditorSelectedTalentIcons.first = &uiData.defaultIcon;
                            ImGui::Image(uiData.treeEditorSelectedTalentIcons.first->texture, ImVec2(40, 40));
                        }

                        if (uiData.treeEditorSelectedTalent->type != Engine::TalentType::SWITCH) ImGui::BeginDisabled();
                        ImGui::Text("Icon name (switch):");
                        const char* iconNameSwitchComboPreviewValue = uiData.treeEditorSelectedTalent->iconName.second.c_str();
                        if (ImGui::BeginCombo("##talentEditIconNameSwitchCombo", iconNameSwitchComboPreviewValue))
                        {
                            for (auto& iconNameTexturePair : uiData.iconMap)
                            {
                                if (uiData.treeEditorEditIconNameFilter != "" && Engine::simplifyString(iconNameTexturePair.first).find(Engine::simplifyString(uiData.treeEditorEditIconNameFilter)) == std::string::npos) {
                                    continue;
                                }
                                const bool is_selected = (iconNameSwitchComboPreviewValue == iconNameTexturePair.first);
                                if (ImGui::Selectable(iconNameTexturePair.first.c_str(), is_selected)) {
                                    uiData.treeEditorSelectedTalent->iconName.second = iconNameTexturePair.first;
                                    uiData.treeEditorSelectedTalentIcons.second = &uiData.iconMap[iconNameTexturePair.first].first;
                                }

                                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                                if (is_selected)
                                    ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }
                        if (uiData.treeEditorSelectedTalent->type == Engine::TalentType::SWITCH) {
                            if (uiData.treeEditorSelectedTalentIcons.second && uiData.treeEditorSelectedTalentIcons.second->texture) {
                                ImGui::Image(uiData.treeEditorSelectedTalentIcons.second->texture, ImVec2(40, 40));
                            }
                            else {
                                uiData.treeEditorSelectedTalentIcons.second = &uiData.defaultIcon;
                                ImGui::Image(uiData.treeEditorSelectedTalentIcons.second->texture, ImVec2(40, 40));
                            }
                        }
                        if (uiData.treeEditorSelectedTalent->type != Engine::TalentType::SWITCH) ImGui::EndDisabled();

                        ImGui::Text("Icon name filter:");
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(textInputWidth - ImGui::GetCursorPosX() - 100);
                        ImGui::InputText("##talentEditIconNameFilterInput", &uiData.treeEditorEditIconNameFilter);
                        ImGui::SameLine();
                        if (ImGui::Button("Clear##talentEditIconNameFilterClearButton", ImVec2(100, 0))) {
                            uiData.treeEditorEditIconNameFilter = "";
                        }

                        if (uiData.treeEditorSelectedTalent->type == Engine::TalentType::SWITCH || uiData.treeEditorSelectedTalent->type == Engine::TalentType::ACTIVE) ImGui::BeginDisabled();
                        ImGui::Text("Maximum talent points:");
                        ImGui::SliderInt("##talentEditMaxPointsSlider", &uiData.treeEditorSelectedTalent->maxPoints, 1, 9, "%d", ImGuiSliderFlags_AlwaysClamp);
                        if (uiData.treeEditorSelectedTalent->type == Engine::TalentType::SWITCH || uiData.treeEditorSelectedTalent->type == Engine::TalentType::ACTIVE) {
                            ImGui::EndDisabled();
                            uiData.treeEditorSelectedTalent->maxPoints = 1;
                        }

                        ImGui::Text("Points required:");
                        ImGui::SliderInt("##talentEditPtsReqSlider", &uiData.treeEditorSelectedTalent->pointsRequired, 0, 50, "%d", ImGuiSliderFlags_AlwaysClamp);

                        ImGui::Text("Is talent pre-filled:");
                        ImGui::Checkbox("##talentEditPreFilledCheckbox", &uiData.treeEditorSelectedTalent->preFilled);

                        ImGui::Text("Descriptions:");
                        if (uiData.treeEditorSelectedTalent->descriptions.size() != uiData.treeEditorSelectedTalent->maxPoints + (uiData.treeEditorSelectedTalent->type == Engine::TalentType::SWITCH)) {
                            uiData.treeEditorSelectedTalent->descriptions.resize(uiData.treeEditorSelectedTalent->maxPoints + (uiData.treeEditorSelectedTalent->type == Engine::TalentType::SWITCH));
                        }
                        for (int i = 0; i < uiData.treeEditorSelectedTalent->descriptions.size(); i++) {
                            ImGui::InputTextMultiline(("##talentEditDescriptionInput" + std::to_string(i)).c_str(), &uiData.treeEditorSelectedTalent->descriptions[i],
                                ImVec2(0, 5 * ImGui::CalcTextSize("@").y));
                        }

                        ImGui::Text("Talent row:");
                        ImGui::SliderInt("##talentEditRowSlider", &uiData.treeEditorSelectedTalent->row, 1, talentTreeCollection.activeTree().maxRowLimit, "%d", ImGuiSliderFlags_AlwaysClamp);

                        ImGui::Text("Talent column:");
                        ImGui::SliderInt("##talentEditColumnSlider", &uiData.treeEditorSelectedTalent->column, 1, talentTreeCollection.activeTree().maxColumnLimit, "%d", ImGuiSliderFlags_AlwaysClamp);

                        std::vector<std::string> talentComboList;
                        std::map<int, Engine::Talent_s> comboIndexTalentMap;
                        struct Funcs { static bool ItemGetter(void* data, int n, const char** out_str) { *out_str = ((*((std::vector<std::string>*)data))[n].c_str()); return true; } };
                        if (uiData.treeEditorSelectedTalentParentsPlaceholder.size() > 0 || uiData.treeEditorSelectedTalentChildrenPlaceholder.size() > 0) {
                            //create array of talentIndex: talentName strings to show in combo menu and create map<comboindex, talent> to select correct talent
                            //not sure if it's guaranteed to have talentIndex == comboindex, therefore map
                            int comboIndex = 0;
                            for (auto& talent : talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.orderedTalents) {
                                comboIndexTalentMap[comboIndex] = talent.second;
                                if (talent.second->type != Engine::TalentType::SWITCH) {
                                    talentComboList.push_back(std::to_string(talent.first) + ": " + talent.second->name);
                                }
                                else {
                                    talentComboList.push_back(std::to_string(talent.first) + ": " + talent.second->name + " / " + talent.second->nameSwitch);
                                }
                                comboIndex++;
                            }
                        }
                        ImGui::Text("Parents:");
                        int parentCount = static_cast<int>(uiData.treeEditorSelectedTalentParentsPlaceholder.size());
                        ImGui::SliderInt("##talentEditParentCountSlider", &parentCount, 0, talentTreeCollection.activeTree().nodeCount - 1, "%d", ImGuiSliderFlags_AlwaysClamp);
                        if (parentCount != uiData.treeEditorSelectedTalentParentsPlaceholder.size())
                            uiData.treeEditorSelectedTalentParentsPlaceholder.resize(parentCount);

                        for (int i = 0; i < uiData.treeEditorSelectedTalentParentsPlaceholder.size(); i++) {
                            // Simplified one-liner Combo() using an accessor function
                            ImGui::Combo(
                                ("##talentEditParentCombo" + std::to_string(i)).c_str(),
                                &uiData.treeEditorSelectedTalentParentsPlaceholder[i],
                                &Funcs::ItemGetter,
                                &talentComboList,
                                static_cast<int>(talentComboList.size()));
                        }
                        ImGui::Text("Children:");
                        int childrenCount = static_cast<int>(uiData.treeEditorSelectedTalentChildrenPlaceholder.size());
                        ImGui::SliderInt("##talentEditChildrenCountSlider", &childrenCount, 0, talentTreeCollection.activeTree().nodeCount - 1, "%d", ImGuiSliderFlags_AlwaysClamp);
                        if (childrenCount != uiData.treeEditorSelectedTalentChildrenPlaceholder.size())
                            uiData.treeEditorSelectedTalentChildrenPlaceholder.resize(childrenCount);

                        for (int i = 0; i < uiData.treeEditorSelectedTalentChildrenPlaceholder.size(); i++) {
                            // Simplified one-liner Combo() using an accessor function
                            ImGui::Combo(
                                ("##talentEditChildrenCombo" + std::to_string(i)).c_str(),
                                &uiData.treeEditorSelectedTalentChildrenPlaceholder[i],
                                &Funcs::ItemGetter,
                                &talentComboList,
                                static_cast<int>(talentComboList.size()));
                        }
                        ImGui::Separator();
                        if (ImGui::Button("Reset talent", ImVec2(ImGui::GetContentRegionAvail().x / 4.0f, 0))) {
                            uiData.treeEditorSelectedTalent = std::make_shared<Engine::Talent>(
                                *talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.orderedTalents[uiData.treeEditorSelectedTalent->index]
                                );
                            uiData.treeEditorSelectedTalentParentsPlaceholder.clear();
                            uiData.treeEditorSelectedTalentChildrenPlaceholder.clear();

                            if (uiData.treeEditorSelectedTalent->parents.size() > 0 || uiData.treeEditorSelectedTalent->children.size() > 0) {
                                std::map<Engine::Talent_s, int> comboIndexTalentMap;
                                //create array of talentIndex: talentName strings to show in combo menu and create map<comboindex, talent> to select correct talent
                                //not sure if it's guaranteed to have talentIndex == comboindex, therefore map
                                int comboIndex = 0;
                                for (auto& talent : talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.orderedTalents) {
                                    comboIndexTalentMap[talent.second] = comboIndex;
                                    comboIndex++;
                                }

                                for (auto& parent : uiData.treeEditorSelectedTalent->parents)
                                    uiData.treeEditorSelectedTalentParentsPlaceholder.push_back(comboIndexTalentMap[parent]);
                                for (auto& child : uiData.treeEditorSelectedTalent->children)
                                    uiData.treeEditorSelectedTalentChildrenPlaceholder.push_back(comboIndexTalentMap[child]);
                            }
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Update talent", ImVec2(ImGui::GetContentRegionAvail().x / 3.0f, 0))) {
                            //Validates talent and inserts it into tree, otherwise show modal popup
                            validateTalentUpdate(uiData, talentTreeCollection, comboIndexTalentMap);
                            loadActiveIcons(uiData, talentTreeCollection, true);
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Delete talent", ImVec2(ImGui::GetContentRegionAvail().x / 2.0f, 0)) 
                            || ((ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)) && ImGui::IsKeyPressed(ImGuiKey_Delete))) {
                            int talentIndex = uiData.treeEditorSelectedTalent->index;
                            //check children for new root talents &
                            //delete from children's parents lists
                            for (auto& child : talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.orderedTalents[talentIndex]->children) {
                                child->parents.erase(
                                    std::remove(
                                        child->parents.begin(),
                                        child->parents.end(),
                                        talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.orderedTalents[talentIndex]),
                                    child->parents.end());
                                if (child->parents.size() == 0)
                                    talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.talentRoots.push_back(child);
                            }
                            //delete from root talent list if it was root
                            if (talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.orderedTalents[talentIndex]->parents.size() == 0) {
                                talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.talentRoots.erase(
                                    std::remove(
                                        talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.talentRoots.begin(),
                                        talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.talentRoots.end(),
                                        talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.orderedTalents[talentIndex]),
                                    talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.talentRoots.end());
                            }
                            //delete from parents' children lists
                            for (auto& parent : talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.orderedTalents[talentIndex]->parents) {
                                parent->children.erase(
                                    std::remove(
                                        parent->children.begin(),
                                        parent->children.end(),
                                        talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.orderedTalents[talentIndex]),
                                    parent->children.end());
                            }
                            //delete from tree.orderedTalents
                            talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.orderedTalents.erase(talentIndex);

                            Engine::updateNodeCountAndMaxTalentPointsAndMaxID(talentTreeCollection.activeTree());

                            talentTreeCollection.activeTree().presetName = "custom";
                            Engine::updateNodeCountAndMaxTalentPointsAndMaxID(talentTreeCollection.activeTree());
                            Engine::updateOrderedTalentList(talentTreeCollection.activeTree());
                            Engine::updateRequirementSeparatorInfo(talentTreeCollection.activeTree());
                            Engine::validateLoadout(talentTreeCollection.activeTree(), true);
                            clearSolvingProcess(uiData, talentTreeCollection);

                            uiData.treeEditorSelectedTalent = nullptr;
                            loadActiveIcons(uiData, talentTreeCollection, true);
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Clear selection", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                            uiData.treeEditorSelectedTalent = nullptr;
                        }
                    }
                    else {
                        ImGui::Text("Select talent to start editing...");
                    }
                }
                else {
                    uiData.treeEditorSelectedTalent = nullptr;
                }
                if (uiData.talentJustSelected) {
                    ImGui::SetNextItemOpen(false, ImGuiCond_Always);
                    uiData.talentJustSelected = false;
                }
                if (ImGui::CollapsingHeader("Misc."))
                {
                    ImGui::Text("Shift all talents");
                    ImGui::Spacing();
                    ImGui::Text("Shift rows by:");
                    ImGui::SliderInt("##treeEditAllRowSlider", &uiData.treeEditorShiftAllRowsBy, -talentTreeCollection.activeTree().maxRowLimit + 1, talentTreeCollection.activeTree().maxRowLimit - 1, "%d", ImGuiSliderFlags_AlwaysClamp);
                    ImGui::Text("Shift columns by:");
                    ImGui::SliderInt("##reeEditAllColumnSlider", &uiData.treeEditorShiftAllColumnsBy, -talentTreeCollection.activeTree().maxColumnLimit + 1, talentTreeCollection.activeTree().maxColumnLimit - 1, "%d", ImGuiSliderFlags_AlwaysClamp);
                    if (ImGui::Button("Shift talents")) {
                        uiData.minRowShift = -talentTreeCollection.activeTree().maxRowLimit + 1;
                        uiData.maxRowShift = talentTreeCollection.activeTree().maxRowLimit - 1;
                        uiData.minColShift = -talentTreeCollection.activeTree().maxColumnLimit + 1;
                        uiData.maxColShift = talentTreeCollection.activeTree().maxColumnLimit - 1;
                        bool shiftExceedsBounds = false;
                        for (auto& talent : talentTreeCollection.activeTree().orderedTalents) {
                            uiData.minRowShift = 1 - talent.second->row > uiData.minRowShift ? 1 - talent.second->row : uiData.minRowShift;
                            uiData.minColShift = 1 - talent.second->column > uiData.minColShift ? 1 - talent.second->column : uiData.minColShift;
                            uiData.maxRowShift = talentTreeCollection.activeTree().maxRowLimit - talent.second->row < uiData.maxRowShift ? talentTreeCollection.activeTree().maxRowLimit - talent.second->row : uiData.maxRowShift;
                            uiData.maxColShift = talentTreeCollection.activeTree().maxColumnLimit - talent.second->column < uiData.maxColShift ? talentTreeCollection.activeTree().maxColumnLimit - talent.second->column : uiData.maxColShift;
                            if (uiData.treeEditorShiftAllRowsBy < uiData.minRowShift || uiData.treeEditorShiftAllRowsBy > uiData.maxRowShift
                                || uiData.treeEditorShiftAllColumnsBy < uiData.minColShift || uiData.treeEditorShiftAllColumnsBy > uiData.maxColShift) {
                                shiftExceedsBounds = true;
                            }
                        }
                        if (shiftExceedsBounds) {
                            ImGui::OpenPopup("Shift value exceeds bounds");
                        }
                        else {
                            talentTreeCollection.activeTree().maxCol = 0;
                            for (auto& talent : talentTreeCollection.activeTree().orderedTalents) {
                                talent.second->row += uiData.treeEditorShiftAllRowsBy;
                                talent.second->column += uiData.treeEditorShiftAllColumnsBy;
                                if (talent.second->column > talentTreeCollection.activeTree().maxCol) {
                                    talentTreeCollection.activeTree().maxCol = talent.second->column;
                                }
                            }
                            uiData.treeEditorShiftAllRowsBy = 0;
                            uiData.treeEditorShiftAllColumnsBy = 0;
                        }

                        talentTreeCollection.activeTree().presetName = "custom";
                        Engine::updateNodeCountAndMaxTalentPointsAndMaxID(talentTreeCollection.activeTree());
                        Engine::updateOrderedTalentList(talentTreeCollection.activeTree());
                        Engine::updateRequirementSeparatorInfo(talentTreeCollection.activeTree());
                        Engine::validateLoadout(talentTreeCollection.activeTree(), true);
                        clearSolvingProcess(uiData, talentTreeCollection);

                        uiData.treeEditorSelectedTalent = nullptr;
                    }
                    ImGui::Separator();
                    ImGui::Text("Place empty nodes");
                    ImGui::Spacing();
                    ImGui::Text("Active talents:");
                    ImGui::SliderInt("##treeEditEmptyActiveSlider", &uiData.treeEditorEmptyActiveNodes, 0, 40, "%d", ImGuiSliderFlags_AlwaysClamp);
                    ImGui::Text("Passive talents:");
                    ImGui::SliderInt("##treeEditEmptyPassiveSlider", &uiData.treeEditorEmptyPassiveNodes, 0, 40, "%d", ImGuiSliderFlags_AlwaysClamp);
                    ImGui::Text("Switch talents:");
                    ImGui::SliderInt("##treeEditEmptySwitchSlider", &uiData.treeEditorEmptySwitchNodes, 0, 40, "%d", ImGuiSliderFlags_AlwaysClamp);
                    if (ImGui::Button("Insert talents")) {
                        if (uiData.treeEditorEmptyActiveNodes + uiData.treeEditorEmptyPassiveNodes
                            + uiData.treeEditorEmptySwitchNodes + talentTreeCollection.activeTree().orderedTalents.size() > talentTreeCollection.activeTree().maxRowLimit * talentTreeCollection.activeTree().maxColumnLimit / 4) {
                            ImGui::OpenPopup("Too many talents inserted");
                        }
                        else {
                            int an = uiData.treeEditorEmptyActiveNodes;
                            int pn = uiData.treeEditorEmptyPassiveNodes;
                            int sn = uiData.treeEditorEmptySwitchNodes;
                            int currentPosX = 1;
                            int currentPosY = 1;
                            int currentTalentIndex = talentTreeCollection.activeTree().maxID;
                            Engine::vec2d<int> occupiedSpots;
                            occupiedSpots.resize(talentTreeCollection.activeTree().maxRowLimit, std::vector<int>(talentTreeCollection.activeTree().maxColumnLimit));
                            for (auto& talent : talentTreeCollection.activeTree().orderedTalents) {
                                occupiedSpots[talent.second->row - 1][talent.second->column - 1] = 1;
                            }
                            while (an + pn + sn > 0) {
                                if (currentPosX == talentTreeCollection.activeTree().maxColumnLimit + 1 && currentPosY == talentTreeCollection.activeTree().maxRowLimit) {
                                    break;
                                }
                                if (occupiedSpots[currentPosY - 1][currentPosX - 1] == 0) {
                                    Engine::Talent_s talent = std::make_shared<Engine::Talent>();
                                    talent->row = currentPosY;
                                    talent->column = currentPosX;
                                    talent->index = currentTalentIndex;
                                    currentTalentIndex++;
                                    if (an > 0) {
                                        an--;
                                        talent->type = Engine::TalentType::ACTIVE;
                                        talent->descriptions.push_back("");
                                    }
                                    else if (pn > 0) {
                                        pn--;
                                        talent->type = Engine::TalentType::PASSIVE;
                                        talent->descriptions.push_back("");
                                    }
                                    else {
                                        sn--;
                                        talent->type = Engine::TalentType::SWITCH;
                                        talent->descriptions.push_back("");
                                        talent->descriptions.push_back("");
                                    }
                                    talentTreeCollection.activeTree().talentRoots.push_back(talent);
                                    occupiedSpots[currentPosY - 1][currentPosX - 1] = 1;
                                }
                                currentPosX += 2;
                                //TTMNOTE: No idea if this breaks for different values of treeEditorMaxColumn/Row
                                if (currentPosX > talentTreeCollection.activeTree().maxColumnLimit && currentPosY < talentTreeCollection.activeTree().maxRowLimit - 1) {
                                    currentPosY += 2;
                                    currentPosX = talentTreeCollection.activeTree().maxColumnLimit / 2 + 1;
                                }
                                else if (currentPosX == talentTreeCollection.activeTree().maxColumnLimit / 2 + 1 && currentPosY + 2 > talentTreeCollection.activeTree().maxRowLimit) {
                                    currentPosY = 1;
                                }
                                else if (currentPosX == talentTreeCollection.activeTree().maxColumnLimit / 2 + 1 && currentPosY + 2 <= talentTreeCollection.activeTree().maxRowLimit) {
                                    currentPosY += 2;
                                    currentPosX = 1;
                                }
                            }
                            Engine::updateNodeCountAndMaxTalentPointsAndMaxID(talentTreeCollection.activeTree());
                            Engine::updateOrderedTalentList(talentTreeCollection.activeTree());
                            Engine::updateRequirementSeparatorInfo(talentTreeCollection.activeTree());

                            talentTreeCollection.activeTree().presetName = "custom";
                            Engine::updateNodeCountAndMaxTalentPointsAndMaxID(talentTreeCollection.activeTree());
                            Engine::updateOrderedTalentList(talentTreeCollection.activeTree());
                            Engine::updateRequirementSeparatorInfo(talentTreeCollection.activeTree());
                            Engine::validateLoadout(talentTreeCollection.activeTree(), true);
                            clearSolvingProcess(uiData, talentTreeCollection);

                            uiData.treeEditorSelectedTalent = nullptr;
                            uiData.treeEditorEmptyActiveNodes = 0;
                            uiData.treeEditorEmptyPassiveNodes = 0;
                            uiData.treeEditorEmptySwitchNodes = 0;

                            loadActiveIcons(uiData, talentTreeCollection, true);
                        }
                    }
                    ImGui::Separator();
                    if (ImGui::Button("Cleanup tree (deletes loadout!)")) {
                        Engine::reindexTree(talentTreeCollection.activeTree());
                        Engine::autoShiftTreeToCorner(talentTreeCollection.activeTree());
                        Engine::autoPointRequirements(talentTreeCollection.activeTree());

                        talentTreeCollection.activeTree().presetName = "custom";
                        Engine::updateNodeCountAndMaxTalentPointsAndMaxID(talentTreeCollection.activeTree());
                        Engine::updateOrderedTalentList(talentTreeCollection.activeTree());
                        Engine::updateRequirementSeparatorInfo(talentTreeCollection.activeTree());
                        clearSolvingProcess(uiData, talentTreeCollection);

                        uiData.treeEditorSelectedTalent = nullptr;

                        loadActiveIcons(uiData, talentTreeCollection, true);
                    }
                    ImGui::Separator();
                    /*
                    ImGui::Text("Warning: Potentially very long runtime/unstable (beta feature)");
                    if (ImGui::Button("Auto position talents in tree")) {
                        Engine::autoPositionTreeNodes(talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree);

                        talentTreeCollection.activeTree().presetName = "custom";
                        Engine::updateNodeCountAndMaxTalentPointsAndMaxID(talentTreeCollection.activeTree());
                        Engine::updateOrderedTalentList(talentTreeCollection.activeTree());
                        Engine::updateRequirementSeparatorInfo(talentTreeCollection.activeTree());
                        Engine::validateLoadout(talentTreeCollection.activeTree(), true);
                        clearSolvingProcess(uiData, talentTreeCollection);

                        uiData.treeEditorSelectedTalent = nullptr;
                    }
                    */
                    if (ImGui::Button("Double talent positions")) {
                        //add talents to vector so that we can sort vector to have bottom right talent first as we begin
                        //to move talents from bottom right to top left so that talent old pos cannot occupy another talent's new pos
                        std::vector<Engine::Talent_s> tempTalents;
                        for (auto& talent : talentTreeCollection.activeTree().orderedTalents) {
                            tempTalents.push_back(talent.second);
                        }
                        std::sort(tempTalents.begin(), tempTalents.end(), [](const Engine::Talent_s& a, const Engine::Talent_s& b) {
                            if (a->row != b->row)
                                return a->row > b->row;
                            else
                                return a->column > b->column;
                            });
                        int maxCol = 0;
                        for (auto& talent : tempTalents) {
                            int newRow = talent->row * 2;
                            newRow = newRow > talentTreeCollection.activeTree().maxRowLimit ? talentTreeCollection.activeTree().maxRowLimit : newRow;
                            int newCol = talent->column * 2;
                            newCol = newCol > talentTreeCollection.activeTree().maxColumnLimit ? talentTreeCollection.activeTree().maxColumnLimit : newCol;
                            bool isOccupied = false;
                            for (auto& occTalent : talentTreeCollection.activeTree().orderedTalents) {
                                if (occTalent.second->row == newRow && occTalent.second->column == newCol) {
                                    isOccupied = true;
                                }
                            }
                            if (!isOccupied) {
                                talent->row = newRow;
                                talent->column = newCol;
                                if (talent->column > maxCol) {
                                    maxCol = talent->column;
                                }
                            }
                        }
                        talentTreeCollection.activeTree().maxCol = maxCol + 1;

                        talentTreeCollection.activeTree().presetName = "custom";
                        Engine::updateNodeCountAndMaxTalentPointsAndMaxID(talentTreeCollection.activeTree());
                        Engine::updateOrderedTalentList(talentTreeCollection.activeTree());
                        Engine::updateRequirementSeparatorInfo(talentTreeCollection.activeTree());
                        //These two shouldn't be necessary but to keep it consistent
                        Engine::validateLoadout(talentTreeCollection.activeTree(), true);
                        clearSolvingProcess(uiData, talentTreeCollection);

                        uiData.treeEditorSelectedTalent = nullptr;
                    }
                    if (ImGui::Button("Sort talent indices (deletes loadout!)")) {
                        Engine::reindexTree(talentTreeCollection.activeTree());

                        talentTreeCollection.activeTree().presetName = "custom";
                        Engine::updateNodeCountAndMaxTalentPointsAndMaxID(talentTreeCollection.activeTree());
                        Engine::updateOrderedTalentList(talentTreeCollection.activeTree());
                        Engine::updateRequirementSeparatorInfo(talentTreeCollection.activeTree());
                        //loadout gets cleared anyway since it will be nonsensical
                        //Engine::validateLoadout(talentTreeCollection.activeTree(), true);
                        clearSolvingProcess(uiData, talentTreeCollection);

                        uiData.treeEditorSelectedTalent = nullptr;

                        loadActiveIcons(uiData, talentTreeCollection, true);
                    }
                    if (ImGui::Button("Auto set point requirements")) {
                        Engine::autoPointRequirements(talentTreeCollection.activeTree());

                        talentTreeCollection.activeTree().presetName = "custom";
                        Engine::updateNodeCountAndMaxTalentPointsAndMaxID(talentTreeCollection.activeTree());
                        Engine::updateOrderedTalentList(talentTreeCollection.activeTree());
                        Engine::updateRequirementSeparatorInfo(talentTreeCollection.activeTree());
                        Engine::validateLoadout(talentTreeCollection.activeTree(), true);
                        clearSolvingProcess(uiData, talentTreeCollection);

                        uiData.treeEditorSelectedTalent = nullptr;
                    }
                    if (ImGui::Button("Auto shift tree to corner")) {
                        Engine::autoShiftTreeToCorner(talentTreeCollection.activeTree());

                        talentTreeCollection.activeTree().presetName = "custom";
                        Engine::updateNodeCountAndMaxTalentPointsAndMaxID(talentTreeCollection.activeTree());
                        Engine::updateOrderedTalentList(talentTreeCollection.activeTree());
                        Engine::updateRequirementSeparatorInfo(talentTreeCollection.activeTree());
                        //These two shouldn't be necessary but to keep it consistent
                        Engine::validateLoadout(talentTreeCollection.activeTree(), true);
                        clearSolvingProcess(uiData, talentTreeCollection);

                        uiData.treeEditorSelectedTalent = nullptr;
                    }
                    if (ImGui::Button("Auto insert icon names")) {
                        std::vector<std::string> iconNames;
                        for (auto it = uiData.iconMap.begin(); it != uiData.iconMap.end(); ++it) {
                            iconNames.push_back(it->first);
                        }
                        Engine::autoInsertIconNames(iconNames, talentTreeCollection.activeTree());

                        talentTreeCollection.activeTree().presetName = "custom";
                        Engine::updateNodeCountAndMaxTalentPointsAndMaxID(talentTreeCollection.activeTree());
                        Engine::updateOrderedTalentList(talentTreeCollection.activeTree());
                        Engine::updateRequirementSeparatorInfo(talentTreeCollection.activeTree());
                        Engine::validateLoadout(talentTreeCollection.activeTree(), true);
                        clearSolvingProcess(uiData, talentTreeCollection);

                        uiData.treeEditorSelectedTalent = nullptr;

                        loadActiveIcons(uiData, talentTreeCollection, true);
                    }
                    if (ImGui::Button("Remove all connections")) {
                        talentTreeCollection.activeTree().talentRoots.clear();
                        for (auto& talent : talentTreeCollection.activeTree().orderedTalents) {
                            talent.second->children.clear();
                            talent.second->parents.clear();
                            talentTreeCollection.activeTree().talentRoots.push_back(talent.second);
                        }

                        talentTreeCollection.activeTree().presetName = "custom";
                        Engine::updateNodeCountAndMaxTalentPointsAndMaxID(talentTreeCollection.activeTree());
                        Engine::updateOrderedTalentList(talentTreeCollection.activeTree());
                        Engine::updateRequirementSeparatorInfo(talentTreeCollection.activeTree());
                        Engine::validateLoadout(talentTreeCollection.activeTree(), true);
                        clearSolvingProcess(uiData, talentTreeCollection);

                        uiData.treeEditorSelectedTalent = nullptr;
                    }
                }
                //Call all the different modal popups that can appear
                ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                if (ImGui::BeginPopupModal("Talent spot occupied##creation", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("Talent spot (%d, %d) is already occupied!", uiData.treeEditorCreationTalent->row, uiData.treeEditorCreationTalent->column);

                    ImGui::SetItemDefaultFocus();
                    if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
                    ImGui::EndPopup();
                }
                if (ImGui::BeginPopupModal("Talent spot occupied##update", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("Talent spot (%d, %d) is already occupied!", uiData.treeEditorSelectedTalent->row, uiData.treeEditorSelectedTalent->column);

                    ImGui::SetItemDefaultFocus();
                    if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
                    ImGui::EndPopup();
                }
                if (ImGui::BeginPopupModal("Cycle detected", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("Talent will introduce a cycle to the tree and invalidate it! Check talent children and parents and remove talents that will create a loop.");

                    ImGui::SetItemDefaultFocus();
                    if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
                    ImGui::EndPopup();
                }
                if (ImGui::BeginPopupModal("Pre filled talent error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("Talent cannot be pre filled if it is not a root talent or if no parent is pre filled!");

                    ImGui::SetItemDefaultFocus();
                    if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
                    ImGui::EndPopup();
                }
                if (ImGui::BeginPopupModal("Shift value exceeds bounds", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("The selected values for row and column shifts exceed the bounds. Please select smaller values.");
                    ImGui::Text("Minimum row shift:");
                    ImGui::SameLine(ImGui::CalcTextSize("@@@@@@@@@@@@@@@@@").x);
                    ImGui::Text("%d", uiData.minRowShift);
                    ImGui::Text("Maximum row shift:");
                    ImGui::SameLine(ImGui::CalcTextSize("@@@@@@@@@@@@@@@@@").x);
                    ImGui::Text("%d", uiData.maxRowShift);
                    ImGui::Text("Minimum column shift:");
                    ImGui::SameLine(ImGui::CalcTextSize("@@@@@@@@@@@@@@@@@").x);
                    ImGui::Text("%d", uiData.minColShift);
                    ImGui::Text("Maximum column shift:");
                    ImGui::SameLine(ImGui::CalcTextSize("@@@@@@@@@@@@@@@@@").x);
                    ImGui::Text("%d", uiData.maxColShift);

                    ImGui::SetItemDefaultFocus();
                    if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
                    ImGui::EndPopup();
                }
                if (ImGui::BeginPopupModal("Too many talents inserted", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("Trying to insert too many talents into the tree. Max number of talents is 400.");

                    ImGui::SetItemDefaultFocus();
                    if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
                    ImGui::EndPopup();
                }
            }break;
            case TreeEditPage::SaveLoadTree: {
                ImGui::Text("Note: Trees also include the loadout and its description!");
                if (ImGui::CollapsingHeader("Load preset"))
                {
                    ImGui::Text("Class:");
                    int oldClass = uiData.treeEditorPresetClassCombo;
                    ImGui::Combo("##treeEditorPresetClassCombo", &uiData.treeEditorPresetClassCombo, Presets::CLASSES, IM_ARRAYSIZE(Presets::CLASSES));
                    if (oldClass != uiData.treeEditorPresetClassCombo) {
                        uiData.treeEditorPresetSpecCombo = 0;
                    }
                    ImGui::Text("Specialization:");
                    int specCount = Presets::RETURN_SPEC_COUNT(uiData.treeEditorPresetClassCombo);
                    if (uiData.treeEditorPresetSpecCombo >= specCount) {
                        uiData.treeEditorPresetSpecCombo = specCount - 1;
                    }
                    ImGui::Combo(
                        "##treeEditorPresetSpecCombo",
                        &uiData.treeEditorPresetSpecCombo,
                        Presets::RETURN_SPECS(uiData.treeEditorPresetClassCombo),
                        specCount
                    );
                    ImGui::Checkbox("Try to keep skillsets", &uiData.treeEditorPresetsKeepLoadout);
                    if (ImGui::Button("Load##treeEditorPresetLoadButton")) {
                        ImGui::OpenPopup("Load preset confirmation");
                    }
                }
                if (ImGui::CollapsingHeader("Custom trees"))
                {
                    ImGui::Text("Custom trees:");
                    if (!uiData.treeEditorIsCustomTreeFileListValid) {
                        updateCustomTreeFileList(uiData);
                        uiData.treeEditorIsCustomTreeFileListValid = true;
                    }
                    if (ImGui::BeginListBox("##treeEditorCustomTreesListbox", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
                    {
                        for (int n = 0; n < uiData.treeEditorCustomTreeFileList.size(); n++)
                        {
                            const bool is_selected = (uiData.treeEditorCustomTreeListCurrent == n);
                            if (ImGui::Selectable(uiData.treeEditorCustomTreeFileList[n].second.c_str(), is_selected))
                                uiData.treeEditorCustomTreeListCurrent = n;

                            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                            if (is_selected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndListBox();
                    }
                    if (ImGui::Button("Save tree##treeEditorCustomTreeSaveButton", ImVec2(ImGui::GetContentRegionAvail().x / 4.0f, 0))) {
                        //check if file list has changed and abort if yes while updating file list
                        bool hasFileListChanged = updateCustomTreeFileList(uiData);
                        if (hasFileListChanged) {
                            //check if file exists and warn (based on tree name, filter the name)
                            ImGui::OpenPopup("File list changed");
                        }
                        else {
                            //create file and string tree rep and safe it
                            if (checkIfTreeFileExists(uiData, talentTreeCollection)) {
                                ImGui::OpenPopup("Overwrite custom tree?");
                            }
                            else {
                                if (!saveTreeToFile(uiData, talentTreeCollection)) {
                                    ImGui::OpenPopup("Tree save error");
                                }
                            }
                            uiData.treeEditorIsCustomTreeFileListValid = false;
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Load tree##treeEditorCustomTreeLoadButton", ImVec2(ImGui::GetContentRegionAvail().x / 3.0f, 0))) {
                        //check if file list has changed and abort if yes while updating file list
                        bool hasFileListChanged = updateCustomTreeFileList(uiData);
                        if (hasFileListChanged) {
                            //check if file exists and warn (based on tree name, filter the name)
                            ImGui::OpenPopup("File list changed");
                        }
                        else {
                            if (uiData.treeEditorCustomTreeListCurrent >= 0) {
                                //Modal popup that tells current tree gets discarded, just like load preset confirmation
                                //load string from file and do a parse tree
                                ImGui::OpenPopup("Load custom tree confirmation");
                                uiData.treeEditorIsCustomTreeFileListValid = false;
                            }
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Delete tree##treeEditorCustomTreeDeleteButton", ImVec2(ImGui::GetContentRegionAvail().x / 2.0f, 0))) {
                        if (uiData.treeEditorCustomTreeFileList.size() > 0) {
                            //check if file list has changed and abort if yes while updating file list
                            bool hasFileListChanged = updateCustomTreeFileList(uiData);
                            if (hasFileListChanged) {
                                //check if file exists and warn (based on tree name, filter the name)
                                ImGui::OpenPopup("File list changed");
                            }
                            else {
                                //Modal popup with confirmation and says that this is only about file
                                //then simply delete file from disk
                                ImGui::OpenPopup("Delete custom tree confirmation");
                                uiData.treeEditorIsCustomTreeFileListValid = false;
                            }
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Refresh list##treeEditorCustomTreeRefreshButton", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                        uiData.treeEditorIsCustomTreeFileListValid = false;
                    }
                }
                if (ImGui::CollapsingHeader("Import/Export"))
                {
                    ImGui::Text("Import talent tree:");
                    ImGui::InputText("##treeEditorImportTalentTreeInput", &uiData.treeEditorImportTreeString);
                    ImGui::SameLine();
                    if (ImGui::Button("Import##treeEditorImportTalentTreeButton")) {
                        if (!Engine::validateAndRepairTreeStringFormat(uiData.treeEditorImportTreeString)) {
                            ImGui::OpenPopup("Invalid import tree string");
                        }
                        else {
                            talentTreeCollection.activeTree() = Engine::parseTree(uiData.treeEditorImportTreeString);
                            Engine::validateLoadout(talentTreeCollection.activeTree(), true);
                            clearSolvingProcess(uiData, talentTreeCollection);

                            uiData.treeEditorSelectedTalent = nullptr;
                            loadActiveIcons(uiData, talentTreeCollection, true);
                            ImGui::OpenPopup("Tree import successful");
                        }
                    }

                    ImGui::Text("Export talent tree:");
                    ImGui::InputText("##treeEditorExportTalentInput", &uiData.treeEditorExportTreeString, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AutoSelectAll);
                    ImGui::SameLine();
                    if (ImGui::Button("Export##treeEditorExportTalentTreeButton")) {
                        uiData.treeEditorExportTreeString = Engine::createTreeStringRepresentation(talentTreeCollection.activeTree());
                    }

                    ImGui::Text("Export to pastebin (1 minute cooldown):");
                    ImGui::InputText("##treeEditorPastebinExportTalentInput", &uiData.treeEditorPastebinExportTreeString, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AutoSelectAll);
                    ImGui::SameLine();
                    double timeSinceLastExport = 0.0;
                    if (ImGui::Button("Export##treeEditorPastebinExportTalentTreeButton")) {
                        auto currentTime = std::chrono::high_resolution_clock::now();
                        timeSinceLastExport = std::chrono::duration<double>(currentTime - uiData.lastPastebinExportTime).count();
                        if (timeSinceLastExport < Presets::PASTEBIN_EXPORT_COOLDOWN) {
                            uiData.pastebinExportCooldownActive = true;
                        }
                        else {
                            uiData.lastPastebinExportTime = currentTime;
                            std::string treeRep = Engine::createTreeStringRepresentation(talentTreeCollection.activeTree());
                            uiData.treeEditorPastebinExportTreeString = exportToPastebin(treeRep, talentTreeCollection.activeTree().name);
                        }
                    }
                    if (uiData.pastebinExportCooldownActive) {
                        auto currentTime = std::chrono::high_resolution_clock::now();
                        timeSinceLastExport = std::chrono::duration<double>(currentTime - uiData.lastPastebinExportTime).count();
                        ImGui::TextUnformattedColored(
                            Presets::GET_TOOLTIP_TALENT_TYPE_COLOR(uiData.style), 
                            ("Please wait " + std::to_string(static_cast<int>(Presets::PASTEBIN_EXPORT_COOLDOWN - timeSinceLastExport)) + " seconds.").c_str()
                        );
                        if (timeSinceLastExport >= Presets::PASTEBIN_EXPORT_COOLDOWN) {
                            uiData.pastebinExportCooldownActive = false;
                        }
                    }

                    ImGui::Text("Export readable talent tree:");
                    ImGui::InputText("##treeEditorReadableExportTalentInput", &uiData.treeEditorReadableExportTreeString, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AutoSelectAll);
                    ImGui::SameLine();
                    if (ImGui::Button("Export##treeEditorReadableExportTalentTreeButton")) {
                        uiData.treeEditorReadableExportTreeString = Engine::createReadableTreeString(talentTreeCollection.activeTree());
                    }

                    ImGui::Text("Screenshot of tree window:");
                    if (ImGui::Button("To clipboard##treeEditorScreenshotExportTalentTreeButton")) {
                        createScreenshotToClipboard(ImGui::FindWindowByName("TreeWindow")->WorkRect);
                    }
                }
                //Call all the different modal popups that can appear
                ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                if (ImGui::BeginPopupModal("Load preset confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("Do you want to load the preset?\nCurrent tree will be overwritten!\n(Skillsets will either be discarded or validated.)");

                    ImGui::SetItemDefaultFocus();
                    if (ImGui::Button("OK", ImVec2(120, 0))) {
                        if (uiData.treeEditorPresetsKeepLoadout) {
                            talentTreeCollection.activeTree() = Engine::restorePreset(talentTreeCollection.activeTree(),
                                Presets::RETURN_PRESET(talentTreeCollection.presets, uiData.treeEditorPresetClassCombo, uiData.treeEditorPresetSpecCombo));
                        }
                        else {
                            talentTreeCollection.activeTree() = Engine::loadTreePreset(Presets::RETURN_PRESET(talentTreeCollection.presets, uiData.treeEditorPresetClassCombo, uiData.treeEditorPresetSpecCombo));
                        }
                        Engine::validateLoadout(talentTreeCollection.activeTree(), true);
                        clearSolvingProcess(uiData, talentTreeCollection);

                        loadActiveIcons(uiData, talentTreeCollection, true);
                        uiData.treeEditorSelectedTalent = nullptr;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
                    ImGui::EndPopup();
                }
                if (ImGui::BeginPopupModal("Load custom tree confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("Do you want to load this custom tree? This will discard the currently loaded tree. Don't forget to save changes!");

                    ImGui::SetItemDefaultFocus();
                    if (ImGui::Button("OK", ImVec2(120, 0))) {
                        if (!loadTreeFromFile(uiData, talentTreeCollection)) {
                            ImGui::CloseCurrentPopup();
                            ImGui::OpenPopup("Tree load error");
                        }
                        else {
                            ImGui::CloseCurrentPopup();
                        }
                        uiData.treeEditorIsCustomTreeFileListValid = false;
                        Engine::validateLoadout(talentTreeCollection.activeTree(), true);
                        clearSolvingProcess(uiData, talentTreeCollection);

                        loadActiveIcons(uiData, talentTreeCollection, true);
                        uiData.treeEditorSelectedTalent = nullptr;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
                    ImGui::EndPopup();
                }
                if (ImGui::BeginPopupModal("Delete custom tree confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("Do you want to delete this custom tree? This will not reset/change the currently loaded tree!");

                    ImGui::SetItemDefaultFocus();
                    if (ImGui::Button("OK", ImVec2(120, 0))) {
                        if (!deleteTreeFromFile(uiData, talentTreeCollection)) {
                            ImGui::CloseCurrentPopup();
                            ImGui::OpenPopup("Tree delete error");
                        }
                        else {
                            if (uiData.treeEditorCustomTreeFileList.size() == 0) {
                                uiData.treeEditorCustomTreeListCurrent = -1;
                            }
                            ImGui::CloseCurrentPopup();
                        }
                        uiData.treeEditorIsCustomTreeFileListValid = false;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
                    ImGui::EndPopup();
                }
                if (ImGui::BeginPopupModal("Tree import successful", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("Tree was imported successfully.");

                    ImGui::SetItemDefaultFocus();
                    if (ImGui::Button("OK", ImVec2(120, 0))) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                if (ImGui::BeginPopupModal("Invalid import tree string", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("Tree import string is invalid!");

                    ImGui::SetItemDefaultFocus();
                    if (ImGui::Button("OK", ImVec2(120, 0))) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                if (ImGui::BeginPopupModal("File list changed", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("File list has changed after last update. Operation aborted and file list updated.\nPlease retry.");

                    ImGui::SetItemDefaultFocus();
                    if (ImGui::Button("OK", ImVec2(120, 0))) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                if (ImGui::BeginPopupModal("Overwrite custom tree?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("A custom tree with the same name already exists! Overwrite?");

                    ImGui::SetItemDefaultFocus();
                    if (ImGui::Button("Overwrite", ImVec2(120, 0))) {
                        if (!saveTreeToFile(uiData, talentTreeCollection)) {
                            ImGui::CloseCurrentPopup();
                            ImGui::OpenPopup("Tree save error");
                        }
                        else {
                            ImGui::CloseCurrentPopup();
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
                    ImGui::EndPopup();
                }
                if (ImGui::BeginPopupModal("Tree save error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("Something went wrong while attempting to save the tree. Maybe directory is corrupted.");

                    ImGui::SetItemDefaultFocus();
                    if (ImGui::Button("OK", ImVec2(120, 0))) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                if (ImGui::BeginPopupModal("Tree load error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("Something went wrong while attempting to load the tree. Maybe directory is corrupted or tree is invalid/has cycles.");

                    ImGui::SetItemDefaultFocus();
                    if (ImGui::Button("OK", ImVec2(120, 0))) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                if (ImGui::BeginPopupModal("Tree delete error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("Something went wrong while attempting to delete the tree. Maybe directory is corrupted.");

                    ImGui::SetItemDefaultFocus();
                    if (ImGui::Button("OK", ImVec2(120, 0))) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            }break;
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

    void validateAndInsertTalent(UIData& uiData, TalentTreeCollection& talentTreeCollection, std::map<int, Engine::Talent_s> comboIndexTalentMap) {
        //check if position is not occupied
        for (auto& talent : talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.orderedTalents) {
            if (uiData.treeEditorCreationTalent->row == talent.second->row && uiData.treeEditorCreationTalent->column == talent.second->column) {
                ImGui::OpenPopup("Talent spot occupied##creation");
                return;
            }
        }
        //Create set of unique comboindices in parents/children
        //check if any index appears in both -> early abort cause cycle!
        std::unordered_set<int> parentComboIndices;
        std::unordered_set<int> childComboIndices;
        for (auto& index : uiData.treeEditorCreationTalentParentsPlaceholder) {
            parentComboIndices.insert(index);
        }
        for (auto& index : uiData.treeEditorCreationTalentChildrenPlaceholder) {
            childComboIndices.insert(index);
            if (parentComboIndices.count(index)) {
                ImGui::OpenPopup("Cycle detected");
                return;
            }
        }
        //clear uidata talent parent/child vector
        uiData.treeEditorCreationTalent->parents.clear();
        uiData.treeEditorCreationTalent->children.clear();
        //push back talent pointers with unique comboindex set and comboindex talent map
        for (auto& index : parentComboIndices) {
            uiData.treeEditorCreationTalent->parents.push_back(comboIndexTalentMap[index]);
        }
        for (auto& index : childComboIndices) {
            uiData.treeEditorCreationTalent->children.push_back(comboIndexTalentMap[index]);
        }

        //check if talent is not erroneously pre filled
        if (uiData.treeEditorCreationTalent->preFilled && uiData.treeEditorCreationTalent->parents.size() > 0) {
            bool hasPreFilledParent = false;
            for (auto& talent : uiData.treeEditorCreationTalent->parents) {
                if (talent->preFilled) {
                    hasPreFilledParent = true;
                }
            }
            if (!hasPreFilledParent) {
                ImGui::OpenPopup("Pre filled talent error");
                return;
            }
        }

        //check for cycles in graph -> if yes then don't insert
        if (Engine::checkIfTalentInsertsCycle(talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree,
            uiData.treeEditorCreationTalent)) {
            ImGui::OpenPopup("Cycle detected");
            return;
        }
        //insert talent in child list of parents and parent list of childs
        for (auto& parent : uiData.treeEditorCreationTalent->parents) {
            parent->children.push_back(uiData.treeEditorCreationTalent);
        }
        for (auto& child : uiData.treeEditorCreationTalent->children) {
            child->parents.push_back(uiData.treeEditorCreationTalent);
        }
        //if talent has a tree root as child then update tree roots (either talent is new root or root vanishes)
        //-> iterate through root and delete every element that has a parent
        //very ugly call, should probably use a reference here or put this in Engine::TalentTrees
        talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.talentRoots.erase(
            std::remove_if(
                talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.talentRoots.begin(),
                talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.talentRoots.end(),
                [](Engine::Talent_s root) { return root->parents.size() > 0; }),
            talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.talentRoots.end());
        //-> if talent has no parent, add to root
        if (uiData.treeEditorCreationTalent->parents.size() == 0)
            talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.talentRoots.push_back(uiData.treeEditorCreationTalent);
        uiData.treeEditorCreationTalent->index = talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.maxID;

        talentTreeCollection.activeTree().presetName = "custom";
        Engine::updateNodeCountAndMaxTalentPointsAndMaxID(talentTreeCollection.activeTree());
        Engine::updateOrderedTalentList(talentTreeCollection.activeTree());
        Engine::updateRequirementSeparatorInfo(talentTreeCollection.activeTree());
        Engine::validateLoadout(talentTreeCollection.activeTree(), true);
        clearSolvingProcess(uiData, talentTreeCollection);

        uiData.treeEditorSelectedTalent = nullptr;

        uiData.treeEditorCreationTalent = std::make_shared<Engine::Talent>();
        uiData.treeEditorCreationTalentParentsPlaceholder.clear();
        uiData.treeEditorCreationTalentChildrenPlaceholder.clear();

        uiData.treeEditorCreationTalentIcons.first = &uiData.defaultIcon;

        uiData.treeEditorCreationTalentIcons.second = &uiData.defaultIcon;
    }

    void validateTalentUpdate(UIData& uiData, TalentTreeCollection& talentTreeCollection, std::map<int, Engine::Talent_s> comboIndexTalentMap) {
        //check if position is not occupied
        for (auto& talent : talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.orderedTalents) {
            if (uiData.treeEditorSelectedTalent->index == talent.second->index)
                continue;
            if (uiData.treeEditorSelectedTalent->row == talent.second->row && uiData.treeEditorSelectedTalent->column == talent.second->column) {
                ImGui::OpenPopup("Talent spot occupied##update");
                return;
            }
        }

        //Create set of unique comboindices in parents/children
        //check if any index appears in both -> early abort cause cycle!
        std::unordered_set<int> parentComboIndices;
        std::unordered_set<int> childComboIndices;
        for (auto& index : uiData.treeEditorSelectedTalentParentsPlaceholder) {
            parentComboIndices.insert(index);
        }
        for (auto& index : uiData.treeEditorSelectedTalentChildrenPlaceholder) {
            childComboIndices.insert(index);
            if (parentComboIndices.count(index)) {
                ImGui::OpenPopup("Cycle detected");
                return;
            }
        }
        //clear uidata talent parent/child vector
        uiData.treeEditorSelectedTalent->parents.clear();
        uiData.treeEditorSelectedTalent->children.clear();
        //push back talent pointers with unique comboindex set and comboindex talent map
        for (auto& index : parentComboIndices) {
            uiData.treeEditorSelectedTalent->parents.push_back(comboIndexTalentMap[index]);
        }
        for (auto& index : childComboIndices) {
            uiData.treeEditorSelectedTalent->children.push_back(comboIndexTalentMap[index]);
        }

        //check if pre filled conditions are met
        //go from pre filled to not pre filled
        if (!uiData.treeEditorSelectedTalent->preFilled && talentTreeCollection.activeTree().orderedTalents[uiData.treeEditorSelectedTalent->index]->preFilled) {
            //check if every pre filled child has another pre filled parent
            for (auto& child : uiData.treeEditorSelectedTalent->children) {
                if (child->preFilled) {
                    bool hasAnotherPreFilledParent = false;
                    for (auto& childParent : child->parents) {
                        if (childParent->index == uiData.treeEditorSelectedTalent->index) {
                            continue;
                        }
                        if (childParent->preFilled) {
                            hasAnotherPreFilledParent = true;
                        }
                    }
                    if (!hasAnotherPreFilledParent) {
                        ImGui::OpenPopup("Pre filled talent error");
                        return;
                    }
                }
            }
        }
        //go to pre filled
        if (uiData.treeEditorSelectedTalent->preFilled && uiData.treeEditorSelectedTalent->parents.size() > 0) {
            bool hasPreFilledParent = false;
            for (auto& talent : uiData.treeEditorSelectedTalent->parents) {
                if (talent->preFilled) {
                    hasPreFilledParent = true;
                }
            }
            if (!hasPreFilledParent) {
                ImGui::OpenPopup("Pre filled talent error");
                return;
            }
        }

        Engine::Talent_s originalTalent = talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.orderedTalents[
            uiData.treeEditorSelectedTalent->index
        ];
        std::vector<Engine::Talent_s> origParents = originalTalent->parents;
        std::vector<Engine::Talent_s> origChildren = originalTalent->children;

        //check cycles (replace child/parent vectors, don't discard old -> check cycle -> if no change back and continue, if yes, then change back and abort)
        originalTalent->parents = uiData.treeEditorSelectedTalent->parents;
        originalTalent->children = uiData.treeEditorSelectedTalent->children;
        if (Engine::checkIfTreeHasCycle(talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree)) {
            //switch back the original parents/children
            originalTalent->parents = origParents;
            originalTalent->children = origChildren;

            ImGui::OpenPopup("Cycle detected");
            return;
        }
        //delete talent from children's parents list and parent's children list
        originalTalent->parents = origParents;
        originalTalent->children = origChildren;
        for (auto& child : origChildren) {
            child->parents.erase(
                std::remove(
                    child->parents.begin(),
                    child->parents.end(),
                    originalTalent),
                child->parents.end());
        }
        //delete from parents' children lists
        for (auto& parent : origParents) {
            parent->children.erase(
                std::remove(
                    parent->children.begin(),
                    parent->children.end(),
                    originalTalent),
                parent->children.end());
        }
        //replace talent in ordered talents list
        talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.orderedTalents[originalTalent->index] = uiData.treeEditorSelectedTalent;
        //insert talent into children's parents list and parent's children list
        for (auto& parent : uiData.treeEditorSelectedTalent->parents) {
            parent->children.push_back(uiData.treeEditorSelectedTalent);
        }
        for (auto& child : uiData.treeEditorSelectedTalent->children) {
            child->parents.push_back(uiData.treeEditorSelectedTalent);
        }
        //update root talents by iterating ordered talents (clear first) and check parents size and if 0 add again
        talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.talentRoots.clear();
        for (auto& talent : talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.orderedTalents) {
            if (talent.second->parents.size() == 0)
                talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.talentRoots.push_back(talent.second);
        }
        //replace selected talent with a copy of the selected talent (like when button is pressed)
        uiData.treeEditorSelectedTalent = std::make_shared<Engine::Talent>(*uiData.treeEditorSelectedTalent);

        Engine::updateNodeCountAndMaxTalentPointsAndMaxID(talentTreeCollection.activeTree());

        talentTreeCollection.activeTree().presetName = "custom";
        Engine::updateNodeCountAndMaxTalentPointsAndMaxID(talentTreeCollection.activeTree());
        Engine::updateOrderedTalentList(talentTreeCollection.activeTree());
        Engine::updateRequirementSeparatorInfo(talentTreeCollection.activeTree());
        Engine::validateLoadout(talentTreeCollection.activeTree(), true);
        clearSolvingProcess(uiData, talentTreeCollection);
    }

    bool updateCustomTreeFileList(UIData& uiData) {
        std::filesystem::path treePath = getCustomTreePath();
        //read all tree names from all files and append to temp vector
        std::vector<std::pair<std::filesystem::path, std::string>> tempFileList;
        for (const auto& entry : std::filesystem::directory_iterator(treePath)) {
            if (std::filesystem::is_regular_file(entry)) {
                try {
                    std::string treeName;
                    if (getTreeNameFromFile(entry, treeName))
                        tempFileList.push_back(std::pair<std::filesystem::path, std::string>(entry, treeName));
                }
                catch (const std::ifstream::failure& e) {
                    //if file is not readable we can just ignore it, user shouldn't mingle in that folder anyway
                    ImGui::LogText(e.what());
                    continue;
                }
            }
        }

        //check if temp vector and uiData vector are different
        bool fileListChanged = false;
        if (tempFileList.size() == uiData.treeEditorCustomTreeFileList.size()) {
            for (int i = 0; i < tempFileList.size(); i++) {
                if (tempFileList[i].first != uiData.treeEditorCustomTreeFileList[i].first
                    || tempFileList[i].second != uiData.treeEditorCustomTreeFileList[i].second) {
                    fileListChanged = true;
                    break;
                }
            }
        }
        else {
            fileListChanged = true;
        }

        //if different, replace uiData vector
        if (fileListChanged) {
            /*uiData.treeEditorCustomTreeFileList.clear();
            for (auto& treeFile : tempFileList) {
                uiData.treeEditorCustomTreeFileList.push_back(treeFile);
            }*/
            uiData.treeEditorCustomTreeFileList = tempFileList;
        }

        return fileListChanged;
    }

    bool checkIfTreeFileExists(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        for (auto& treeFileInfo : uiData.treeEditorCustomTreeFileList) {
            if (treeFileInfo.second == talentTreeCollection.activeTree().name) {
                return true;
            }
        }
        return false;
    }

    bool saveTreeToFile(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        std::filesystem::path treePath = getCustomTreePath();
        try {
            std::filesystem::path treeFile = treePath / (treeNameToFileName(talentTreeCollection.activeTree().name) + ".txt");
            std::ofstream treeFileStream;
            treeFileStream.open(treeFile);
            treeFileStream << Engine::createTreeStringRepresentation(talentTreeCollection.activeTree());
            treeFileStream.close();
            return true;
        }
        catch (const std::ofstream::failure& e) {
            ImGui::LogText(e.what());
            return false;
        }
    }

    bool loadTreeFromFile(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        if (uiData.treeEditorCustomTreeListCurrent < 0 || uiData.treeEditorCustomTreeListCurrent >= uiData.treeEditorCustomTreeFileList.size()) {
            return false;
        }
        //tree file path should be valid path since the tree file list checks for that
        //only way to invalid it is if user manually deletes/changes files since last validation
        try {
            std::filesystem::path treeFile = uiData.treeEditorCustomTreeFileList[uiData.treeEditorCustomTreeListCurrent].first;
            std::string treeRepresentation;
            std::ifstream treeFileStream;
            treeFileStream.open(treeFile);
            if (treeFileStream.is_open())
            {
                if (!treeFileStream.eof())
                {
                    std::getline(treeFileStream, treeRepresentation);
                    treeFileStream.close();
                    if (Engine::validateAndRepairTreeStringFormat(treeRepresentation)) {
                        talentTreeCollection.activeTree() = Engine::parseTree(treeRepresentation);
                        return true;
                    }
                    else {
                        return false;
                    }
                }
            }
        }
        catch (const std::ifstream::failure& e) {
            ImGui::LogText(e.what());
            return false;
        }
        return false;
    }

    bool deleteTreeFromFile(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        if (uiData.treeEditorCustomTreeListCurrent < 0 || uiData.treeEditorCustomTreeListCurrent >= uiData.treeEditorCustomTreeFileList.size()) {
            return false;
        }
        try {
            std::filesystem::path treeFile = uiData.treeEditorCustomTreeFileList[uiData.treeEditorCustomTreeListCurrent].first;
            std::filesystem::remove(treeFile);
            return true;
        }
        catch (const std::filesystem::filesystem_error &e) {
            ImGui::LogText(e.what());
            return false;
        }
    }

    std::string treeNameToFileName(std::string treeName) {
        treeName.erase(std::remove_if(treeName.begin(), treeName.end(), [](auto const& c) -> bool { return !std::isalnum(c); }), treeName.end());
        return treeName;
    }

    bool getTreeNameFromFile(std::filesystem::path entry, std::string& treeName) {
        //We checked if entry leads to valid file, so open should not crash
        bool readSuccessful = false;
        std::string line;
        std::ifstream treeFile;
        treeFile.open(entry);

        if (treeFile.is_open())
        {
            if (!treeFile.eof())
            {
                std::getline(treeFile, line);
                std::vector<std::string> treeComponents = Engine::splitString(line, ";");
                if (treeComponents.size() > 0) {
                    std::vector<std::string> treeInfo = Engine::splitString(treeComponents[0], ":");
                    if (treeInfo.size() == 8) {
                        treeName = treeInfo[3];
                        readSuccessful = true;
                    }
                }
            }
            treeFile.close();
        }

        return readSuccessful;
    }

    void placeTreeEditorTreeElements(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        Engine::TalentTree tree = talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree;
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
            if (uiData.enableGlow && !searchActive && uiData.treeEditorSelectedTalent && uiData.treeEditorSelectedTalent->index == talent.second->index) {
                ImGui::Image(uiData.redIconGlow.texture, ImVec2(uiData.treeEditorZoomFactor * uiData.redIconGlow.width, uiData.treeEditorZoomFactor * uiData.redIconGlow.height));
            }
            else if (searchActive && std::find(uiData.searchedTalents.begin(), uiData.searchedTalents.end(), talent.second) != uiData.searchedTalents.end()) {
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

            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
            ImGui::PushID(std::to_string(talent.second->index).c_str());
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
                //Quick parent/child connection edit
                if (ImGui::IsKeyDown(ImGuiKey_LeftAlt) && uiData.treeEditorSelectedTalent != nullptr) {
                    //make pressed button the parent
                    Engine::Talent_s parentTalent;
                    Engine::Talent_s childTalent;
                    if (ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
                        parentTalent = talent.second;
                        childTalent = talentTreeCollection.activeTree().orderedTalents[uiData.treeEditorSelectedTalent->index];
                    }
                    //make pressed button the child
                    else {
                        childTalent = talent.second;
                        parentTalent = talentTreeCollection.activeTree().orderedTalents[uiData.treeEditorSelectedTalent->index];
                    }
                    std::vector<Engine::Talent_s>::iterator alreadyLinkedChild = std::find(parentTalent->children.begin(), parentTalent->children.end(), childTalent);
                    if (alreadyLinkedChild == parentTalent->children.end()) {
                        parentTalent->children.push_back(childTalent);
                        childTalent->parents.push_back(parentTalent);
                        if (Engine::checkIfTreeHasCycle(talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree)) {
                            parentTalent->children.pop_back();
                            childTalent->parents.pop_back();

                            ImGui::OpenPopup("Cycle detected");
                        }
                    }
                    else {
                        std::vector<Engine::Talent_s>::iterator alreadyLinkedParent = std::find(childTalent->parents.begin(), childTalent->parents.end(), parentTalent);
                        childTalent->parents.erase(alreadyLinkedParent);
                        parentTalent->children.erase(alreadyLinkedChild);
                    }

                    talentTreeCollection.activeTree().presetName = "custom";
                    Engine::updateNodeCountAndMaxTalentPointsAndMaxID(talentTreeCollection.activeTree());
                    Engine::updateOrderedTalentList(talentTreeCollection.activeTree());
                    Engine::updateRequirementSeparatorInfo(talentTreeCollection.activeTree());
                    Engine::validateLoadout(talentTreeCollection.activeTree(), true);
                    clearSolvingProcess(uiData, talentTreeCollection);
                }
                //regular talent selection
                else {
                    selectTalent(uiData, talentTreeCollection, talent);
                }
            }
            ImGui::PopID();
            ImGui::PopStyleColor(5);
            Presets::POP_FONT();
            if (ImGui::IsItemActive())
            {
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    uiData.treeEditorDragTalentStartRow = talent.second->row;
                    uiData.treeEditorDragTalentStartColumn = talent.second->column;
                    uiData.treeEditorTempReplacedTalents.clear();
                }
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                    selectTalent(uiData, talentTreeCollection, talent);
                    ImVec2 mouseClickedPos = *ImGui::GetIO().MouseClickedPos;
                    float mouseTotalDeltaX = ImGui::GetIO().MousePos.x - mouseClickedPos.x;
                    float mouseTotalDeltaY = ImGui::GetIO().MousePos.y - mouseClickedPos.y;
                    repositionTalent(uiData, talentTreeCollection, talent, mouseClickedPos, ImVec2(mouseTotalDeltaX, mouseTotalDeltaY), ImVec2(posX, posY));
                }
            }
            else {
                AttachTalentEditTooltip(uiData, talent.second);
            }
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

            drawTreeEditorShapeAroundTalent(
                talent.second,
                drawList,
                imStyle.Colors,
                ImVec2(posX, posY),
                talentSize,
                ImGui::GetWindowPos(),
                ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY()),
                uiData,
                talentTreeCollection,
                uiData.treeEditorSelectedTalent != nullptr && uiData.treeEditorSelectedTalent->index == talent.second->index,
                searchActive,
                talentIsSearchedFor);
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
                AddWrappedText(infoText, textBoxPos, 5.0f, ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 2.0f*talentSize, 2.0f*talentSize, ImGui::GetWindowDrawList());
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

    void selectTalent(UIData& uiData, TalentTreeCollection& talentTreeCollection, std::pair<int, Engine::Talent_s> talent) {
        uiData.talentJustSelected = true;
        uiData.treeEditorSelectedTalent = std::make_shared<Engine::Talent>(*talent.second);
        uiData.treeEditPage = TreeEditPage::TreeEdit;
        uiData.treeEditorSelectedTalentParentsPlaceholder.clear();
        uiData.treeEditorSelectedTalentChildrenPlaceholder.clear();

        if (uiData.treeEditorSelectedTalent->parents.size() > 0 || uiData.treeEditorSelectedTalent->children.size() > 0) {
            std::map<Engine::Talent_s, int> comboIndexTalentMap;
            //create array of talentIndex: talentName strings to show in combo menu and create map<comboindex, talent> to select correct talent
            //not sure if it's guaranteed to have talentIndex == comboindex, therefore map
            int comboIndex = 0;
            for (auto& talent : talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.orderedTalents) {
                comboIndexTalentMap[talent.second] = comboIndex;
                comboIndex++;
            }

            for (auto& parent : uiData.treeEditorSelectedTalent->parents)
                uiData.treeEditorSelectedTalentParentsPlaceholder.push_back(comboIndexTalentMap[parent]);
            for (auto& child : uiData.treeEditorSelectedTalent->children)
                uiData.treeEditorSelectedTalentChildrenPlaceholder.push_back(comboIndexTalentMap[child]);
        }

        uiData.treeEditorSelectedTalentIcons.first = &uiData.iconMap[talent.second->iconName.first].first;

        uiData.treeEditorSelectedTalentIcons.second = &uiData.iconMap[talent.second->iconName.second].first;
    }

    void repositionTalent(
        UIData& uiData,
        TalentTreeCollection& talentTreeCollection,
        std::pair<int, Engine::Talent_s> dragTalent,
        ImVec2 mouseClickedPos,
        ImVec2 deltaMouseTot,
        ImVec2 buttonPos)
    {
        float offsetX = mouseClickedPos.x - buttonPos.x;
        float offsetY = mouseClickedPos.y - buttonPos.y;
        float gridUnit = static_cast<float>(2 * uiData.treeEditorBaseTalentHalfSpacing);
        float deltaGridX = deltaMouseTot.x / gridUnit / uiData.treeEditorZoomFactor;
        float deltaGridY = deltaMouseTot.y / gridUnit / uiData.treeEditorZoomFactor;
        int resDeltaGridX = deltaGridX >= 0 ? static_cast<int>(deltaGridX + 0.5) : static_cast<int>(deltaGridX - 0.5);
        int resDeltaGridY = deltaGridY >= 0 ? static_cast<int>(deltaGridY + 0.5) : static_cast<int>(deltaGridY - 0.5);
        if (uiData.treeEditorDragTalentStartRow + resDeltaGridY < 1) {
            resDeltaGridY = -uiData.treeEditorDragTalentStartRow + 1;
        }
        if (uiData.treeEditorDragTalentStartRow + resDeltaGridY > talentTreeCollection.activeTree().maxRowLimit) {
            resDeltaGridY = -uiData.treeEditorDragTalentStartRow + talentTreeCollection.activeTree().maxRowLimit;
        }
        if (uiData.treeEditorDragTalentStartColumn + resDeltaGridX < 1) {
            resDeltaGridX = -uiData.treeEditorDragTalentStartColumn + 1;
        }
        if (uiData.treeEditorDragTalentStartColumn + resDeltaGridX > talentTreeCollection.activeTree().maxColumnLimit) {
            resDeltaGridX = -uiData.treeEditorDragTalentStartColumn + talentTreeCollection.activeTree().maxColumnLimit;
        }

        for (auto& talent : talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.orderedTalents) {
            if (dragTalent.second->index != talent.second->index
                && uiData.treeEditorDragTalentStartRow + resDeltaGridY == talent.second->row
                && uiData.treeEditorDragTalentStartColumn + resDeltaGridX == talent.second->column) {
                bool alreadyInserted = false;
                for (int i = 0; i < uiData.treeEditorTempReplacedTalents.size(); i++) {
                    if (std::get<3>(uiData.treeEditorTempReplacedTalents[i]) == talent.second->index) {
                        alreadyInserted = true;
                    }
                }
                if (!alreadyInserted) {
                    uiData.treeEditorTempReplacedTalents.push_back(
                        std::tuple<Engine::Talent_s, int, int, int>(talent.second, talent.second->row, talent.second->column, talent.second->index)
                    );
                }
                talent.second->row = dragTalent.second->row;
                talent.second->column = dragTalent.second->column;

                break;
            }
        }
        dragTalent.second->row = uiData.treeEditorDragTalentStartRow + resDeltaGridY;
        dragTalent.second->column = uiData.treeEditorDragTalentStartColumn + resDeltaGridX;

        if (resDeltaGridX != 0 || resDeltaGridY != 0) {
            talentTreeCollection.activeTree().presetName = "custom";
            Engine::updateNodeCountAndMaxTalentPointsAndMaxID(talentTreeCollection.activeTree());
            Engine::updateOrderedTalentList(talentTreeCollection.activeTree());
            Engine::updateRequirementSeparatorInfo(talentTreeCollection.activeTree());
            Engine::validateLoadout(talentTreeCollection.activeTree(), true);
            clearSolvingProcess(uiData, talentTreeCollection);
        }
        int restoredTalentPositions;
        do {
            restoredTalentPositions = 0;
            std::vector<std::tuple<Engine::Talent_s, int, int, int> >::iterator it = uiData.treeEditorTempReplacedTalents.begin();

            while (it != uiData.treeEditorTempReplacedTalents.end()) {
                bool spotFree = true;
                for (auto& talent : talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.orderedTalents) {
                    if (talent.second->row == std::get<1>(*it) && talent.second->column == std::get<2>(*it)) {
                        spotFree = false;
                    }
                }

                if (spotFree) {
                    restoredTalentPositions++;
                    std::get<0>(*it)->row = std::get<1>(*it);
                    std::get<0>(*it)->column = std::get<2>(*it);
                    it = uiData.treeEditorTempReplacedTalents.erase(it);
                }
                else {
                    ++it;
                }
            }
        } while (restoredTalentPositions > 0);
    }

    //I have no idea what I'm doing
    void createScreenshotToClipboard(ImRect cropRect) {

        HWND activeWindow = GetActiveWindow();

        RECT ttmWindowRect;
        GetClientRect(activeWindow, &ttmWindowRect);

        // copy screen to bitmap
        HDC     hScreen = GetDC(NULL);
        HDC     hDC = CreateCompatibleDC(hScreen);
        HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, ttmWindowRect.right - ttmWindowRect.left, ttmWindowRect.bottom - ttmWindowRect.top);
        HGDIOBJ old_obj = SelectObject(hDC, hBitmap);
        PrintWindow(activeWindow, hDC, PW_CLIENTONLY);

        // Get the BITMAP from the HBITMAP
        BITMAP bmpScreen;
        GetObject(hBitmap, sizeof(BITMAP), &bmpScreen);

        BITMAPINFOHEADER   bi;

        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = bmpScreen.bmWidth;
        bi.biHeight = bmpScreen.bmHeight;
        bi.biPlanes = 1;
        bi.biBitCount = 32;
        bi.biCompression = BI_RGB;
        bi.biSizeImage = 0;
        bi.biXPelsPerMeter = 0;
        bi.biYPelsPerMeter = 0;
        bi.biClrUsed = 0;
        bi.biClrImportant = 0;

        DWORD dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;

        // Starting with 32-bit Windows, GlobalAlloc and LocalAlloc are implemented as wrapper functions that 
        // call HeapAlloc using a handle to the process's default heap. Therefore, GlobalAlloc and LocalAlloc 
        // have greater overhead than HeapAlloc.
        HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);
        if (hDIB == 0) {
            return;
        }
        char* lpbitmap = (char*)GlobalLock(hDIB);

        // Gets the "bits" from the bitmap and copies them into a buffer 
        // which is pointed to by lpbitmap.
        GetDIBits(hDC, hBitmap, 0,
            (UINT)bmpScreen.bmHeight,
            lpbitmap,
            (BITMAPINFO*)&bi, DIB_RGB_COLORS);

        
        int height = static_cast<int>(cropRect.GetHeight());
        int width = static_cast<int>(cropRect.GetWidth());
        int x0 = static_cast<int>(cropRect.Min.x);
        int y0 = bi.biHeight - height - static_cast<int>(cropRect.Min.y);
        char* cropBitmap = (char*)malloc(width * height * 4);
        if (cropBitmap == NULL) {
            return;
        }
        for (int y = y0; y < height + y0; y++) {
            for (int x4 = 4 * x0; x4 < 4 * width + 4 * x0; x4++) {
                *(cropBitmap + (y - y0) * 4 * width + (x4 - 4 * x0)) = *(lpbitmap + (height + 2 * y0 - 1 - y) * 4 * bi.biWidth + x4);
            }
        }
        

        HBITMAP newHBitmap = CreateBitmap(width, height, 1, 32, cropBitmap);

        // save bitmap to clipboard
        OpenClipboard(NULL);
        EmptyClipboard();
        SetClipboardData(CF_BITMAP, newHBitmap);
        CloseClipboard();

        // clean up
        SelectObject(hDC, old_obj);
        DeleteDC(hDC);
        ReleaseDC(NULL, hScreen);
        DeleteObject(hBitmap);

        //Unlock and Free the DIB from the heap
        GlobalUnlock(hDIB);
        GlobalFree(hDIB);
    }
}