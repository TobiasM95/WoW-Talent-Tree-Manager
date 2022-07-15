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

#include "SimAnalysisWindow.h"

#include <numeric>

#include "TTMGUIPresets.h"

//TTMTODO: TEMPORARY RANDOM
#include <random>

namespace TTM {
    bool OptionSwitch(std::string leftText, std::string rightText, int* switchVariable, float switchWidth, bool centered, std::string uniqueSliderId) {
        float rowWidth = ImGui::GetContentRegionAvail().x;
        float leftTextWidth = ImGui::CalcTextSize(leftText.c_str()).x;
        float padding = ImGui::GetStyle().ItemSpacing.x;
        ImVec2 start = ImGui::GetCursorPos();

        bool eventFired = false;

        if (centered) {
            ImGui::SetCursorPos(ImVec2(start.x + 0.5f * rowWidth - 0.5f * switchWidth - padding - leftTextWidth, start.y));
            ImGui::Text(leftText.c_str());

            ImGui::SetCursorPos(ImVec2(start.x + 0.5f * rowWidth - 0.5f * switchWidth, start.y));
            ImGui::SetNextItemWidth(switchWidth);
            if (ImGui::SliderInt(("##" + uniqueSliderId).c_str(), switchVariable, 0, 1, "", ImGuiSliderFlags_NoInput)) {
                eventFired = true;
            }

            ImGui::SetCursorPos(ImVec2(start.x + 0.5f * rowWidth + 0.5f * switchWidth + padding, start.y));
            ImGui::Text(rightText.c_str());
            ImGui::Spacing();
            ImGui::Spacing();
        }
        else {
            ImGui::Text(leftText.c_str());
            ImGui::SameLine();

            ImGui::SetNextItemWidth(switchWidth);
            if (ImGui::SliderInt(("##" + uniqueSliderId).c_str(), (int*)&switchVariable, 0, 1, "", ImGuiSliderFlags_NoInput)) {
                eventFired = true;
            }
            ImGui::SameLine();

            ImGui::Text(rightText.c_str());
        }

        return eventFired;
    }

    static void HelperTooltip(std::string hovered, std::string helptext) {
        ImGui::TextUnformattedColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), hovered.c_str());
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 15.0f);
            ImGui::TextUnformatted(helptext.c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    static void AttachSimAnalysisTooltip(const UIData& uiData, Engine::Talent_s talent)
    {
        if (ImGui::IsItemHovered())
        {
            std::string idLabel = "Id: " + std::to_string(talent->index) + ", Pos: (" + std::to_string(talent->row) + ", " + std::to_string(talent->column) + ")";
            if (talent->type != Engine::TalentType::SWITCH) {
                ImGui::BeginTooltip();
                ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
                ImGui::Text(talent->getName().c_str());
                ImGui::PopFont();
                ImGui::Text(idLabel.c_str());

                ImGui::EndTooltip();
            }
            else {
                ImGui::BeginTooltip();
                ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
                ImGui::Text(talent->name.c_str());
                ImGui::PopFont();
                ImGui::Text(idLabel.c_str());
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
                ImGui::Text(talent->nameSwitch.c_str());
                ImGui::PopFont();
                ImGui::Text(idLabel.c_str());

                ImGui::EndTooltip();
            }
        }
    }

    void RenderSimAnalysisWindow(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
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

            placeSimAnalysisTreeElements(uiData, talentTreeCollection);

            ImGui::EndChild();
        }
        ImGui::End();
        ImGui::PopStyleVar();
        if (ImGui::Begin("SettingsWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
            if (ImGui::Button("Sim Analysis Settings", ImVec2(ImGui::GetContentRegionAvail().x, 25))) {
                //potentially multiple sim analysis pages
            }
            ImGui::Spacing();
            //switch (uiData.loadoutEditPage) {
            //case LoadoutEditPage::LoadoutInformation: {

            ImGui::Text("Import Raidbots results:");
            ImGui::SameLine();
            HelperTooltip("(?)", "In order to use the sim analysis tool, create a Raidbots sim with copies (not profilesets!) that are named after your loadout skillsets.\n\nIf you have multiple skillsets with the same name, the first one is used! If no skillset matches the sim import then that dps value is discarded.\n\nIf your sim is done, take the result URL and paste it here.");
            ImGui::InputText("##simAnalysisRaidbotsInputText", &uiData.raidbotsInputURL, ImGuiInputTextFlags_AutoSelectAll);
            ImGui::SameLine();
            if (ImGui::Button("Add result##simAnalysisRaidbotsAddResultButton")) {
                GenerateFakeData(talentTreeCollection.activeTree());
                AnalyzeRawResults(talentTreeCollection.activeTree());
                CalculateAnalysisRankings(uiData, talentTreeCollection.activeTree().analysisResult);
                UpdateColorGlowTextures(uiData, talentTreeCollection.activeTree(), talentTreeCollection.activeTree().analysisResult);
            }

            if (ImGui::BeginListBox("##simAnalysisResultsListbox", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
            {
                for (int n = 0; n < talentTreeCollection.activeTree().simAnalysisRawResults.size(); n++)
                {
                    const bool is_selected = (talentTreeCollection.activeTree().selectedSimAnalysisRawResult == n);
                    if (ImGui::Selectable((talentTreeCollection.activeTree().simAnalysisRawResults[n].name + "##" + std::to_string(n)).c_str(), is_selected)) {
                        talentTreeCollection.activeTree().selectedSimAnalysisRawResult = n;
                    }

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndListBox();
            }
            if (ImGui::Button("Remove result##simAnalysisRaidbotsRemoveResultButton")) {
                if (talentTreeCollection.activeTree().selectedSimAnalysisRawResult >= 0
                    && talentTreeCollection.activeTree().selectedSimAnalysisRawResult < talentTreeCollection.activeTree().simAnalysisRawResults.size()) {
                    talentTreeCollection.activeTree().simAnalysisRawResults.erase(
                        talentTreeCollection.activeTree().simAnalysisRawResults.begin() + 
                        talentTreeCollection.activeTree().selectedSimAnalysisRawResult);
                    AnalyzeRawResults(talentTreeCollection.activeTree());
                    CalculateAnalysisRankings(uiData, talentTreeCollection.activeTree().analysisResult);
                    UpdateColorGlowTextures(uiData, talentTreeCollection.activeTree(), talentTreeCollection.activeTree().analysisResult);
                }
            }
            ImGui::Separator();

            Engine::AnalysisResult& res = talentTreeCollection.activeTree().analysisResult;
            ImGui::Text("Currently analyzed skillsets: %d", res.skillsetCount);
            if (res.skillsetDPS.size() > 0) {
                ImGui::Text("Lowest skillset \"%s\" in import \"%s\" dps: %.1f", 
                    res.lowestDPSSkillset.first.second.c_str(),
                    res.lowestDPSSkillset.first.first.c_str(),
                    res.lowestDPSSkillset.second);
                ImGui::Text("Median skillset \"%s\" in import \"%s\" dps: %.1f",
                    res.medianDPSSkillset.first.second.c_str(),
                    res.medianDPSSkillset.first.first.c_str(),
                    res.medianDPSSkillset.second);
                ImGui::Text("Highest skillset \"%s\" in import \"%s\" dps: %.1f",
                    res.highestDPSSkillset.first.second.c_str(),
                    res.highestDPSSkillset.first.first.c_str(),
                    res.highestDPSSkillset.second);
                ImGui::Text("Skillset distribution:");
                ImGui::PlotHistogram("##allSkillsetsDistributionHistogram",
                    &res.skillsetDPS[0],
                    static_cast<int>(res.skillsetDPS.size()),
                    0, NULL,
                    0.95f * res.lowestDPSSkillset.second,
                    1.05f * res.highestDPSSkillset.second,
                    ImVec2(0, 80.0f));
            }
            ImGui::Separator();

            ImGui::Text("Visualization settings:");
            ImGui::Spacing();
            ImGui::Spacing();
            if (OptionSwitch("Talent Icon", "Rating", &uiData.simAnalysisIconRatingSwitch, 80.0f, true, "simAnalysisRatingSwitch")) {
                //probably needs nothing here
            }
            if (OptionSwitch("Relative dps", "Ranking", &uiData.relativeDpsRankingSwitch, 80.0f, true, "simAnalysisRelativeDpsRankingSwitch")) {
                CalculateAnalysisRankings(uiData, talentTreeCollection.activeTree().analysisResult);
                UpdateColorGlowTextures(uiData, talentTreeCollection.activeTree(), talentTreeCollection.activeTree().analysisResult);
            }
            ImGui::Text("Ranking metric:");
            int oldSetting = uiData.topMedianPerformanceSwitch;
            ImGui::RadioButton("Top 1", &uiData.topMedianPerformanceSwitch, static_cast<int>(Engine::PerformanceMetric::TOP1));
            ImGui::SameLine();
            ImGui::RadioButton("Top 3", &uiData.topMedianPerformanceSwitch, static_cast<int>(Engine::PerformanceMetric::TOP3));
            ImGui::SameLine();
            ImGui::RadioButton("Top 5", &uiData.topMedianPerformanceSwitch, static_cast<int>(Engine::PerformanceMetric::TOP5));
            ImGui::SameLine();
            ImGui::RadioButton("Median", &uiData.topMedianPerformanceSwitch, static_cast<int>(Engine::PerformanceMetric::MEDIAN));
            ImGui::SameLine();
            ImGui::RadioButton("Top 1 + Median", &uiData.topMedianPerformanceSwitch, static_cast<int>(Engine::PerformanceMetric::TOPMEDIAN));
            if (oldSetting != uiData.topMedianPerformanceSwitch) {
                CalculateAnalysisRankings(uiData, talentTreeCollection.activeTree().analysisResult);
                UpdateColorGlowTextures(uiData, talentTreeCollection.activeTree(), talentTreeCollection.activeTree().analysisResult);
            }

            ImGui::Separator();
            ImGui::Text("Export:");
            ImGui::Spacing();
            ImGui::Spacing();
            if (ImGui::Button("Put past top performing skillset in loadout")) {

            }
            if (ImGui::Button("Create actual top performing skillset")) {

            }

            /*if (ImGui::BeginPopupModal("Import skillsets result", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Imported %d skillsets!\n(%d skillsets might have been discarded due to mismatched trees\nor corrupted import strings.)",
                    uiData.loadoutEditorImportSkillsetsResult.first, uiData.loadoutEditorImportSkillsetsResult.second);

                ImGui::SetItemDefaultFocus();
                if (ImGui::Button("OK", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }*/
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

    void placeSimAnalysisTreeElements(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        Engine::TalentTree& tree = talentTreeCollection.activeTree();
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
            bool talentIsSearchedFor = false;
            bool searchActive = uiData.talentSearchString != "";
            ImGui::SetCursorPos(ImVec2(posX - 0.5f * (uiData.treeEditorZoomFactor * uiData.redIconGlow.width - talentSize), posY - 0.5f * (uiData.treeEditorZoomFactor * uiData.redIconGlow.height - talentSize)));
            if (uiData.enableGlow && !searchActive && uiData.simAnalysisColorGlowTextures.count(talent.first)) {
                ImGui::Image(
                    uiData.simAnalysisColorGlowTextures[talent.first].second.texture,
                    ImVec2(uiData.treeEditorZoomFactor * uiData.greenIconGlow.width, uiData.treeEditorZoomFactor * uiData.greenIconGlow.height),
                    ImVec2(0, 0), ImVec2(1, 1),
                    ImVec4(1, 1, 1, 1.0f - 0.5f * (uiData.style == Presets::STYLES::COMPANY_GREY))
                );
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

            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
            ImGui::PushID((std::to_string(talent.second->points) + "/" + std::to_string(talent.second->maxPoints) + "##" + std::to_string(talent.second->index)).c_str());
            TextureInfo iconContent;
            if (talent.second->type == Engine::TalentType::SWITCH) {
                iconContent = uiData.splitIconIndexMap[talent.second->index];
            }
            else {
                iconContent = uiData.iconIndexMap[talent.second->index].first;
            }
            if (ImGui::ImageButton(iconContent.texture,
                ImVec2(static_cast<float>(talentSize), static_cast<float>(talentSize)), ImVec2(0, 0), ImVec2(1, 1), 0
            )) {
            }
            ImGui::PopID();
            ImGui::PopStyleColor(5);
            drawSimAnalysisShapeAroundTalent(
                talent.second,
                drawList,
                imStyle.Colors,
                ImVec2(posX, posY),
                talentSize,
                ImGui::GetWindowPos(),
                ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY()),
                uiData,
                talentTreeCollection,
                searchActive,
                talentIsSearchedFor);
            ImGui::PopFont();
            AttachSimAnalysisTooltip(uiData, talent.second);
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
                    uiData,
                    true);
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

    void AnalyzeRawResults(Engine::TalentTree& tree) {
        Engine::AnalysisResult result;
        int col = 0;
        for (auto& indexTalentPair : tree.orderedTalents) {
            result.indexToArrayColMap[indexTalentPair.first] = col;
            if (indexTalentPair.second->type == Engine::TalentType::SWITCH) {
                col += 2;
            }
            else {
                col += indexTalentPair.second->maxPoints;
            }
        }

        std::vector<std::pair<std::string, std::string>> tempSkillsetNames;
        for (auto& simRes : tree.simAnalysisRawResults) {
            for (int i = 0; i < simRes.dps.size(); i++) {
                std::vector<int> talentSelection(col, 0);
                for (auto& indexPointsPair : simRes.skillsets[i].assignedSkillPoints) {
                    if (indexPointsPair.second > 0) {
                        talentSelection[result.indexToArrayColMap[indexPointsPair.first] + indexPointsPair.second - 1] = 1;
                    }
                }
                tempSkillsetNames.emplace_back(simRes.name, simRes.skillsets[i].name);
                result.talentArray.push_back(talentSelection);
                result.skillsetDPS.push_back(simRes.dps[i]);
            }
        }
        result.skillsetCount = static_cast<int>(result.skillsetDPS.size());
        if (result.skillsetCount == 0) {
            result.indexToArrayColMap.clear();
            tree.analysisResult = std::move(result);
            return;
        }
        //sort talentArray and skillsetDPS vectors in the same order
        auto p = sort_permutation(result.skillsetDPS,
            [](float const& a, float const& b) { return a < b; });

        result.skillsetDPS = apply_permutation(result.skillsetDPS, p);
        result.talentArray = apply_permutation(result.talentArray, p);
        tempSkillsetNames = apply_permutation(tempSkillsetNames, p);

        result.lowestDPSSkillset = std::pair<std::pair<std::string, std::string>, float>(tempSkillsetNames[0], result.skillsetDPS[0]);
        int medianIndex = (result.skillsetCount + 1) / 2;
        result.medianDPSSkillset = std::pair<std::pair<std::string, std::string>, float>(tempSkillsetNames[medianIndex], result.skillsetDPS[medianIndex]);
        result.highestDPSSkillset = std::pair<std::pair<std::string, std::string>, float>(tempSkillsetNames[result.skillsetCount - 1], result.skillsetDPS[result.skillsetCount - 1]);

        //calculate talent performances
        std::vector<std::pair<std::string, std::string>> tempTalentSkillsetNames;
        for (auto& indexTalentPair : tree.orderedTalents) {
            int maxP = indexTalentPair.second->type == Engine::TalentType::SWITCH ? 2 : indexTalentPair.second->maxPoints;
            for (int points = 1; points <= maxP; points++) {
                int colIndex = result.indexToArrayColMap[indexTalentPair.first] + points - 1;
                Engine::TalentPerformanceInfo tInfo;
                tempTalentSkillsetNames.clear();
                for (int i = 0; i < result.talentArray.size(); i++) {
                    if (result.talentArray[i][colIndex] > 0) {
                        tInfo.skillsetDPS.push_back(result.skillsetDPS[i]);
                        tempTalentSkillsetNames.push_back(tempSkillsetNames[i]);
                    }
                }
                if (tempTalentSkillsetNames.size() > 0 && tInfo.skillsetDPS.size() > 0) {
                    tInfo.lowestDPSSkillset = std::pair<std::pair<std::string, std::string>, float>(tempTalentSkillsetNames[0], tInfo.skillsetDPS[0]);
                    size_t medianIndex = (tInfo.skillsetDPS.size() + 1) / 2;
                    tInfo.medianDPSSkillset = std::pair<std::pair<std::string, std::string>, float>(tempTalentSkillsetNames[medianIndex], tInfo.skillsetDPS[medianIndex]);
                    tInfo.highestDPSSkillset = std::pair<std::pair<std::string, std::string>, float>(tempTalentSkillsetNames[tInfo.skillsetDPS.size() - 1], tInfo.skillsetDPS[tInfo.skillsetDPS.size() - 1]);
                }
                result.talentPerformances.push_back(tInfo);
            }
        }

        tree.analysisResult = std::move(result);
    }

    void CalculateAnalysisRankings(UIData& uiData, Engine::AnalysisResult& result) {
        result.minRelPerf = 1.0f;
        result.talentAbsolutePositionRankings.clear();
        result.talentRelativePerformanceRankings.clear();
        if (result.skillsetCount == 0 || result.talentPerformances.size() == 0) {
            return;
        }

        Engine::PerformanceMetric performanceMetric = static_cast<Engine::PerformanceMetric>(uiData.topMedianPerformanceSwitch);

        const int DPS_SLOTS = 6;
        std::vector<std::pair<int, std::vector<float>>> performance;
        int numTalents = static_cast<int>(result.talentPerformances.size());
        //TTMNOTE: dps[0] holds the relative performance value and is main sorting value, all additional criteria are put in dps[1...], relevant for TOPMEDIAN
        for (int i = 0; i < numTalents; i++) {
            switch (performanceMetric) {
            case Engine::PerformanceMetric::TOP1: {
                performance.emplace_back(i, std::vector<float>(DPS_SLOTS, result.talentPerformances[i].highestDPSSkillset.second));
            }break;
            case Engine::PerformanceMetric::TOP3: {
                std::vector<float> dps(DPS_SLOTS, 0);
                float avg = 0.0f;
                if (result.talentPerformances[i].skillsetDPS.size() >= 3) {
                    for (int j = 0; j < 3; j++) {
                        dps[j + 1] = result.talentPerformances[i].skillsetDPS[result.talentPerformances[i].skillsetDPS.size() - 1 - j];
                        avg += dps[j + 1];
                    }
                }
                dps[0] = avg / 3;
                performance.emplace_back(i, dps);
            }break;
            case Engine::PerformanceMetric::TOP5: {
                std::vector<float> dps(DPS_SLOTS, 0);
                float avg = 0.0f;
                if (result.talentPerformances[i].skillsetDPS.size() >= 5) {
                    for (int j = 0; j < 5; j++) {
                        dps[j + 1] = result.talentPerformances[i].skillsetDPS[result.talentPerformances[i].skillsetDPS.size() - 1 - j];
                        avg += dps[j + 1];
                    }
                }
                dps[0] = avg / 5;
                performance.emplace_back(i, dps);
            }break;
            case Engine::PerformanceMetric::MEDIAN: {
                performance.emplace_back(i, std::vector<float>(DPS_SLOTS, result.talentPerformances[i].medianDPSSkillset.second));
            }break;
            case Engine::PerformanceMetric::TOPMEDIAN: {
                std::vector<float> dps(DPS_SLOTS, 0);
                dps[0] = result.talentPerformances[i].highestDPSSkillset.second;
                dps[1] = result.talentPerformances[i].medianDPSSkillset.second;
                performance.emplace_back(i, dps);
            }break;
            }
        }
        std::sort(performance.begin(), performance.end(), [](auto& left, auto& right) {
            return left.second < right.second;
            });
        float maxPerf = performance[performance.size() - 1].second[0];
        float minPerf = maxPerf;
        for (auto& p : performance) {
            if (p.second[0] > 0.0f && p.second[0] < minPerf) {
                minPerf = p.second[0];
            }
        }
        //this is needed for color glow texture generation, since that is a separate function
        result.minRelPerf = minPerf / maxPerf;

        result.talentAbsolutePositionRankings.resize(numTalents);
        result.talentRelativePerformanceRankings.resize(numTalents);

        std::map<std::vector<float>, int> rankMap;
        int rankCounter = 0;
        for (int i = 0; i < numTalents; i++) {
            if (performance[i].second[0] > 0.0f && !rankMap.count(performance[i].second)) {
                rankMap[performance[i].second] = rankCounter;
                rankCounter++;
            }
        }
        for (int i = 0; i < numTalents; i++) {
            result.talentAbsolutePositionRankings[performance[i].first] = performance[i].second[0] > 0.0f ? rankMap[performance[i].second] * 1.0f / (rankCounter - 1) : -1.0f;
            result.talentRelativePerformanceRankings[performance[i].first] = performance[i].second[0] > 0.0f ? performance[i].second[0] / maxPerf : -1.0f;
        }
    }


    void UpdateColorGlowTextures(UIData& uiData, Engine::TalentTree& tree, Engine::AnalysisResult& result) {
        //create glows and colors
        //glow always goes from red to green 
        //but for absolute rankings red = 0, green = 1
        //and for relative rankings red = minRelPerf, green = 1
        //store pairs of (color, glowTexture) in uiData
        for (auto& indexColorGlowPair : uiData.simAnalysisColorGlowTextures) {
            if (indexColorGlowPair.second.second.texture) {
                indexColorGlowPair.second.second.texture->Release();
                indexColorGlowPair.second.second.texture = nullptr;
            }
        }
        uiData.simAnalysisColorGlowTextures.clear();

        if (result.talentAbsolutePositionRankings.size() == 0 || result.talentRelativePerformanceRankings.size() == 0) {
            return;
        }

        bool calculateAbsolutePositionRanking = uiData.relativeDpsRankingSwitch;
        for (auto& indexTalentPair : tree.orderedTalents) {
            float highestRanking = -1.0f;
            std::vector<unsigned char> color(3);
            if (calculateAbsolutePositionRanking) {
                //get highest position ranking value of multipoint/switch/regular talent
                if (indexTalentPair.second->type == Engine::TalentType::SWITCH) {
                    if (result.talentAbsolutePositionRankings[result.indexToArrayColMap[indexTalentPair.first]]
                        > result.talentAbsolutePositionRankings[result.indexToArrayColMap[indexTalentPair.first] + 1]) {
                        highestRanking = result.talentAbsolutePositionRankings[result.indexToArrayColMap[indexTalentPair.first]];
                    }
                    else {
                        highestRanking = result.talentAbsolutePositionRankings[result.indexToArrayColMap[indexTalentPair.first] + 1];
                    }
                }
                else {
                    for (int i = 0; i < indexTalentPair.second->maxPoints; i++) {
                        if (result.talentAbsolutePositionRankings[result.indexToArrayColMap[indexTalentPair.first] + i] > highestRanking) {
                            highestRanking = result.talentAbsolutePositionRankings[result.indexToArrayColMap[indexTalentPair.first] + i];
                        }
                    }
                }
                //get interpolated red-green color for 0=red, 1=green
                color[0] = 2.0f * (1.0f - highestRanking) >= 1.0f ? 255 : static_cast<unsigned char>(2.0f * (1.0f - highestRanking) * 255);
                color[1] = 2.0f * highestRanking >= 1.0f ? 255 : static_cast<unsigned char>(2.0f * highestRanking * 255);
                color[2] = 0;
            }
            else {
                //get highest position ranking value of multipoint/switch/regular talent
                if (indexTalentPair.second->type == Engine::TalentType::SWITCH) {
                    if (result.talentRelativePerformanceRankings[result.indexToArrayColMap[indexTalentPair.first]]
                        > result.talentRelativePerformanceRankings[result.indexToArrayColMap[indexTalentPair.first] + 1]) {
                        highestRanking = result.talentRelativePerformanceRankings[result.indexToArrayColMap[indexTalentPair.first]];
                    }
                    else {
                        highestRanking = result.talentRelativePerformanceRankings[result.indexToArrayColMap[indexTalentPair.first] + 1];
                    }
                }
                else {
                    for (int i = 0; i < indexTalentPair.second->maxPoints; i++) {
                        if (result.talentRelativePerformanceRankings[result.indexToArrayColMap[indexTalentPair.first] + i] > highestRanking) {
                            highestRanking = result.talentRelativePerformanceRankings[result.indexToArrayColMap[indexTalentPair.first] + i];
                        }
                    }
                }
                //get interpolated red-green color for minRelPerf=red, 1=green
                float intermediate = (highestRanking - result.minRelPerf) / (1.0f - result.minRelPerf);
                color[0] = 2.0f * (1.0f - intermediate) >= 1.0f ? 255 : static_cast<unsigned char>(2.0f * (1.0f - intermediate) * 255);
                color[1] = 2.0f * intermediate >= 1.0f ? 255 : static_cast<unsigned char>(2.0f * intermediate * 255);
                color[2] = 0;
            }
            if(highestRanking > 0.0f){
                //create glow texture with that color
                TextureInfo texInfo;
                CreateGlowTextureFromColor(&texInfo.texture, &texInfo.width, &texInfo.height, uiData.g_pd3dDevice, color);
                //add it to map with talent index as key
                uiData.simAnalysisColorGlowTextures[indexTalentPair.first] = std::pair<ImVec4, TextureInfo>(ImVec4(color[0] / 255.0f, color[1] / 255.0f, color[2] / 255.0f, 1.0f), texInfo);
            }
        }

    }

    //taken from: https://stackoverflow.com/questions/17074324/how-can-i-sort-two-vectors-in-the-same-way-with-criteria-that-uses-only-one-of
    template <typename T, typename Compare>
    std::vector<std::size_t> sort_permutation(
        const std::vector<T>& vec,
        Compare compare)
    {
        std::vector<std::size_t> p(vec.size());
        std::iota(p.begin(), p.end(), 0);
        std::sort(p.begin(), p.end(),
            [&](std::size_t i, std::size_t j) { return compare(vec[i], vec[j]); });
        return p;
    }

    //taken from: https://stackoverflow.com/questions/17074324/how-can-i-sort-two-vectors-in-the-same-way-with-criteria-that-uses-only-one-of
    template <typename T>
    std::vector<T> apply_permutation(
        const std::vector<T>& vec,
        const std::vector<std::size_t>& p)
    {
        std::vector<T> sorted_vec(vec.size());
        std::transform(p.begin(), p.end(), sorted_vec.begin(),
            [&](std::size_t i) { return vec[i]; });
        return sorted_vec;
    }

    void GenerateFakeData(Engine::TalentTree& tree) {
        std::random_device dev;
        std::mt19937 rng(dev());
        rng.seed(12345023);
        std::normal_distribution<> randDPS(15000, 5000);

        std::vector<Engine::SimResult>& res = tree.simAnalysisRawResults;
        for (int i = 0; i < 3; i++) {
            Engine::SimResult simResult;
            simResult.name = "Test sim result " + std::to_string(i);
            for (int j = 0; j < 50; j++) {
                Engine::TalentSkillset skillset;
                skillset.name = "Skillset " + std::to_string(j);
                for (auto& indexTalentPair : tree.orderedTalents) {
                    std::uniform_int_distribution<std::mt19937::result_type> randPoints(0, indexTalentPair.second->maxPoints);
                    skillset.assignedSkillPoints[indexTalentPair.first] = randPoints(rng);
                }
                simResult.skillsets.push_back(skillset);
                simResult.dps.push_back(static_cast<float>(randDPS(rng)));
            }
            res.push_back(simResult);
        }
    }
}