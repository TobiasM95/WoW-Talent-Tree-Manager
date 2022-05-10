#include "TalentTreeEditorWindow.h"

#include <windows.h>
#include <shlobj.h>

#include <fstream>
#include <string>
#include <algorithm>

#include "TTMEnginePresets.h"

namespace TTM {
    //Talent tooltip for the tree editor view
    static void AttachTalentEditTooltip(Engine::Talent talent)
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
                ImGui::Text(("Max points: " + std::to_string(talent.maxPoints) + ", points required: " + std::to_string(talent.pointsRequired)).c_str());
                ImGui::Spacing();
                ImGui::Spacing();

                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 15.0f);
                for (int i = 0; i < talent.maxPoints - 1; i++) {
                    ImGui::TextColored(ImVec4(0.533f, 0.533f, 1.0f, 1.0f), talent.descriptions[i].c_str());
                    ImGui::Separator();
                }
                ImGui::TextColored(ImVec4(0.533f, 0.533f, 1.0f, 1.0f), talent.descriptions[talent.maxPoints - 1].c_str());
                ImGui::PopTextWrapPos();

                ImGui::EndTooltip();
            }
            else {
                ImGui::BeginTooltip();
                ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
                ImGui::Text(talent.name.c_str());
                ImGui::PopFont();
                //ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(idLabel.c_str()).x);
                ImGui::Text(idLabel.c_str());
                ImGui::TextColored(ImVec4(0.92f, 0.24f, 0.24f, 1.0f), "(switch)");
                ImGui::Text(("Max points: 1, points required: " + std::to_string(talent.pointsRequired)).c_str());
                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.533f, 0.533f, 1.0f, 1.0f), talent.descriptions[0].c_str());
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
                ImGui::Text(talent.nameSwitch.c_str());
                ImGui::PopFont();
                //ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(idLabel.c_str()).x);
                ImGui::Text(idLabel.c_str());
                ImGui::TextColored(ImVec4(0.92f, 0.24f, 0.24f, 1.0f), "(switch)");
                ImGui::Text(("Max points: 1, points required: " + std::to_string(talent.pointsRequired)).c_str());
                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.533f, 0.533f, 1.0f, 1.0f), talent.descriptions[1].c_str());

                ImGui::EndTooltip();
            }
        }
    }

    void RenderTalentTreeEditorWindow(UIData& uiData, TalentTreeCollection& talentTreeCollection) {

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

            placeTreeEditorTreeElements(uiData, talentTreeCollection);

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
                uiData.treeEditPage = TreeEditPage::SaveLoadTree;
            }
            ImGui::Spacing();
            switch (uiData.treeEditPage) {
            case TreeEditPage::TreeInformation: {
                ImGui::Text("Tree name:");
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputText("##TreeNameInputText", &talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.name);

                ImGui::Text("Preset: ");
                ImGui::SameLine(ImGui::GetContentRegionAvail().x / 3.f);
                ImGui::Text(talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.presetName.c_str());

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
                if (ImGui::CollapsingHeader("Create Node"))
                {
                    ImGui::Text("Name:");
                    ImGui::InputText("##talentCreationNameInput", &uiData.treeEditorCreationTalent->name, ImGuiInputTextFlags_AutoSelectAll);

                    if (uiData.treeEditorCreationTalent->type != Engine::TalentType::SWITCH) ImGui::BeginDisabled();
                    ImGui::Text("Name (switch):");
                    ImGui::InputText("##talentCreationNameSwitchInput", &uiData.treeEditorCreationTalent->nameSwitch, ImGuiInputTextFlags_AutoSelectAll);
                    if (uiData.treeEditorCreationTalent->type != Engine::TalentType::SWITCH) ImGui::EndDisabled();

                    ImGui::Text("Talent type:");
                    int currentType = static_cast<int>(uiData.treeEditorCreationTalent->type);
                    ImGui::Combo("##talentCreationTypeCombo", &currentType, "Active\0Passive\0Switch");
                    uiData.treeEditorCreationTalent->type = static_cast<Engine::TalentType>(currentType);

                    if (uiData.treeEditorCreationTalent->type == Engine::TalentType::SWITCH || uiData.treeEditorCreationTalent->type == Engine::TalentType::ACTIVE) ImGui::BeginDisabled();
                    ImGui::Text("Maximum talent points:");
                    ImGui::SliderInt("##talentCreationMaxPointsSlider", &uiData.treeEditorCreationTalent->maxPoints, 1, 10, "%d", ImGuiSliderFlags_NoInput);
                    if (uiData.treeEditorCreationTalent->type == Engine::TalentType::SWITCH || uiData.treeEditorCreationTalent->type == Engine::TalentType::ACTIVE) {
                        ImGui::EndDisabled();
                        uiData.treeEditorCreationTalent->maxPoints = 1;
                    }


                    ImGui::Text("Points required:");
                    ImGui::SliderInt("##talentCreationPtsReqSlider", &uiData.treeEditorCreationTalent->pointsRequired, 0, 50, "%d", ImGuiSliderFlags_NoInput);

                    ImGui::Text("Descriptions:");
                    if (uiData.treeEditorCreationTalent->descriptions.size() != uiData.treeEditorCreationTalent->maxPoints + (uiData.treeEditorCreationTalent->type == Engine::TalentType::SWITCH)) {
                        uiData.treeEditorCreationTalent->descriptions.resize(uiData.treeEditorCreationTalent->maxPoints + (uiData.treeEditorCreationTalent->type == Engine::TalentType::SWITCH));
                    }
                    for (int i = 0; i < uiData.treeEditorCreationTalent->descriptions.size(); i++) {
                        ImGui::InputTextMultiline(("##talentCreationDescriptionInput" + std::to_string(i)).c_str(), &uiData.treeEditorCreationTalent->descriptions[i],
                            ImVec2(0, 5 * ImGui::CalcTextSize("@").y));
                    }

                    ImGui::Text("Talent row:");
                    ImGui::SliderInt("##talentCreationRowSlider", &uiData.treeEditorCreationTalent->row, 0, 20, "%d", ImGuiSliderFlags_NoInput);

                    ImGui::Text("Talent column:");
                    ImGui::SliderInt("##talentCreationColumnSlider", &uiData.treeEditorCreationTalent->column, 0, 20, "%d", ImGuiSliderFlags_NoInput);

                    std::vector<std::string> talentComboList;
                    std::map<int, std::shared_ptr<Engine::Talent>> comboIndexTalentMap;
                    struct Funcs { static bool ItemGetter(void* data, int n, const char** out_str) { *out_str = ((*((std::vector<std::string>*)data))[n].c_str()); return true; } };
                    if (uiData.treeEditorCreationTalentParentsPlaceholder.size() > 0 || uiData.treeEditorCreationTalentChildrenPlaceholder.size() > 0) {
                        //create array of talentIndex: talentName strings to show in combo menu and create map<comboindex, talent> to select correct talent
                        //not sure if it's guaranteed to have talentIndex == comboindex, therefore map
                        int comboIndex = 0;
                        for (auto& talent : talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.orderedTalents) {
                            comboIndexTalentMap[comboIndex] = talent.second;
                            talentComboList.push_back(std::to_string(talent.first) + ": " + talent.second->name);
                            comboIndex++;
                        }
                    }
                    ImGui::Text("Parents:");
                    int parentCount = static_cast<int>(uiData.treeEditorCreationTalentParentsPlaceholder.size());
                    ImGui::SliderInt("##talentCreationParentCountSlider", &parentCount, 0, talentTreeCollection.activeTree().nodeCount, "%d", ImGuiSliderFlags_NoInput);
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
                    ImGui::SliderInt("##talentCreationChildrenCountSlider", &childrenCount, 0, talentTreeCollection.activeTree().nodeCount, "%d", ImGuiSliderFlags_NoInput);
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

                    if (ImGui::Button("Create talent")) {
                        //Validates talent and inserts it into tree, otherwise show modal popup
                        validateAndInsertTalent(uiData, talentTreeCollection, comboIndexTalentMap);
                    }
                }
                if (uiData.talentJustSelected) {
                    ImGui::SetNextItemOpen(true, ImGuiCond_Always);
                }
                if (ImGui::CollapsingHeader("Edit/Delete Node"))
                {
                    if (uiData.treeEditorSelectedTalent != nullptr) {

                        ImGui::Text("Name:");
                        ImGui::InputText("##talentEditNameInput", &uiData.treeEditorSelectedTalent->name, ImGuiInputTextFlags_AutoSelectAll);

                        if (uiData.treeEditorSelectedTalent->type != Engine::TalentType::SWITCH) ImGui::BeginDisabled();
                        ImGui::Text("Name (switch):");
                        ImGui::InputText("##talentEditNameSwitchInput", &uiData.treeEditorSelectedTalent->nameSwitch, ImGuiInputTextFlags_AutoSelectAll);
                        if (uiData.treeEditorSelectedTalent->type != Engine::TalentType::SWITCH) ImGui::EndDisabled();

                        ImGui::Text("Talent type:");
                        int currentType = static_cast<int>(uiData.treeEditorSelectedTalent->type);
                        ImGui::Combo("##talentEditTypeCombo", &currentType, "Active\0Passive\0Switch");
                        uiData.treeEditorSelectedTalent->type = static_cast<Engine::TalentType>(currentType);

                        if (uiData.treeEditorSelectedTalent->type == Engine::TalentType::SWITCH || uiData.treeEditorSelectedTalent->type == Engine::TalentType::ACTIVE) ImGui::BeginDisabled();
                        ImGui::Text("Maximum talent points:");
                        ImGui::SliderInt("##talentEditMaxPointsSlider", &uiData.treeEditorSelectedTalent->maxPoints, 1, 10, "%d", ImGuiSliderFlags_NoInput);
                        if (uiData.treeEditorSelectedTalent->type == Engine::TalentType::SWITCH || uiData.treeEditorSelectedTalent->type == Engine::TalentType::ACTIVE) {
                            ImGui::EndDisabled();
                            uiData.treeEditorSelectedTalent->maxPoints = 1;
                        }


                        ImGui::Text("Points required:");
                        ImGui::SliderInt("##talentEditPtsReqSlider", &uiData.treeEditorSelectedTalent->pointsRequired, 0, 50, "%d", ImGuiSliderFlags_NoInput);

                        ImGui::Text("Descriptions:");
                        if (uiData.treeEditorSelectedTalent->descriptions.size() != uiData.treeEditorSelectedTalent->maxPoints + (uiData.treeEditorSelectedTalent->type == Engine::TalentType::SWITCH)) {
                            uiData.treeEditorSelectedTalent->descriptions.resize(uiData.treeEditorSelectedTalent->maxPoints + (uiData.treeEditorSelectedTalent->type == Engine::TalentType::SWITCH));
                        }
                        for (int i = 0; i < uiData.treeEditorSelectedTalent->descriptions.size(); i++) {
                            ImGui::InputTextMultiline(("##talentEditDescriptionInput" + std::to_string(i)).c_str(), &uiData.treeEditorSelectedTalent->descriptions[i],
                                ImVec2(0, 5 * ImGui::CalcTextSize("@").y));
                        }

                        ImGui::Text("Talent row:");
                        ImGui::SliderInt("##talentEditRowSlider", &uiData.treeEditorSelectedTalent->row, 0, 20, "%d", ImGuiSliderFlags_NoInput);

                        ImGui::Text("Talent column:");
                        ImGui::SliderInt("##talentEditColumnSlider", &uiData.treeEditorSelectedTalent->column, 0, 20, "%d", ImGuiSliderFlags_NoInput);

                        std::vector<std::string> talentComboList;
                        std::map<int, std::shared_ptr<Engine::Talent>> comboIndexTalentMap;
                        struct Funcs { static bool ItemGetter(void* data, int n, const char** out_str) { *out_str = ((*((std::vector<std::string>*)data))[n].c_str()); return true; } };
                        if (uiData.treeEditorSelectedTalentParentsPlaceholder.size() > 0 || uiData.treeEditorSelectedTalentChildrenPlaceholder.size() > 0) {
                            //create array of talentIndex: talentName strings to show in combo menu and create map<comboindex, talent> to select correct talent
                            //not sure if it's guaranteed to have talentIndex == comboindex, therefore map
                            int comboIndex = 0;
                            for (auto& talent : talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.orderedTalents) {
                                comboIndexTalentMap[comboIndex] = talent.second;
                                talentComboList.push_back(std::to_string(talent.first) + ": " + talent.second->name);
                                comboIndex++;
                            }
                        }
                        ImGui::Text("Parents:");
                        int parentCount = static_cast<int>(uiData.treeEditorSelectedTalentParentsPlaceholder.size());
                        ImGui::SliderInt("##talentEditParentCountSlider", &parentCount, 0, talentTreeCollection.activeTree().nodeCount - 1, "%d", ImGuiSliderFlags_NoInput);
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
                        ImGui::SliderInt("##talentEditChildrenCountSlider", &childrenCount, 0, talentTreeCollection.activeTree().nodeCount - 1, "%d", ImGuiSliderFlags_NoInput);
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
                                std::map<std::shared_ptr<Engine::Talent>, int> comboIndexTalentMap;
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
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Delete talent", ImVec2(ImGui::GetContentRegionAvail().x / 2.0f, 0))) {
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
                            Engine::validateLoadout(talentTreeCollection.activeTree(), true);

                            uiData.treeEditorSelectedTalent = nullptr;
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
                    if (ImGui::Button("Auto position talents in tree")) {
                        Engine::autoPositionTreeNodes(talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree);

                        talentTreeCollection.activeTree().presetName = "custom";
                        Engine::validateLoadout(talentTreeCollection.activeTree(), true);
                    }
                }
                //Call all the different modal popups that can appear
                ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                if (ImGui::BeginPopupModal("Talent spot occupied", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("Talent spot (%d, %d) is already occupied!", uiData.treeEditorCreationTalent->row, uiData.treeEditorCreationTalent->column);

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
            }break;
            case TreeEditPage::SaveLoadTree: {
                ImGui::Text("Note: Trees also include the loadout and its description!");
                if (ImGui::CollapsingHeader("Load preset"))
                {
                    ImGui::Text("Class:");
                    ImGui::Combo("##treeEditorPresetClassCombo", &uiData.treeEditorPresetClassCombo, Presets::CLASSES, IM_ARRAYSIZE(Presets::CLASSES));
                    ImGui::Text("Specialization:");
                    ImGui::Combo(
                        "##treeEditorPresetSpecCombo",
                        &uiData.treeEditorPresetSpecCombo,
                        Presets::RETURN_SPECS(uiData.treeEditorPresetClassCombo),
                        IM_ARRAYSIZE(Presets::RETURN_SPECS(uiData.treeEditorPresetClassCombo))
                    );
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
                            //Modal popup that tells current tree gets discarded, just like load preset confirmation
                            //load string from file and do a parse tree
                            ImGui::OpenPopup("Load custom tree confirmation");
                            uiData.treeEditorIsCustomTreeFileListValid = false;
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
                        try {
                            talentTreeCollection.activeTree() = Engine::parseTree(uiData.treeEditorImportTreeString);
                        }
                        catch (const std::exception& e) {
                            //TTMTODO: Implement proper validation method for a tree rep
                            ImGui::OpenPopup("Invalid import tree string");
                        }
                    }

                    ImGui::Text("Export talent tree:");
                    ImGui::InputText("##treeEditorExportTalentInput", &uiData.treeEditorExportTreeString, ImGuiInputTextFlags_ReadOnly);
                    ImGui::SameLine();
                    if (ImGui::Button("Export##treeEditorExportTalentTreeButton")) {
                        uiData.treeEditorExportTreeString = Engine::createTreeStringRepresentation(talentTreeCollection.activeTree());
                    }
                }
                //Call all the different modal popups that can appear
                ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                if (ImGui::BeginPopupModal("Load preset confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("Do you want to load the preset?\nCurrent tree will be overwritten!");

                    ImGui::SetItemDefaultFocus();
                    if (ImGui::Button("OK", ImVec2(120, 0))) {
                        talentTreeCollection.activeTree() = Engine::loadTreePreset(Presets::RETURN_PRESET(uiData.treeEditorPresetClassCombo, uiData.treeEditorPresetSpecCombo));
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
                            ImGui::CloseCurrentPopup();
                        }
                        uiData.treeEditorIsCustomTreeFileListValid = false;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
                    ImGui::EndPopup();
                }
                if (ImGui::BeginPopupModal("Invalid import tree string", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("Tree import is invalid!");

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
    }

    void validateAndInsertTalent(UIData& uiData, TalentTreeCollection& talentTreeCollection, std::map<int, std::shared_ptr<Engine::Talent>> comboIndexTalentMap) {
        //check if position is not occupied
        for (auto& talent : talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.orderedTalents) {
            if (uiData.treeEditorCreationTalent->row == talent.second->row && uiData.treeEditorCreationTalent->column == talent.second->column) {
                ImGui::OpenPopup("Talent spot occupied");
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
                [](std::shared_ptr<Engine::Talent> root) { return root->parents.size() > 0; }),
            talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.talentRoots.end());
        //-> if talent has no parent, add to root
        if (uiData.treeEditorCreationTalent->parents.size() == 0)
            talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.talentRoots.push_back(uiData.treeEditorCreationTalent);
        uiData.treeEditorCreationTalent->index = talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.maxID;
        //call updateNodeCountAndMaxTalentPointsAndMaxID and updateOrderedTalentList
        Engine::updateNodeCountAndMaxTalentPointsAndMaxID(talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree);
        Engine::updateOrderedTalentList(talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree);

        talentTreeCollection.activeTree().presetName = "custom";
        Engine::validateLoadout(talentTreeCollection.activeTree(), true);

        uiData.treeEditorCreationTalent = std::make_shared<Engine::Talent>();
        uiData.treeEditorCreationTalentParentsPlaceholder.clear();
        uiData.treeEditorCreationTalentChildrenPlaceholder.clear();
    }

    void validateTalentUpdate(UIData& uiData, TalentTreeCollection& talentTreeCollection, std::map<int, std::shared_ptr<Engine::Talent>> comboIndexTalentMap) {
        //check if position is not occupied
        for (auto& talent : talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.orderedTalents) {
            if (uiData.treeEditorSelectedTalent->index == talent.second->index)
                continue;
            if (uiData.treeEditorSelectedTalent->row == talent.second->row && uiData.treeEditorSelectedTalent->column == talent.second->column) {
                ImGui::OpenPopup("Talent spot occupied");
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

        std::shared_ptr<Engine::Talent> originalTalent = talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.orderedTalents[
            uiData.treeEditorSelectedTalent->index
        ];
        std::vector<std::shared_ptr<Engine::Talent>> origParents = originalTalent->parents;
        std::vector<std::shared_ptr<Engine::Talent>> origChildren = originalTalent->children;

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
        Engine::validateLoadout(talentTreeCollection.activeTree(), true);
    }

    std::filesystem::path getCustomTreePath() {
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
        std::filesystem::path treePath = appPath / "CustomTrees";

        /* Free memory :) */
        CoTaskMemFree(path_tmp);

        //create app folder if it doesn't exist
        if (!std::filesystem::is_directory(appPath)) {
            std::filesystem::create_directory(appPath);
        }

        //create custom tree folder if it doesn't exist
        if (!std::filesystem::is_directory(treePath)) {
            std::filesystem::create_directory(treePath);
        }

        return treePath;
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
                catch (std::exception& e) {
                    //if file is not readable we can just ignore it, user shouldn't mingle in that folder anyway
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
        catch (std::exception& e) {
            return false;
        }
    }

    bool loadTreeFromFile(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
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
                    talentTreeCollection.activeTree() = Engine::parseTree(treeRepresentation);
                    return true;
                }
            }
        }
        catch (const std::exception& e) {
            return false;
        }
        return false;
    }

    bool deleteTreeFromFile(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        try {
            std::filesystem::path treeFile = uiData.treeEditorCustomTreeFileList[uiData.treeEditorCustomTreeListCurrent].first;
            std::filesystem::remove(treeFile);
            return true;
        }
        catch (std::exception& e) {
            return false;
        }
    }

    std::string treeNameToFileName(std::string treeName) {
        treeName.erase(std::remove_if(treeName.begin(), treeName.end(), [](auto const& c) -> bool { return !std::isalnum(c); }), treeName.end());
        return treeName;
    }

    bool getTreeNameFromFile(std::filesystem::path entry, std::string& treeName) {
        bool readSuccessful = false;
        std::string line;
        std::ifstream treeFile;
        treeFile.open(entry);

        if (treeFile.is_open())
        {
            if (!treeFile.eof())
            {
                std::getline(treeFile, line);
                //TTMTODO: is there a better way to check for valid format, maybe via mask?
                std::vector<std::string> treeComponents = Engine::splitString(line, ";");
                if (treeComponents.size() > 0) {
                    std::vector<std::string> treeInfo = Engine::splitString(treeComponents[0], ":");
                    if (treeInfo.size() == 6) {
                        treeName = treeInfo[1];
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
            if (uiData.treeEditorSelectedTalent != nullptr && uiData.treeEditorSelectedTalent->index == talent.second->index) {
                ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.9f, 0.2f, 0.2f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor(1.0f, 0.2f, 0.2f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(1.0f, 0.1f, 0.1f));
                changedColor = true;
            }
            ImGui::SetCursorPos(ImVec2(posX, posY));
            ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f + (talent.second->type == Engine::TalentType::SWITCH) * 8.0f * uiData.treeEditorZoomFactor + (talent.second->type == Engine::TalentType::PASSIVE) * 15.0f * uiData.treeEditorZoomFactor);
            ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 0.0f + (talent.second->type == Engine::TalentType::SWITCH) * 8.0f * uiData.treeEditorZoomFactor + (talent.second->type == Engine::TalentType::PASSIVE) * 15.0f * uiData.treeEditorZoomFactor);
            if (ImGui::Button(std::to_string(talent.second->index).c_str(), ImVec2(static_cast<float>(talentSize), static_cast<float>(talentSize)))) {
                selectTalent(uiData, talentTreeCollection, talent);
            }
            ImGui::PopStyleVar(2);
            ImGui::PopFont();
            if (changedColor) {
                ImGui::PopStyleColor(3);
            }
            if (ImGui::IsItemActive())
            {
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    uiData.treeEditorDragTalentStartRow = talent.second->row;
                    uiData.treeEditorDragTalentStartColumn = talent.second->column;
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
                AttachTalentEditTooltip(*talent.second);
            }
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

    void selectTalent(UIData& uiData, TalentTreeCollection& talentTreeCollection, std::pair<int, std::shared_ptr<Engine::Talent>> talent) {
        uiData.talentJustSelected = true;
        uiData.treeEditorSelectedTalent = std::make_shared<Engine::Talent>(*talent.second);
        uiData.treeEditPage = TreeEditPage::TreeEdit;
        uiData.treeEditorSelectedTalentParentsPlaceholder.clear();
        uiData.treeEditorSelectedTalentChildrenPlaceholder.clear();

        if (uiData.treeEditorSelectedTalent->parents.size() > 0 || uiData.treeEditorSelectedTalent->children.size() > 0) {
            std::map<std::shared_ptr<Engine::Talent>, int> comboIndexTalentMap;
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

    void repositionTalent(
        UIData& uiData,
        TalentTreeCollection& talentTreeCollection,
        std::pair<int, std::shared_ptr<Engine::Talent>> dragTalent,
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

        for (auto& talent : talentTreeCollection.trees[talentTreeCollection.activeTreeIndex].tree.orderedTalents) {
            if (dragTalent.second->index != talent.second->index
                && uiData.treeEditorDragTalentStartRow + resDeltaGridY == talent.second->row
                && uiData.treeEditorDragTalentStartColumn + resDeltaGridX == talent.second->column) {
                return;
            }
        }
        dragTalent.second->row = uiData.treeEditorDragTalentStartRow + resDeltaGridY;
        dragTalent.second->column = uiData.treeEditorDragTalentStartColumn + resDeltaGridX;

        if (resDeltaGridX != 0 || resDeltaGridY != 0) {
            talentTreeCollection.activeTree().presetName = "custom";
            Engine::validateLoadout(talentTreeCollection.activeTree(), true);
        }
    }
}