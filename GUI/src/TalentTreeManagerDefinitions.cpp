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

#include "TalentTreeManagerDefinitions.h"

#include "imgui_internal.h"

namespace TTM {
    void refreshIconList(UIData& uiData) {
        std::filesystem::path iconRootPath = "resources/icons/";
        std::filesystem::path customIconPath = "resources/icons/custom";
        uiData.iconPathMap.clear();
        //first iterate through pre-shipped directories and add paths to map while skipping custom dir
        if (!std::filesystem::is_directory(iconRootPath)) {
            return;
        }
        for (auto& entry : std::filesystem::recursive_directory_iterator{ iconRootPath }) {
            if (!std::filesystem::is_regular_file(entry)
                || entry.path().parent_path().compare(customIconPath) == 0) {
                continue;
            }
            if (entry.path().extension() == ".png") {
                uiData.iconPathMap[entry.path().filename().string()] = entry.path();
            }
        }
        if (!std::filesystem::is_directory(customIconPath)) {
            return;
        }
        for (auto& entry : std::filesystem::recursive_directory_iterator{ customIconPath }) {
            if (std::filesystem::is_regular_file(entry) && entry.path().extension() == ".png") {
                uiData.iconPathMap[entry.path().filename().string()] = entry.path();
            }
        }
    }

    void loadActiveIcons(UIData& uiData, TalentTreeCollection& talentTreeCollection, bool forceReload) {
        if (talentTreeCollection.activeTreeIndex == -1 || (!forceReload && uiData.loadedIconTreeIndex == talentTreeCollection.activeTreeIndex)) {
            return;
        }
        //FREE STUFF HERE
        //FREE ALL POINTERS!
        for (auto& indexTexInfoPair : uiData.iconIndexMap) {
            if (indexTexInfoPair.second.first.texture) {
                indexTexInfoPair.second.first.texture->Release();
                indexTexInfoPair.second.first.texture = nullptr;
            }
            if (indexTexInfoPair.second.second.texture) {
                indexTexInfoPair.second.second.texture->Release();
                indexTexInfoPair.second.second.texture = nullptr;
            }
        }
        for (auto& indexTexInfoPair : uiData.splitIconIndexMap) {
            if (indexTexInfoPair.second.texture) {
                indexTexInfoPair.second.texture->Release();
                indexTexInfoPair.second.texture = nullptr;
            }
        }
        uiData.iconIndexMap.clear();
        uiData.splitIconIndexMap.clear();
        //load default texture first
        int defaultImageWidth = 0;
        int defaultImageHeight = 0;
        ID3D11ShaderResourceView* defaultTexture = NULL;
        std::string iconPath(uiData.defaultIconPath.string());
        bool defaultSuccess = LoadDefaultTexture(&defaultTexture, &defaultImageWidth, &defaultImageHeight, uiData.g_pd3dDevice, Engine::TalentType::PASSIVE);
        bool redGlowSuccess = LoadRedIconGlowTexture(&uiData.redIconGlow.texture, &uiData.redIconGlow.width, &uiData.redIconGlow.height, uiData.g_pd3dDevice);
        bool greenGlowSuccess = LoadGreenIconGlowTexture(&uiData.greenIconGlow.texture, &uiData.greenIconGlow.width, &uiData.greenIconGlow.height, uiData.g_pd3dDevice);
        bool goldGlowSuccess = LoadGoldIconGlowTexture(&uiData.goldIconGlow.texture, &uiData.goldIconGlow.width, &uiData.goldIconGlow.height, uiData.g_pd3dDevice);
        bool blueGlowSuccess = LoadBlueIconGlowTexture(&uiData.blueIconGlow.texture, &uiData.blueIconGlow.width, &uiData.blueIconGlow.height, uiData.g_pd3dDevice);
        if (!(defaultSuccess && redGlowSuccess && greenGlowSuccess && goldGlowSuccess && blueGlowSuccess)) {
            //TTMNOTE: this should not happen anymore
            throw std::runtime_error("Cannot create default icon!");
        }

        //load individual icons or replace with default texture if error
        for (auto& talent : talentTreeCollection.activeTree().orderedTalents) {
            loadIcon(uiData, talent.second->index, talent.second->iconName.first, defaultTexture, defaultImageWidth, defaultImageHeight, true, talent.second->type);
            loadIcon(uiData, talent.second->index, talent.second->iconName.second, defaultTexture, defaultImageWidth, defaultImageHeight, false, talent.second->type);
            if (talent.second->type == Engine::TalentType::SWITCH) {
                loadSplitIcon(uiData, talent.second, defaultTexture, defaultImageWidth, defaultImageHeight);
            }
        }
        uiData.loadedIconTreeIndex = talentTreeCollection.activeTreeIndex;
    }

    void loadIcon(UIData& uiData, int index, std::string iconName, ID3D11ShaderResourceView* defaultTexture, int defaultImageWidth, int defaultImageHeight, bool first, Engine::TalentType talentType) {
        int width = 0;
        int height = 0;
        ID3D11ShaderResourceView* texture = NULL;
        std::string path;
        if (uiData.iconPathMap.count(iconName)) {
            path = uiData.iconPathMap[iconName].string();
            bool ret = LoadTextureFromFile(path.c_str(), &texture, &width, &height, uiData.g_pd3dDevice, talentType);
            if (!ret) {
                TextureInfo textureInfo;
                textureInfo.texture = defaultTexture;
                textureInfo.width = defaultImageWidth;
                textureInfo.height = defaultImageHeight;
                if (first) {
                    uiData.iconIndexMap[index].first = textureInfo;
                }
                else {
                    uiData.iconIndexMap[index].second = textureInfo;
                }
            }
            else {
                TextureInfo textureInfo;
                textureInfo.texture = texture;
                textureInfo.width = width;
                textureInfo.height = height;
                if (first) {
                    uiData.iconIndexMap[index].first = textureInfo;
                }
                else {
                    uiData.iconIndexMap[index].second = textureInfo;
                }
            }
        }
        else {
            TextureInfo textureInfo;
            textureInfo.texture = defaultTexture;
            textureInfo.width = defaultImageWidth;
            textureInfo.height = defaultImageHeight;
            if (first) {
                uiData.iconIndexMap[index].first = textureInfo;
            }
            else {
                uiData.iconIndexMap[index].second = textureInfo;
            }
        }
    }

    void loadSplitIcon(UIData& uiData, Engine::Talent_s talent, ID3D11ShaderResourceView* defaultTexture, int defaultImageWidth, int defaultImageHeight) {
        int width = 0;
        int height = 0;
        ID3D11ShaderResourceView* texture = NULL;
        std::string path1;
        std::string path2;
        if (uiData.iconPathMap.count(talent->iconName.first)) {
            path1 = uiData.iconPathMap[talent->iconName.first].string();
        }
        else {
            path1 = uiData.defaultIconPath.string();
        }
        if (uiData.iconPathMap.count(talent->iconName.second)) {
            path2 = uiData.iconPathMap[talent->iconName.second].string();
        }
        else {
            path2 = uiData.defaultIconPath.string();
        }
        bool ret = LoadSplitTextureFromFile(path1.c_str(), path2.c_str(), &texture, &width, &height, uiData.g_pd3dDevice);
        if (!ret) {
            TextureInfo textureInfo;
            textureInfo.texture = defaultTexture;
            textureInfo.width = defaultImageWidth;
            textureInfo.height = defaultImageHeight;
            uiData.splitIconIndexMap[talent->index] = textureInfo;
        }
        else {
            TextureInfo textureInfo;
            textureInfo.texture = texture;
            textureInfo.width = width;
            textureInfo.height = height;
            uiData.splitIconIndexMap[talent->index] = textureInfo;
        }
    }

    TextureInfo loadTextureInfoFromFile(UIData& uiData, std::string iconName, Engine::TalentType talentType) {
        int width = 0;
        int height = 0;
        ID3D11ShaderResourceView* texture = NULL;
        std::string path;
        if (uiData.iconPathMap.count(iconName)) {
            path = uiData.iconPathMap[iconName].string();
        }
        else {
            path = uiData.defaultIconPath.string();
        }
        bool ret = LoadTextureFromFile(path.c_str(), &texture, &width, &height, uiData.g_pd3dDevice, talentType);
        TextureInfo textureInfo;
        if (!ret) {
            //load default texture first
            int defaultImageWidth = 0;
            int defaultImageHeight = 0;
            ID3D11ShaderResourceView* defaultTexture = NULL;
            std::string iconPath(uiData.defaultIconPath.string());
            bool ret = LoadDefaultTexture(&defaultTexture, &defaultImageWidth, &defaultImageHeight, uiData.g_pd3dDevice, talentType);
            if (!ret) {
                //TTMNOTE: This should not happen anymore
                throw std::runtime_error("Cannot create default icon!");
            }
            textureInfo.texture = defaultTexture;
            textureInfo.width = defaultImageWidth;
            textureInfo.height = defaultImageHeight;
        }
        else {
            textureInfo.texture = texture;
            textureInfo.width = width;
            textureInfo.height = height;
        }
        return textureInfo;
    }

    void drawArrowBetweenTalents(
        Engine::Talent_s t1,
        Engine::Talent_s t2,
        ImDrawList* drawList,
        ImVec2 windowPos,
        ImVec2 offset,
        ImVec2 talentWindowPadding,
        int talentHalfSpacing,
        int talentSize,
        float talentPadding,
        UIData& uiData,
        bool colored)
    {
        //Arrow constants
        float thickness = 2.0f * uiData.treeEditorZoomFactor;
        float relArrowSpace = 0.15f; //how much space should be between arrow and connecting talents in terms of relative to talentSize
        float relArrowHeadSize = 0.15f; //how long should each side of the the arrow head triangle be in terms of relative to talentSize
        float relArrowHeadAngle = 2.1f;
        ImU32 color = ImColor(120, 120, 120, 255);;
        if (colored) {
            if (t2->points > 0 && t1->points == t1->maxPoints) {
                if (t2->points == t2->maxPoints) {
                    color = ImColor(Presets::TALENT_MAXED_BORDER_COLOR); //Gold
                }
                else {
                    color = ImColor(Presets::TALENT_PARTIAL_BORDER_COLOR);
                }
            }
        }

        float p1X, p1Y, p2X, p2Y;
        if (t1->column < t2->column - 1) {
            if (t1->row < t2->row - 1) {
                //Arrow to the bottom right
                p1X = talentWindowPadding.x + (t1->column - 1) * 2 * talentHalfSpacing + talentPadding + talentSize;
                p1Y = talentWindowPadding.y + (t1->row - 1) * 2 * talentHalfSpacing + talentPadding + talentSize;
                p2X = talentWindowPadding.x + (t2->column - 1) * 2 * talentHalfSpacing + talentPadding;
                p2Y = talentWindowPadding.y + (t2->row - 1) * 2 * talentHalfSpacing + talentPadding;
            }
            else if (t1->row >= t2->row - 1 && t1->row <= t2->row + 1) {
                //Arrow right
                p1X = talentWindowPadding.x + (t1->column - 1) * 2 * talentHalfSpacing + talentPadding + talentSize;
                p1Y = talentWindowPadding.y + (t1->row - 1) * 2 * talentHalfSpacing + talentPadding + 0.5f * talentSize;
                p2X = talentWindowPadding.x + (t2->column - 1) * 2 * talentHalfSpacing + talentPadding;
                p2Y = talentWindowPadding.y + (t2->row - 1) * 2 * talentHalfSpacing + talentPadding + 0.5f * talentSize;
            }
            else {
                //Arrow top right
                p1X = talentWindowPadding.x + (t1->column - 1) * 2 * talentHalfSpacing + talentPadding + talentSize;
                p1Y = talentWindowPadding.y + (t1->row - 1) * 2 * talentHalfSpacing + talentPadding;
                p2X = talentWindowPadding.x + (t2->column - 1) * 2 * talentHalfSpacing + talentPadding;
                p2Y = talentWindowPadding.y + (t2->row - 1) * 2 * talentHalfSpacing + talentPadding + talentSize;
            }
        }
        else if (t1->column >= t2->column - 1 && t1->column <= t2->column + 1) {
            if (t1->row < t2->row - 1) {
                //Arrow straight down
                p1X = talentWindowPadding.x + (t1->column - 1) * 2 * talentHalfSpacing + talentPadding + 0.5f * talentSize;
                p1Y = talentWindowPadding.y + (t1->row - 1) * 2 * talentHalfSpacing + talentPadding + talentSize;
                p2X = talentWindowPadding.x + (t2->column - 1) * 2 * talentHalfSpacing + talentPadding + 0.5f * talentSize;
                p2Y = talentWindowPadding.y + (t2->row - 1) * 2 * talentHalfSpacing + talentPadding;
            }
            else if (t1->row >= t2->row - 1 && t1->row <= t2->row + 1) {
                //TTMTODO: This should never happen but there are currently no validations for imported trees to have proper positioning!
                p1X = talentWindowPadding.x + (t1->column - 1) * 2 * talentHalfSpacing + talentPadding + 0.5f * talentSize;
                p1Y = talentWindowPadding.y + (t1->row - 1) * 2 * talentHalfSpacing + talentPadding + 0.5f * talentSize;
                p2X = talentWindowPadding.x + (t2->column - 1) * 2 * talentHalfSpacing + talentPadding + 0.5f * talentSize;
                p2Y = talentWindowPadding.y + (t2->row - 1) * 2 * talentHalfSpacing + talentPadding + 0.5f * talentSize;
            }
            else {
                //Arrow straight up
                p1X = talentWindowPadding.x + (t1->column - 1) * 2 * talentHalfSpacing + talentPadding + 0.5f * talentSize;
                p1Y = talentWindowPadding.y + (t1->row - 1) * 2 * talentHalfSpacing + talentPadding;
                p2X = talentWindowPadding.x + (t2->column - 1) * 2 * talentHalfSpacing + talentPadding + 0.5f * talentSize;
                p2Y = talentWindowPadding.y + (t2->row - 1) * 2 * talentHalfSpacing + talentPadding + talentSize;
            }
        }
        else {
            if (t1->row < t2->row - 1) {
                //Arrow to the bottom left
                p1X = talentWindowPadding.x + (t1->column - 1) * 2 * talentHalfSpacing + talentPadding;
                p1Y = talentWindowPadding.y + (t1->row - 1) * 2 * talentHalfSpacing + talentPadding + talentSize;
                p2X = talentWindowPadding.x + (t2->column - 1) * 2 * talentHalfSpacing + talentPadding + talentSize;
                p2Y = talentWindowPadding.y + (t2->row - 1) * 2 * talentHalfSpacing + talentPadding;
            }
            else if (t1->row >= t2->row - 1 && t1->row <= t2->row + 1) {
                //Arrow straight left
                p1X = talentWindowPadding.x + (t1->column - 1) * 2 * talentHalfSpacing + talentPadding;
                p1Y = talentWindowPadding.y + (t1->row - 1) * 2 * talentHalfSpacing + talentPadding + 0.5f * talentSize;
                p2X = talentWindowPadding.x + (t2->column - 1) * 2 * talentHalfSpacing + talentPadding + talentSize;
                p2Y = talentWindowPadding.y + (t2->row - 1) * 2 * talentHalfSpacing + talentPadding + 0.5f * talentSize;
            }
            else {
                //Arrow to the top left
                p1X = talentWindowPadding.x + (t1->column - 1) * 2 * talentHalfSpacing + talentPadding;
                p1Y = talentWindowPadding.y + (t1->row - 1) * 2 * talentHalfSpacing + talentPadding;
                p2X = talentWindowPadding.x + (t2->column - 1) * 2 * talentHalfSpacing + talentPadding + talentSize;
                p2Y = talentWindowPadding.y + (t2->row - 1) * 2 * talentHalfSpacing + talentPadding + talentSize;
            }
        }
        p1X += windowPos.x - offset.x;
        p1Y += windowPos.y - offset.y;
        p2X += windowPos.x - offset.x;
        p2Y += windowPos.y - offset.y;
        float arrowLength = std::sqrt((p2X - p1X) * (p2X - p1X) + (p2Y - p1Y) * (p2Y - p1Y));
        float relArrowLength = (arrowLength - 2 * relArrowSpace * talentSize) / arrowLength;
        ImVec2 orig = ImVec2(p1X * relArrowLength + p2X * (1.0f - relArrowLength), p1Y * relArrowLength + p2Y * (1.0f - relArrowLength));
        ImVec2 target = ImVec2(p2X * relArrowLength + p1X * (1.0f - relArrowLength), p2Y * relArrowLength + p1Y * (1.0f - relArrowLength));
        drawList->AddLine(
            orig,
            target,
            color, thickness);
        ImVec2 arrowHeadVector = ImVec2(
            relArrowHeadSize * talentSize * (p2X - p1X) / arrowLength,
            relArrowHeadSize * talentSize * (p2Y - p1Y) / arrowLength
        );
        ImVec2 tri1 = ImRotate(arrowHeadVector, ImCos(relArrowHeadAngle), ImSin(relArrowHeadAngle));
        ImVec2 tri2 = ImRotate(arrowHeadVector, ImCos(2 * relArrowHeadAngle), ImSin(2 * relArrowHeadAngle));
        drawList->AddTriangleFilled(
            ImVec2(target.x + arrowHeadVector.x, target.y + arrowHeadVector.y),
            ImVec2(target.x + tri1.x, target.y + tri1.y),
            ImVec2(target.x + tri2.x, target.y + tri2.y),
            color
        );
    }

    void drawTreeEditorShapeAroundTalent(
        Engine::Talent_s talent,
        ImDrawList* drawList,
        ImVec4* colors,
        ImVec2 pos,
        int talentSize,
        ImVec2 windowPos,
        ImVec2 scroll,
        UIData& uiData,
        TalentTreeCollection& talentTreeCollection,
        bool selected,
        bool searchActive,
        bool talentIsSearchedFor)
    {
        ImVec4 borderCol = colors[ImGuiCol_WindowBg];
        ImVec4 textColor = colors[ImGuiCol_Text];
        ImVec4 textRectColor = colors[ImGuiCol_WindowBg];
        ImVec4 textRectBorderColor = colors[ImGuiCol_Text];
        if (!searchActive) {
            if (selected) {
                borderCol = Presets::TALENT_SELECTED_BORDER_COLOR;
            }
            else {
                if (uiData.style == Presets::STYLES::COMPANY_GREY) {
                    borderCol = Presets::TALENT_DEFAULT_BORDER_COLOR_COMPANY_GREY;
                }
                else {
                    borderCol = Presets::TALENT_DEFAULT_BORDER_COLOR;
                }
            }
            textRectColor = ImVec4(
                0.5f * textRectColor.x + 0.5f * borderCol.x,
                0.5f * textRectColor.y + 0.5f * borderCol.y,
                0.5f * textRectColor.z + 0.5f * borderCol.z,
                0.5f * textRectColor.w + 0.5f * borderCol.w
            );
            textRectBorderColor = ImVec4(
                0.5f * textRectBorderColor.x + 0.5f * borderCol.x,
                0.5f * textRectBorderColor.y + 0.5f * borderCol.y,
                0.5f * textRectBorderColor.z + 0.5f * borderCol.z,
                0.5f * textRectBorderColor.w + 0.5f * borderCol.w
            );
        }
        else {
            if (talentIsSearchedFor) {
                borderCol = Presets::TALENT_SEARCHED_BORDER_COLOR;
            }
            else {
                if (uiData.style == Presets::STYLES::COMPANY_GREY) {
                    borderCol = Presets::TALENT_DEFAULT_BORDER_COLOR_COMPANY_GREY;
                }
                else {
                    borderCol = Presets::TALENT_DEFAULT_BORDER_COLOR;
                }
            }
            textColor = colors[ImGuiCol_Text];
            textRectColor = ImVec4(
                0.5f * textRectColor.x + 0.5f * borderCol.x,
                0.5f * textRectColor.y + 0.5f * borderCol.y,
                0.5f * textRectColor.z + 0.5f * borderCol.z,
                0.5f * textRectColor.w + 0.5f * borderCol.w
            );
            textRectBorderColor = ImVec4(
                0.5f * textRectBorderColor.x + 0.5f * borderCol.x,
                0.5f * textRectBorderColor.y + 0.5f * borderCol.y,
                0.5f * textRectBorderColor.z + 0.5f * borderCol.z,
                0.5f * textRectBorderColor.w + 0.5f * borderCol.w
            );
        }


        switch (talent->type) {
        case Engine::TalentType::SWITCH: {
            drawList->AddNgonRotated(
                ImVec2(
                    pos.x + talentSize * 0.5f + windowPos.x - scroll.x,
                    pos.y + talentSize * 0.5f + windowPos.y - scroll.y
                ),
                talentSize * 0.65f - 3.5f * uiData.treeEditorZoomFactor,
                ImColor(borderCol),
                8,
                4 * uiData.treeEditorZoomFactor,
                IM_PI / 8.0f
            );
        }break;
        case Engine::TalentType::PASSIVE: {
            drawList->AddCircle(
                ImVec2(
                    pos.x + talentSize * 0.5f + windowPos.x - scroll.x,
                    pos.y + talentSize * 0.5f + windowPos.y - scroll.y
                ),
                talentSize * 0.62f - 3.5f * uiData.treeEditorZoomFactor,
                ImColor(borderCol),
                0,
                4 * uiData.treeEditorZoomFactor
            );
        }break;
        case Engine::TalentType::ACTIVE: {
            drawList->AddRect(
                ImVec2(
                    pos.x + windowPos.x - scroll.x,
                    pos.y + windowPos.y - scroll.y
                ),
                ImVec2(
                    pos.x + talentSize + windowPos.x - scroll.x,
                    pos.y + talentSize + windowPos.y - scroll.y
                ),
                ImColor(borderCol),
                0,
                0,
                4 * uiData.treeEditorZoomFactor
            );
        }break;
        }
        float textOffset = 0;
        if (uiData.treeEditorZoomFactor <= 1.0f) {
            ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[3]);
        }
        else if (uiData.treeEditorZoomFactor < 1.5f) {
            textOffset = 0.01f * talentSize;
            ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[0]);
        }
        else if (uiData.treeEditorZoomFactor < 2.0f) {
            textOffset = 0.015f * talentSize;
            ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
        }
        else {
            textOffset = 0.02f * talentSize;
            ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[2]);
        }
        ImVec2 bottomRight(pos.x + windowPos.x - scroll.x + talentSize, pos.y + windowPos.y - scroll.y + talentSize);
        ImVec2 textSize = ImGui::CalcTextSize("9/9");
        drawList->AddRectFilled(
            ImVec2(
                bottomRight.x - 0.65f * textSize.x - 0.05f * talentSize,
                bottomRight.y - 0.65f * textSize.y - 0.05f * talentSize
            ),
            ImVec2(
                bottomRight.x + 0.55f * textSize.x - 0.05f * talentSize,
                bottomRight.y + 0.55f * textSize.y - 0.05f * talentSize
            ),
            ImColor(textRectColor),
            0,
            0
        );
        drawList->AddRect(
            ImVec2(
                bottomRight.x - 0.65f * textSize.x - 0.05f * talentSize,
                bottomRight.y - 0.65f * textSize.y - 0.05f * talentSize
            ),
            ImVec2(
                bottomRight.x + 0.55f * textSize.x - 0.05f * talentSize,
                bottomRight.y + 0.55f * textSize.y - 0.05f * talentSize
            ),
            ImColor(textRectBorderColor),
            0,
            0
        );
        drawList->AddText(
            ImVec2(
                bottomRight.x - 0.5f * textSize.x - 0.075f * talentSize + textOffset,
                bottomRight.y - 0.5f * textSize.y - 0.075f * talentSize + textOffset
            ),
            ImColor(textColor),
            ("0/" + std::to_string(talent->maxPoints)).c_str()
        );
        ImGui::PopFont();
    }

    void drawLoadoutEditorShapeAroundTalent(
        Engine::Talent_s talent,
        ImDrawList* drawList,
        ImVec4* colors,
        ImVec2 pos,
        int talentSize,
        ImVec2 windowPos,
        ImVec2 scroll,
        UIData& uiData,
        TalentTreeCollection& talentTreeCollection,
        float disabledAlpha,
        bool searchActive,
        bool talentIsSearchedFor)
    {
        ImVec4 borderCol = colors[ImGuiCol_WindowBg];
        ImVec4 textColor = colors[ImGuiCol_TextDisabled];
        ImVec4 textRectColor = colors[ImGuiCol_WindowBg];
        ImVec4 textRectBorderColor = colors[ImGuiCol_Text];
        if (!searchActive) {
            if (disabledAlpha >= 1.0f) {
                if (talent->points == 0) {
                    if (uiData.style == Presets::STYLES::COMPANY_GREY) {
                        borderCol = Presets::TALENT_DEFAULT_BORDER_COLOR_COMPANY_GREY;
                    }
                    else {
                        borderCol = Presets::TALENT_DEFAULT_BORDER_COLOR;
                    }
                }
                else if (talent->points == talent->maxPoints) {
                    borderCol = Presets::TALENT_MAXED_BORDER_COLOR;
                }
                else {
                    borderCol = Presets::TALENT_PARTIAL_BORDER_COLOR;
                }
                textColor = colors[ImGuiCol_Text];
                textRectColor = ImVec4(
                    disabledAlpha * (0.5f * textRectColor.x + 0.5f * borderCol.x),
                    disabledAlpha * (0.5f * textRectColor.y + 0.5f * borderCol.y),
                    disabledAlpha * (0.5f * textRectColor.z + 0.5f * borderCol.z),
                    disabledAlpha * (0.5f * textRectColor.w + 0.5f * borderCol.w)
                );
                textRectBorderColor = ImVec4(
                    disabledAlpha * (0.5f * textRectBorderColor.x + 0.5f * borderCol.x),
                    disabledAlpha * (0.5f * textRectBorderColor.y + 0.5f * borderCol.y),
                    disabledAlpha * (0.5f * textRectBorderColor.z + 0.5f * borderCol.z),
                    disabledAlpha * (0.5f * textRectBorderColor.w + 0.5f * borderCol.w)
                );
            }
            else if (talent->preFilled) {
                borderCol = ImVec4(
                    0.9f * disabledAlpha + colors[ImGuiCol_WindowBg].x * (1.0f - disabledAlpha),
                    0.73f * disabledAlpha + colors[ImGuiCol_WindowBg].x * (1.0f - disabledAlpha),
                    0.0f * disabledAlpha + colors[ImGuiCol_WindowBg].x * (1.0f - disabledAlpha),
                    1.0f);
                textColor = colors[ImGuiCol_Text];
                ImVec4 pBorderCol = Presets::TALENT_MAXED_BORDER_COLOR;
                textRectColor = ImVec4(
                    0.5f * textRectColor.x + 0.5f * pBorderCol.x,
                    0.5f * textRectColor.y + 0.5f * pBorderCol.y,
                    0.5f * textRectColor.z + 0.5f * pBorderCol.z,
                    0.5f * textRectColor.w + 0.5f * pBorderCol.w
                );
                textRectBorderColor = ImVec4(
                    0.5f * textRectBorderColor.x + 0.5f * pBorderCol.x,
                    0.5f * textRectBorderColor.y + 0.5f * pBorderCol.y,
                    0.5f * textRectBorderColor.z + 0.5f * pBorderCol.z,
                    0.5f * textRectBorderColor.w + 0.5f * pBorderCol.w
                );
            }
            else {
                if (uiData.style == Presets::STYLES::COMPANY_GREY) {
                    borderCol = ImVec4(
                        Presets::TALENT_DEFAULT_BORDER_COLOR_COMPANY_GREY.x,
                        Presets::TALENT_DEFAULT_BORDER_COLOR_COMPANY_GREY.y,
                        Presets::TALENT_DEFAULT_BORDER_COLOR_COMPANY_GREY.z,
                        disabledAlpha);
                }
                else {
                    borderCol = ImVec4(
                        Presets::TALENT_DEFAULT_BORDER_COLOR.x,
                        Presets::TALENT_DEFAULT_BORDER_COLOR.y,
                        Presets::TALENT_DEFAULT_BORDER_COLOR.z,
                        disabledAlpha);
                }
                textRectColor = ImVec4(
                    disabledAlpha * (0.5f * textRectColor.x + 0.5f * borderCol.x),
                    disabledAlpha * (0.5f * textRectColor.y + 0.5f * borderCol.y),
                    disabledAlpha * (0.5f * textRectColor.z + 0.5f * borderCol.z),
                    disabledAlpha * (0.5f * textRectColor.w + 0.5f * borderCol.w)
                );
                textRectBorderColor = ImVec4(
                    disabledAlpha * (0.5f * textRectBorderColor.x + 0.5f * borderCol.x),
                    disabledAlpha * (0.5f * textRectBorderColor.y + 0.5f * borderCol.y),
                    disabledAlpha * (0.5f * textRectBorderColor.z + 0.5f * borderCol.z),
                    disabledAlpha * (0.5f * textRectBorderColor.w + 0.5f * borderCol.w)
                );
            }
        }
        else {
            if (talentIsSearchedFor) {
                borderCol = Presets::TALENT_SEARCHED_BORDER_COLOR;
            }
            else {
                if (uiData.style == Presets::STYLES::COMPANY_GREY) {
                    borderCol = Presets::TALENT_DEFAULT_BORDER_COLOR_COMPANY_GREY;
                }
                else {
                    borderCol = Presets::TALENT_DEFAULT_BORDER_COLOR;
                }
            }
            textColor = colors[ImGuiCol_Text];
            textRectColor = ImVec4(
                0.5f * textRectColor.x + 0.5f * borderCol.x,
                0.5f * textRectColor.y + 0.5f * borderCol.y,
                0.5f * textRectColor.z + 0.5f * borderCol.z,
                0.5f * textRectColor.w + 0.5f * borderCol.w
            );
            textRectBorderColor = ImVec4(
                0.5f * textRectBorderColor.x + 0.5f * borderCol.x,
                0.5f * textRectBorderColor.y + 0.5f * borderCol.y,
                0.5f * textRectBorderColor.z + 0.5f * borderCol.z,
                0.5f * textRectBorderColor.w + 0.5f * borderCol.w
            );
        }

        switch (talent->type) {
        case Engine::TalentType::SWITCH: {
            drawList->AddNgonRotated(
                ImVec2(
                    pos.x + talentSize * 0.5f + windowPos.x - scroll.x,
                    pos.y + talentSize * 0.5f + windowPos.y - scroll.y
                ),
                talentSize * 0.65f - 3.5f * uiData.treeEditorZoomFactor,
                ImColor(borderCol),
                8,
                4 * uiData.treeEditorZoomFactor,
                IM_PI / 8.0f
            );
        }break;
        case Engine::TalentType::PASSIVE: {
            drawList->AddCircle(
                ImVec2(
                    pos.x + talentSize * 0.5f + windowPos.x - scroll.x,
                    pos.y + talentSize * 0.5f + windowPos.y - scroll.y
                ),
                talentSize * 0.62f - 3.5f * uiData.treeEditorZoomFactor,
                ImColor(borderCol),
                0,
                4 * uiData.treeEditorZoomFactor
            );
        }break;
        case Engine::TalentType::ACTIVE: {
            drawList->AddRect(
                ImVec2(
                    pos.x + windowPos.x - scroll.x,
                    pos.y + windowPos.y - scroll.y
                ),
                ImVec2(
                    pos.x + talentSize + windowPos.x - scroll.x,
                    pos.y + talentSize + windowPos.y - scroll.y
                ),
                ImColor(borderCol),
                0,
                0,
                4 * uiData.treeEditorZoomFactor
            );
        }break;
        }
        float textOffset = 0;
        if (uiData.treeEditorZoomFactor <= 1.0f) {
            ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[3]);
        }
        else if (uiData.treeEditorZoomFactor < 1.5f) {
            textOffset = 0.01f * talentSize;
            ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[0]);
        }
        else if (uiData.treeEditorZoomFactor < 2.0f) {
            textOffset = 0.015f * talentSize;
            ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
        }
        else {
            textOffset = 0.02f * talentSize;
            ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[2]);
        }
        ImVec2 bottomRight(pos.x + windowPos.x - scroll.x + talentSize, pos.y + windowPos.y - scroll.y + talentSize);
        ImVec2 textSize = ImGui::CalcTextSize("9/9");
        drawList->AddRectFilled(
            ImVec2(
                bottomRight.x - 0.65f * textSize.x - 0.05f * talentSize,
                bottomRight.y - 0.65f * textSize.y - 0.05f * talentSize
            ),
            ImVec2(
                bottomRight.x + 0.55f * textSize.x - 0.05f * talentSize,
                bottomRight.y + 0.55f * textSize.y - 0.05f * talentSize
            ),
            ImColor(textRectColor),
            0,
            0
        );
        drawList->AddRect(
            ImVec2(
                bottomRight.x - 0.65f * textSize.x - 0.05f * talentSize,
                bottomRight.y - 0.65f * textSize.y - 0.05f * talentSize
            ),
            ImVec2(
                bottomRight.x + 0.55f * textSize.x - 0.05f * talentSize,
                bottomRight.y + 0.55f * textSize.y - 0.05f * talentSize
            ),
            ImColor(textRectBorderColor),
            0,
            0
        );
        drawList->AddText(
            ImVec2(
                bottomRight.x - 0.5f * textSize.x - 0.075f * talentSize + textOffset,
                bottomRight.y - 0.5f * textSize.y - 0.075f * talentSize + textOffset
            ),
            ImColor(textColor),
            (std::to_string(talent->points) + "/" + std::to_string(talent->maxPoints)).c_str()
        );
        ImGui::PopFont();
    }

    void drawLoadoutSolverShapeAroundTalent(
        Engine::Talent_s talent,
        ImDrawList* drawList,
        ImVec4* colors,
        ImVec2 pos,
        int talentSize,
        ImVec2 windowPos,
        ImVec2 scroll,
        UIData& uiData,
        TalentTreeCollection& talentTreeCollection,
        std::shared_ptr<Engine::TalentSkillset> skillsetFilter,
        bool searchActive,
        bool talentIsSearchedFor)
    {
        ImVec4 borderCol = colors[ImGuiCol_WindowBg];
        ImVec4 textColor = colors[ImGuiCol_TextDisabled];
        ImVec4 textRectColor = colors[ImGuiCol_WindowBg];
        ImVec4 textRectBorderColor = colors[ImGuiCol_Text];
        if (!searchActive) {
            if (!talent->preFilled) {
                if (talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent->index] == 0) {
                    if (uiData.style == Presets::STYLES::COMPANY_GREY) {
                        borderCol = Presets::TALENT_DEFAULT_BORDER_COLOR_COMPANY_GREY;
                    }
                    else {
                        borderCol = Presets::TALENT_DEFAULT_BORDER_COLOR;
                    }
                }
                else if (talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent->index] == talent->maxPoints) {
                    borderCol = Presets::TALENT_MAXED_BORDER_COLOR;
                }
                else if (talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent->index] == -1) {
                    borderCol = Presets::TALENT_SELECTED_BORDER_COLOR;
                }
                else if (talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent->index] == -2) {
                    borderCol = Presets::TALENT_SEARCHED_BORDER_COLOR;
                }
                else {
                    borderCol = Presets::TALENT_PARTIAL_BORDER_COLOR;
                }
                textColor = colors[ImGuiCol_Text];
                textRectColor = ImVec4(
                    0.5f * textRectColor.x + 0.5f * borderCol.x,
                    0.5f * textRectColor.y + 0.5f * borderCol.y,
                    0.5f * textRectColor.z + 0.5f * borderCol.z,
                    0.5f * textRectColor.w + 0.5f * borderCol.w
                );
                textRectBorderColor = ImVec4(
                    0.5f * textRectBorderColor.x + 0.5f * borderCol.x,
                    0.5f * textRectBorderColor.y + 0.5f * borderCol.y,
                    0.5f * textRectBorderColor.z + 0.5f * borderCol.z,
                    0.5f * textRectBorderColor.w + 0.5f * borderCol.w
                );
            }
            else if (talent->preFilled) {
                borderCol = ImVec4(
                    0.9f * 0.35f + colors[ImGuiCol_WindowBg].x * (1.0f - 0.35f),
                    0.73f * 0.35f + colors[ImGuiCol_WindowBg].x * (1.0f - 0.35f),
                    0.0f * 0.35f + colors[ImGuiCol_WindowBg].x * (1.0f - 0.35f),
                    1.0f);
                textColor = colors[ImGuiCol_Text];
                ImVec4 pBorderCol = Presets::TALENT_MAXED_BORDER_COLOR;
                textRectColor = ImVec4(
                    0.5f * textRectColor.x + 0.5f * pBorderCol.x,
                    0.5f * textRectColor.y + 0.5f * pBorderCol.y,
                    0.5f * textRectColor.z + 0.5f * pBorderCol.z,
                    0.5f * textRectColor.w + 0.5f * pBorderCol.w
                );
                textRectBorderColor = ImVec4(
                    0.5f * textRectBorderColor.x + 0.5f * pBorderCol.x,
                    0.5f * textRectBorderColor.y + 0.5f * pBorderCol.y,
                    0.5f * textRectBorderColor.z + 0.5f * pBorderCol.z,
                    0.5f * textRectBorderColor.w + 0.5f * pBorderCol.w
                );
            }
        }
        else {
            if (talentIsSearchedFor) {
                borderCol = Presets::TALENT_SEARCHED_BORDER_COLOR;
            }
            else {
                if (uiData.style == Presets::STYLES::COMPANY_GREY) {
                    borderCol = Presets::TALENT_DEFAULT_BORDER_COLOR_COMPANY_GREY;
                }
                else {
                    borderCol = Presets::TALENT_DEFAULT_BORDER_COLOR;
                }
            }
            textColor = colors[ImGuiCol_Text];
            textRectColor = ImVec4(
                0.5f * textRectColor.x + 0.5f * borderCol.x,
                0.5f * textRectColor.y + 0.5f * borderCol.y,
                0.5f * textRectColor.z + 0.5f * borderCol.z,
                0.5f * textRectColor.w + 0.5f * borderCol.w
            );
            textRectBorderColor = ImVec4(
                0.5f * textRectBorderColor.x + 0.5f * borderCol.x,
                0.5f * textRectBorderColor.y + 0.5f * borderCol.y,
                0.5f * textRectBorderColor.z + 0.5f * borderCol.z,
                0.5f * textRectBorderColor.w + 0.5f * borderCol.w
            );
        }

        switch (talent->type) {
        case Engine::TalentType::SWITCH: {
            drawList->AddNgonRotated(
                ImVec2(
                    pos.x + talentSize * 0.5f + windowPos.x - scroll.x,
                    pos.y + talentSize * 0.5f + windowPos.y - scroll.y
                ),
                talentSize * 0.65f - 3.5f * uiData.treeEditorZoomFactor,
                ImColor(borderCol),
                8,
                4 * uiData.treeEditorZoomFactor,
                IM_PI / 8.0f
            );
        }break;
        case Engine::TalentType::PASSIVE: {
            drawList->AddCircle(
                ImVec2(
                    pos.x + talentSize * 0.5f + windowPos.x - scroll.x,
                    pos.y + talentSize * 0.5f + windowPos.y - scroll.y
                ),
                talentSize * 0.62f - 3.5f * uiData.treeEditorZoomFactor,
                ImColor(borderCol),
                0,
                4 * uiData.treeEditorZoomFactor
            );
        }break;
        case Engine::TalentType::ACTIVE: {
            drawList->AddRect(
                ImVec2(
                    pos.x + windowPos.x - scroll.x,
                    pos.y + windowPos.y - scroll.y
                ),
                ImVec2(
                    pos.x + talentSize + windowPos.x - scroll.x,
                    pos.y + talentSize + windowPos.y - scroll.y
                ),
                ImColor(borderCol),
                0,
                0,
                4 * uiData.treeEditorZoomFactor
            );
        }break;
        }
        float textOffset = 0;
        if (uiData.treeEditorZoomFactor <= 1.0f) {
            ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[3]);
        }
        else if (uiData.treeEditorZoomFactor < 1.5f) {
            textOffset = 0.01f * talentSize;
            ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[0]);
        }
        else if (uiData.treeEditorZoomFactor < 2.0f) {
            textOffset = 0.015f * talentSize;
            ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
        }
        else {
            textOffset = 0.02f * talentSize;
            ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[2]);
        }
        ImVec2 bottomRight(pos.x + windowPos.x - scroll.x + talentSize, pos.y + windowPos.y - scroll.y + talentSize);
        ImVec2 textSize = ImGui::CalcTextSize("9/9");
        drawList->AddRectFilled(
            ImVec2(
                bottomRight.x - 0.65f * textSize.x - 0.05f * talentSize,
                bottomRight.y - 0.65f * textSize.y - 0.05f * talentSize
            ),
            ImVec2(
                bottomRight.x + 0.55f * textSize.x - 0.05f * talentSize,
                bottomRight.y + 0.55f * textSize.y - 0.05f * talentSize
            ),
            ImColor(textRectColor),
            0,
            0
        );
        drawList->AddRect(
            ImVec2(
                bottomRight.x - 0.65f * textSize.x - 0.05f * talentSize,
                bottomRight.y - 0.65f * textSize.y - 0.05f * talentSize
            ),
            ImVec2(
                bottomRight.x + 0.55f * textSize.x - 0.05f * talentSize,
                bottomRight.y + 0.55f * textSize.y - 0.05f * talentSize
            ),
            ImColor(textRectBorderColor),
            0,
            0
        );
        std::string buttonLabel = "";
        if (talent->preFilled) {
            buttonLabel = std::to_string(talent->maxPoints) + "/" + std::to_string(talent->maxPoints);
        }
        else if (talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent->index] >= 0) {
            buttonLabel = std::to_string(talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent->index]) + "/" + std::to_string(talent->maxPoints);
        }
        else if (talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent->index] == -1) {
            buttonLabel = "X/" + std::to_string(talent->maxPoints);
        }
        else {
            buttonLabel = ">/" + std::to_string(talent->maxPoints);
        }
        drawList->AddText(
            ImVec2(
                bottomRight.x - 0.5f * textSize.x - 0.075f * talentSize + textOffset,
                bottomRight.y - 0.5f * textSize.y - 0.075f * talentSize + textOffset
            ),
            ImColor(textColor),
            (buttonLabel).c_str()
        );
        ImGui::PopFont();
    }

    void clearSolvingProcess(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        uiData.loadoutSolverAllCombinationsAdded = 0;
        uiData.loadoutSolverTalentPointSelection = -1;
        uiData.loadoutSolverSkillsetResultPage = -1;
        uiData.loadoutSolverBufferedPage = -1;
        uiData.selectedFilteredSkillset = 0;
        uiData.selectedFilteredSkillsetIndex = -1;
        uiData.loadoutSolverAutoApplyFilter = false;
        talentTreeCollection.activeTreeData().isTreeSolveProcessed = false;
        talentTreeCollection.activeTreeData().isTreeSolveFiltered = false;
        talentTreeCollection.activeTreeData().skillsetFilter = nullptr;
        talentTreeCollection.activeTreeData().treeDAGInfo = nullptr;
    }

    void AddWrappedText(std::string text, ImVec2 position, float padding, ImVec4 color, float maxWidth, float maxHeight, ImDrawList* draw_list) {
        float padFactor = 2.4f;
        std::vector<std::string> words = Engine::splitString(text, " ");
        std::string renderText = "";
        renderText.reserve(text.size() * 2);
        float spaceWidth = ImGui::CalcTextSize(" ").x;
        float widthLeft = maxWidth - padFactor * padding;
        for (std::string& word : words) {
            float wordWidth = ImGui::CalcTextSize(word.c_str()).x;
            if (wordWidth > maxWidth - padFactor * padding) {
                //not even single word fits in line
                for (const char& ch : word) {
                    float charWidth = ImGui::CalcTextSize(&ch, &ch + 1).x;
                    if (charWidth > widthLeft + padding) {
                        if (ImGui::CalcTextSize((renderText + "\n" + ch).c_str()).y > maxHeight - padFactor * padding) {
                            if (ImGui::CalcTextSize((renderText + "...").c_str()).x <= maxWidth - padFactor * padding) {
                                renderText += "...";
                            }
                            else {
                                renderText = renderText.substr(0, renderText.size() - 3) + "...";
                            }
                            draw_list->AddText(ImVec2(position.x + padding, position.y + padding), ImColor(color), renderText.c_str());
                            return;
                        }
                        renderText += "\n";
                        widthLeft = maxWidth - padFactor * padding;
                    }
                    renderText += ch;
                    widthLeft -= charWidth;
                }
                renderText += " ";
                widthLeft -= spaceWidth;
            }
            else if (wordWidth > widthLeft) {
                //full word doesn't fit on line
                if (ImGui::CalcTextSize((renderText + "\n" + word).c_str()).y > maxHeight - padFactor * padding) {
                    if (ImGui::CalcTextSize((renderText + "...").c_str()).x <= maxWidth - padFactor * padding) {
                        renderText += "...";
                    }
                    else {
                        renderText = renderText.substr(0, renderText.size() - 3) + "...";
                    }
                    break;
                }
                renderText += "\n" + word + " ";
                widthLeft = maxWidth - padFactor * padding - wordWidth + spaceWidth;
            }
            else {
                renderText += word + " ";
                widthLeft -= wordWidth + spaceWidth;
            }
        }
        draw_list->AddText(ImVec2(position.x + padding, position.y + padding), ImColor(color), renderText.c_str());
    }

}