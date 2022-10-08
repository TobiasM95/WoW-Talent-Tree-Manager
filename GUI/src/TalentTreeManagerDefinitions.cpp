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
#include "TTMEnginePresets.h"

#include "imgui_internal.h"

namespace TTM {
    void refreshIconMap(UIData& uiData) {
        std::filesystem::path iconRootPath = Presets::getAppPath() / "resources"/ "icons";
        std::filesystem::path customIconPath = Presets::getAppPath() / "resources" / "icons" / "custom";
        for (auto& [filename, textureInfoPair] : uiData.iconMap) {
            if (textureInfoPair.first.texture) {
                textureInfoPair.first.texture->Release();
                textureInfoPair.first.texture = nullptr;
            }
            if (textureInfoPair.second.texture) {
                textureInfoPair.second.texture->Release();
                textureInfoPair.second.texture = nullptr;
            }
        }
        uiData.iconMap.clear();
        std::map<std::string, std::filesystem::path> iconPathMap;
        //first iterate through pre-shipped directories and add paths to map while skipping custom dir
        if (std::filesystem::is_directory(iconRootPath)) {
            for (auto& entry : std::filesystem::recursive_directory_iterator{ iconRootPath }) {
                if (!std::filesystem::is_regular_file(entry)
                    || entry.path().parent_path().compare(customIconPath) == 0) {
                    continue;
                }
                if (entry.path().extension() == ".png") {
                    iconPathMap[entry.path().filename().string()] = entry.path();
                }
            }
        }
        if (std::filesystem::is_directory(customIconPath)) {
            for (auto& entry : std::filesystem::recursive_directory_iterator{ customIconPath }) {
                if (std::filesystem::is_regular_file(entry) && entry.path().extension() == ".png") {
                    iconPathMap[entry.path().filename().string()] = entry.path();
                }
            }
        }
        else {
            std::filesystem::create_directory(customIconPath);
        }
        for (auto& [filename, path] : iconPathMap) {
            uiData.iconMap[filename] = loadTextureInfoFromFile(uiData, path.string());
        }

        //load default texture first
        int defaultImageWidth = 0;
        int defaultImageHeight = 0;
        bool defaultSuccess = LoadDefaultTexture(&uiData.defaultIcon.texture, &uiData.defaultIconGray.texture, &defaultImageWidth, &defaultImageHeight, uiData.g_pd3dDevice, Engine::TalentType::ACTIVE);
        bool maskSuccess = true;
        bool redGlowSuccess = true;
        bool greenGlowSuccess = true;
        bool goldGlowSuccess = true;
        bool blueGlowSuccess = true;
        bool purpleGlowSuccess = true;
        for (int talentType = 0; talentType < 3; talentType++) {
            redGlowSuccess = LoadIconGlowTexture(
                &uiData.redIconGlow[talentType].texture,
                &uiData.redIconGlow[talentType].width,
                &uiData.redIconGlow[talentType].height,
                uiData.g_pd3dDevice,
                static_cast<Engine::TalentType>(talentType),
                1.0f, 0.0f, 0.0f);
            greenGlowSuccess = LoadIconGlowTexture(
                &uiData.greenIconGlow[talentType].texture,
                &uiData.greenIconGlow[talentType].width,
                &uiData.greenIconGlow[talentType].height,
                uiData.g_pd3dDevice,
                static_cast<Engine::TalentType>(talentType),
                0.0f, 1.0f, 0.0f);
            goldGlowSuccess = LoadIconGlowTexture(
                &uiData.goldIconGlow[talentType].texture,
                &uiData.goldIconGlow[talentType].width,
                &uiData.goldIconGlow[talentType].height,
                uiData.g_pd3dDevice,
                static_cast<Engine::TalentType>(talentType),
                0.8f, 0.63f, 0.0f);
            blueGlowSuccess = LoadIconGlowTexture(
                &uiData.blueIconGlow[talentType].texture,
                &uiData.blueIconGlow[talentType].width,
                &uiData.blueIconGlow[talentType].height,
                uiData.g_pd3dDevice,
                static_cast<Engine::TalentType>(talentType),
                0.0f, 0.73f, 1.0f);
            purpleGlowSuccess = LoadIconGlowTexture(
                &uiData.purpleIconGlow[talentType].texture,
                &uiData.purpleIconGlow[talentType].width,
                &uiData.purpleIconGlow[talentType].height,
                uiData.g_pd3dDevice,
                static_cast<Engine::TalentType>(talentType),
                0.73f, 0.0f, 1.0f);

            int maskWidth = 0;
            int maskHeight = 0;
            //load company grey mask
            maskSuccess = LoadIconMaskTexture(
                &uiData.talentIconMasks[static_cast<int>(Presets::STYLES::COMPANY_GREY)][talentType].texture,
                &maskWidth,
                &maskHeight,
                uiData.g_pd3dDevice,
                static_cast<Engine::TalentType>(talentType),
                0.25f, 0.25f, 0.25f);
            //load path of talent tree mask
            maskSuccess = LoadIconMaskTexture(
                &uiData.talentIconMasks[static_cast<int>(Presets::STYLES::PATH_OF_TALENT_TREE)][talentType].texture,
                &maskWidth,
                &maskHeight,
                uiData.g_pd3dDevice,
                static_cast<Engine::TalentType>(talentType),
                0.0f, 0.0f, 0.0f);
            //load light mode mask
            maskSuccess = LoadIconMaskTexture(
                &uiData.talentIconMasks[static_cast<int>(Presets::STYLES::LIGHT_MODE)][talentType].texture,
                &maskWidth,
                &maskHeight,
                uiData.g_pd3dDevice,
                static_cast<Engine::TalentType>(talentType),
                0.9412f, 0.9412f, 0.9412f);
        }
        if (!(defaultSuccess && redGlowSuccess && greenGlowSuccess && goldGlowSuccess && blueGlowSuccess && purpleGlowSuccess && maskSuccess)) {
            //TTMNOTE: this should not happen anymore
            throw std::runtime_error("Cannot create default icon or icon glows!");
        }
    }

    void loadActiveIcons(UIData& uiData, TalentTreeCollection& talentTreeCollection, bool forceReload) {
        if (talentTreeCollection.activeTreeIndex == -1 || (!forceReload && uiData.loadedIconTreeIndex == talentTreeCollection.activeTreeIndex)) {
            return;
        }
        uiData.iconIndexMap.clear();
        uiData.iconIndexMapGrayed.clear();

        for (auto& talent : talentTreeCollection.activeTree().orderedTalents) {
            std::pair<TextureInfo*, TextureInfo*> talentIconPtrs = { nullptr, nullptr };
            std::pair<TextureInfo*, TextureInfo*> talentIconGrayedPtrs = { nullptr, nullptr };
            if (uiData.iconMap.count(talent.second->iconName.first)) {
                talentIconPtrs.first = &uiData.iconMap[talent.second->iconName.first].first;
                talentIconGrayedPtrs.first = &uiData.iconMap[talent.second->iconName.first].second;
            }
            else {
                talentIconPtrs.first = &uiData.defaultIcon;
                talentIconGrayedPtrs.first = &uiData.defaultIconGray;
            }
            if (uiData.iconMap.count(talent.second->iconName.second)) {
                talentIconPtrs.second = &uiData.iconMap[talent.second->iconName.second].first;
                talentIconGrayedPtrs.second = &uiData.iconMap[talent.second->iconName.second].second;
            }
            else {
                talentIconPtrs.second = &uiData.defaultIcon;
                talentIconGrayedPtrs.second = &uiData.defaultIconGray;
            }
            uiData.iconIndexMap[talent.first] = talentIconPtrs;
            uiData.iconIndexMapGrayed[talent.first] = talentIconGrayedPtrs;
        }

        uiData.loadedIconTreeIndex = talentTreeCollection.activeTreeIndex;
    }

    /*
    void loadIcon(UIData& uiData, int index, std::string iconName, ID3D11ShaderResourceView* defaultTexture, ID3D11ShaderResourceView* defaultTextureGray, int defaultImageWidth, int defaultImageHeight, bool first, Engine::TalentType talentType) {
        int width = 0;
        int height = 0;
        ID3D11ShaderResourceView* texture = NULL;
        ID3D11ShaderResourceView* textureGray = NULL;
        std::string path;
        if (uiData.iconPathMap.count(iconName)) {
            path = uiData.iconPathMap[iconName].string();
            bool ret = LoadTextureFromFile(path.c_str(), &texture, &textureGray, &width, &height, uiData.g_pd3dDevice, talentType);
            if (!ret) {
                TextureInfo textureInfo;
                textureInfo.texture = defaultTexture;
                textureInfo.width = defaultImageWidth;
                textureInfo.height = defaultImageHeight;
                TextureInfo textureInfoGray;
                textureInfoGray.texture = defaultTextureGray;
                textureInfoGray.width = defaultImageWidth;
                textureInfoGray.height = defaultImageHeight;
                if (first) {
                    uiData.iconIndexMap[index].first = textureInfo;
                    uiData.iconIndexMapGrayed[index].first = textureInfoGray;
                }
                else {
                    uiData.iconIndexMap[index].second = textureInfo;
                    uiData.iconIndexMapGrayed[index].second = textureInfoGray;
                }
            }
            else {
                TextureInfo textureInfo;
                textureInfo.texture = texture;
                textureInfo.width = width;
                textureInfo.height = height;
                TextureInfo textureInfoGray;
                textureInfoGray.texture = textureGray;
                textureInfoGray.width = width;
                textureInfoGray.height = height;
                if (first) {
                    uiData.iconIndexMap[index].first = textureInfo;
                    uiData.iconIndexMapGrayed[index].first = textureInfoGray;
                }
                else {
                    uiData.iconIndexMap[index].second = textureInfo;
                    uiData.iconIndexMapGrayed[index].second = textureInfoGray;
                }
            }
        }
        else {
            TextureInfo textureInfo;
            textureInfo.texture = defaultTexture;
            textureInfo.width = defaultImageWidth;
            textureInfo.height = defaultImageHeight;
            TextureInfo textureInfoGray;
            textureInfoGray.texture = defaultTextureGray;
            textureInfoGray.width = defaultImageWidth;
            textureInfoGray.height = defaultImageHeight;
            if (first) {
                uiData.iconIndexMap[index].first = textureInfo;
                uiData.iconIndexMapGrayed[index].first = textureInfoGray;
            }
            else {
                uiData.iconIndexMap[index].second = textureInfo;
                uiData.iconIndexMapGrayed[index].second= textureInfoGray;
            }
        }
    }

    void loadSplitIcon(UIData& uiData, Engine::Talent_s talent, ID3D11ShaderResourceView* defaultTexture, ID3D11ShaderResourceView* defaultTextureGray, int defaultImageWidth, int defaultImageHeight) {
        int width = 0;
        int height = 0;
        ID3D11ShaderResourceView* texture = NULL;
        ID3D11ShaderResourceView* textureGray = NULL;
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
        bool ret = LoadSplitTextureFromFile(path1.c_str(), path2.c_str(), &texture, &textureGray, &width, &height, uiData.g_pd3dDevice);
        if (!ret) {
            TextureInfo textureInfo;
            textureInfo.texture = defaultTexture;
            textureInfo.width = defaultImageWidth;
            textureInfo.height = defaultImageHeight;
            TextureInfo textureInfoGray;
            textureInfoGray.texture = defaultTextureGray;
            textureInfoGray.width = defaultImageWidth;
            textureInfoGray.height = defaultImageHeight;
            uiData.splitIconIndexMap[talent->index] = textureInfo;
            uiData.splitIconIndexMapGrayed[talent->index] = textureInfoGray;
        }
        else {
            TextureInfo textureInfo;
            textureInfo.texture = texture;
            textureInfo.width = width;
            textureInfo.height = height;
            TextureInfo textureInfoGray;
            textureInfoGray.texture = textureGray;
            textureInfoGray.width = width;
            textureInfoGray.height = height;
            uiData.splitIconIndexMap[talent->index] = textureInfo;
            uiData.splitIconIndexMapGrayed[talent->index] = textureInfoGray;
        }
    }
    */

    std::pair<TextureInfo, TextureInfo> loadTextureInfoFromFile(UIData& uiData, std::string path) {
        std::pair<TextureInfo, TextureInfo> textureInfoPair;
        int width = 0;
        int height = 0;
        ID3D11ShaderResourceView* texture = NULL;
        ID3D11ShaderResourceView* textureGray = NULL;
        bool ret = LoadTextureFromFile(path.c_str(), &texture, &textureGray, &width, &height, uiData.g_pd3dDevice, Engine::TalentType::ACTIVE);
        if (!ret) {
            //load default texture first
            int defaultImageWidth = 0;
            int defaultImageHeight = 0;
            ID3D11ShaderResourceView* defaultTexture = NULL;
            ID3D11ShaderResourceView* defaultTextureGray = NULL;
            bool ret = LoadDefaultTexture(&defaultTexture, &defaultTextureGray, &defaultImageWidth, &defaultImageHeight, uiData.g_pd3dDevice, Engine::TalentType::ACTIVE);
            if (!ret) {
                //TTMNOTE: This should not happen anymore
                throw std::runtime_error("Cannot create default icon!");
            }
            textureInfoPair.first.texture = defaultTexture;
            textureInfoPair.first.width = defaultImageWidth;
            textureInfoPair.first.height = defaultImageHeight;

            textureInfoPair.second.texture = defaultTextureGray;
            textureInfoPair.second.width = defaultImageWidth;
            textureInfoPair.second.height = defaultImageHeight;
        }
        else {
            textureInfoPair.first.texture = texture;
            textureInfoPair.first.width = width;
            textureInfoPair.first.height = height;

            textureInfoPair.second.texture = textureGray;
            textureInfoPair.second.width = width;
            textureInfoPair.second.height = height;
        }
        return textureInfoPair;
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

    void drawPreviewArrowBetweenTalents(
        Engine::Talent_s t1,
        Engine::Talent_s t2,
        int pointsSpent1,
        int pointsSpent2,
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
            if (pointsSpent2 > 0 && pointsSpent1 >= t1->maxPoints) {
                if (pointsSpent2 >= t2->maxPoints) {
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
            Presets::PUSH_FONT(uiData.fontsize, 3);
        }
        else if (uiData.treeEditorZoomFactor < 1.5f) {
            textOffset = 0.01f * talentSize;
            Presets::PUSH_FONT(uiData.fontsize, 0);
        }
        else if (uiData.treeEditorZoomFactor < 2.0f) {
            textOffset = 0.015f * talentSize;
            Presets::PUSH_FONT(uiData.fontsize, 1);
        }
        else {
            textOffset = 0.02f * talentSize;
            Presets::PUSH_FONT(uiData.fontsize, 2);
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
        Presets::POP_FONT();
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
            Presets::PUSH_FONT(uiData.fontsize, 3);
        }
        else if (uiData.treeEditorZoomFactor < 1.5f) {
            textOffset = 0.01f * talentSize;
            Presets::PUSH_FONT(uiData.fontsize, 0);
        }
        else if (uiData.treeEditorZoomFactor < 2.0f) {
            textOffset = 0.015f * talentSize;
            Presets::PUSH_FONT(uiData.fontsize, 1);
        }
        else {
            textOffset = 0.02f * talentSize;
            Presets::PUSH_FONT(uiData.fontsize, 2);
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
        Presets::POP_FONT();
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
                else if (talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent->index] == -3) {
                    borderCol = Presets::TALENT_PURPLE_BORDER_COLOR;
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
            Presets::PUSH_FONT(uiData.fontsize, 3);
        }
        else if (uiData.treeEditorZoomFactor < 1.5f) {
            textOffset = 0.01f * talentSize;
            Presets::PUSH_FONT(uiData.fontsize, 0);
        }
        else if (uiData.treeEditorZoomFactor < 2.0f) {
            textOffset = 0.015f * talentSize;
            Presets::PUSH_FONT(uiData.fontsize, 1);
        }
        else {
            textOffset = 0.02f * talentSize;
            Presets::PUSH_FONT(uiData.fontsize, 2);
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
        else if (talentTreeCollection.activeTreeData().skillsetFilter->assignedSkillPoints[talent->index] == -2) {
            buttonLabel = ">/" + std::to_string(talent->maxPoints);
        }
        else {
            buttonLabel = "=/" + std::to_string(talent->maxPoints);
        }
        drawList->AddText(
            ImVec2(
                bottomRight.x - 0.5f * textSize.x - 0.075f * talentSize + textOffset,
                bottomRight.y - 0.5f * textSize.y - 0.075f * talentSize + textOffset
            ),
            ImColor(textColor),
            (buttonLabel).c_str()
        );
        Presets::POP_FONT();
    }

    void drawSkillsetPreviewShapeAroundTalent(
        Engine::Talent_s talent,
        int pointsSpent,
        ImDrawList* drawList,
        ImVec4* colors,
        ImVec2 pos,
        int talentSize,
        ImVec2 windowPos,
        ImVec2 scroll,
        UIData& uiData,
        TalentTreeCollection& talentTreeCollection,
        float disabledAlpha)
    {
        ImVec4 borderCol = colors[ImGuiCol_WindowBg];
        ImVec4 textColor = colors[ImGuiCol_TextDisabled];
        ImVec4 textRectColor = colors[ImGuiCol_WindowBg];
        ImVec4 textRectBorderColor = colors[ImGuiCol_Text];
        if (disabledAlpha >= 1.0f) {
            if (pointsSpent == 0) {
                if (uiData.style == Presets::STYLES::COMPANY_GREY) {
                    borderCol = Presets::TALENT_DEFAULT_BORDER_COLOR_COMPANY_GREY;
                }
                else {
                    borderCol = Presets::TALENT_DEFAULT_BORDER_COLOR;
                }
            }
            else if (pointsSpent >= talent->maxPoints) {
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
            Presets::PUSH_FONT(uiData.fontsize, 3);
        }
        else if (uiData.treeEditorZoomFactor < 1.5f) {
            textOffset = 0.01f * talentSize;
            Presets::PUSH_FONT(uiData.fontsize, 0);
        }
        else if (uiData.treeEditorZoomFactor < 2.0f) {
            textOffset = 0.015f * talentSize;
            Presets::PUSH_FONT(uiData.fontsize, 1);
        }
        else {
            textOffset = 0.02f * talentSize;
            Presets::PUSH_FONT(uiData.fontsize, 2);
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
        int netPointsSpent = (talent->type == Engine::TalentType::SWITCH && pointsSpent > 0) ? 1 : pointsSpent;
        drawList->AddText(
            ImVec2(
                bottomRight.x - 0.5f * textSize.x - 0.075f * talentSize + textOffset,
                bottomRight.y - 0.5f * textSize.y - 0.075f * talentSize + textOffset
            ),
            ImColor(textColor),
            (std::to_string(netPointsSpent) + "/" + std::to_string(talent->maxPoints)).c_str()
        );
        Presets::POP_FONT();
    }

    void clearTextboxes(UIData& uiData) {
        uiData.treeEditorCreationIconNameFilter = "";
        uiData.treeEditorEditIconNameFilter = "";
        uiData.treeEditorImportTreeString = "";
        uiData.treeEditorExportTreeString = "";
        uiData.treeEditorPastebinExportTreeString = "";
        uiData.treeEditorReadableExportTreeString = "";
        uiData.loadoutEditorExportActiveSkillsetString = "";
        uiData.loadoutEditorExportAllSkillsetsString = "";
        uiData.loadoutEditorExportActiveSkillsetSimcString = "";
        uiData.loadoutEditorExportAllSkillsetsSimcString = "";
        uiData.loadoutEditorImportSkillsetsString = "";
        uiData.loadoutSolverSkillsetPrefix = "";
        uiData.raidbotsInputURL = "";
        uiData.simAnalysisSingleTalentExportString = "";
    }

    void updateSolverStatus(UIData& uiData, TalentTreeCollection& talentTreeCollection, bool forceUpdate) {
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - uiData.currentSolversLastUpdateTime);
        if (milliseconds > uiData.currentSolversUpdateInterval || forceUpdate) {
            uiData.currentSolversLastUpdateTime = std::chrono::steady_clock::now();
            uiData.currentSolvers.clear();
            uiData.solvedTrees.clear();
            for (auto& talentTreeData : talentTreeCollection.trees) {
                if (talentTreeData.isTreeSolveInProgress) {
                    uiData.currentSolvers.emplace_back(
                            talentTreeData.tree.name,
                            &talentTreeData
                        );
                }
                else if (talentTreeData.treeDAGInfo) {
                    uiData.solvedTrees.emplace_back(
                        talentTreeData.tree.name,
                        &talentTreeData
                    );
                }
            }
        }
    }

    void stopAllSolvers(TalentTreeCollection& talentTreeCollection) {
        for (auto& talentTreeData : talentTreeCollection.trees) {
            if (talentTreeData.isTreeSolveInProgress) {
                talentTreeData.safetyGuardTriggered = true;
            }
        }
    }

    void drawSimAnalysisShapeAroundTalent(
        Engine::Talent_s talent,
        ImDrawList* drawList,
        ImVec4* colors,
        ImVec2 pos,
        int talentSize,
        ImVec2 windowPos,
        ImVec2 scroll,
        UIData& uiData,
        TalentTreeCollection& talentTreeCollection,
        bool searchActive,
        bool talentIsSearchedFor)
    {
        ImVec4 borderCol = colors[ImGuiCol_WindowBg];
        ImVec4 textColor = colors[ImGuiCol_TextDisabled];
        ImVec4 textRectColor = colors[ImGuiCol_WindowBg];
        ImVec4 textRectBorderColor = colors[ImGuiCol_Text];
        if (!searchActive) {
            if (!talentTreeCollection.activeTreeData().simAnalysisTalentColor.count(talent->index)) {
                if (uiData.style == Presets::STYLES::COMPANY_GREY) {
                    borderCol = Presets::TALENT_DEFAULT_BORDER_COLOR_COMPANY_GREY;
                }
                else {
                    borderCol = Presets::TALENT_DEFAULT_BORDER_COLOR;
                }
            }
            else {
                borderCol = talentTreeCollection.activeTreeData().simAnalysisTalentColor[talent->index];
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
            Presets::PUSH_FONT(uiData.fontsize, 3);
        }
        else if (uiData.treeEditorZoomFactor < 1.5f) {
            textOffset = 0.01f * talentSize;
            Presets::PUSH_FONT(uiData.fontsize, 0);
        }
        else if (uiData.treeEditorZoomFactor < 2.0f) {
            textOffset = 0.015f * talentSize;
            Presets::PUSH_FONT(uiData.fontsize, 1);
        }
        else {
            textOffset = 0.02f * talentSize;
            Presets::PUSH_FONT(uiData.fontsize, 2);
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
        Presets::POP_FONT();
    }

    void drawSkillsetPreview(UIData& uiData, TalentTreeCollection& talentTreeCollection, std::shared_ptr<Engine::TalentSkillset> skillset) {
        Engine::TalentTree& tree = talentTreeCollection.activeTree();

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
                drawPreviewArrowBetweenTalents(
                    talent.second,
                    child,
                    skillset->assignedSkillPoints[talent.first],
                    skillset->assignedSkillPoints[child->index],
                    drawList,
                    windowPos,
                    scrollOffset,
                    origin,
                    talentHalfSpacing,
                    talentSize,
                    0.0f,
                    uiData,
                    true);
            }
        }
        for (auto& reqInfo : tree.requirementSeparatorInfo) {
            ImVec4 separatorColor = Presets::GET_TOOLTIP_TALENT_DESC_COLOR(uiData.style);
            if (skillset->talentPointsSpent - tree.preFilledTalentPoints >= reqInfo.first) {
                separatorColor.w = 0.4f;
            }
            drawList->AddLine(
                ImVec2(windowPos.x - scrollOffset.x + origin.x - 2 * talentSize, windowPos.y - scrollOffset.y + talentWindowPaddingY + (reqInfo.second - 1) * talentSize),
                ImVec2(windowPos.x - scrollOffset.x + origin.x + (tree.maxCol + 1) * talentSize, windowPos.y - scrollOffset.y + talentWindowPaddingY + (reqInfo.second - 1) * talentSize),
                ImColor(separatorColor),
                2.0f
            );
            drawList->AddText(
                ImVec2(windowPos.x - scrollOffset.x + origin.x - 2 * talentSize, windowPos.y - scrollOffset.y + talentWindowPaddingY + (reqInfo.second - 1) * talentSize),
                ImColor(separatorColor),
                (std::to_string(skillset->talentPointsSpent - tree.preFilledTalentPoints) + " / " + std::to_string(reqInfo.first) + " points").c_str()
            );
        }

        for (auto& talent : tree.orderedTalents) {
            maxRow = talent.second->row > maxRow ? talent.second->row : maxRow;
            float posX = origin.x + (talent.second->column - 1) * 2 * talentHalfSpacing;
            float posY = origin.y + (talent.second->row - 1) * 2 * talentHalfSpacing;
            bool talentDisabled = skillset->assignedSkillPoints[talent.first] == 0;

            ImGui::SetCursorPos(ImVec2(posX, posY));
            if (talentDisabled) {
                ImGui::GetCurrentContext()->Style.DisabledAlpha = 0.20f;
                ImGui::BeginDisabled();
            }

            ImGui::PushID((std::to_string(talent.second->points) + "/" + std::to_string(talent.second->maxPoints) + "##" + std::to_string(talent.second->index)).c_str());
            TextureInfo* iconContent = nullptr;
            TextureInfo* iconContentChoice = nullptr;
            if (talent.second->type == Engine::TalentType::SWITCH) {
                if (skillset->assignedSkillPoints[talent.first] > 0) {
                    if (skillset->assignedSkillPoints[talent.first] == 2) {
                        iconContent = uiData.iconIndexMap[talent.second->index].second;
                    }
                    else {
                        iconContent = uiData.iconIndexMap[talent.second->index].first;
                    }
                }
                else {
                    iconContent = uiData.iconIndexMapGrayed[talent.second->index].first;
                    iconContentChoice = uiData.iconIndexMapGrayed[talent.second->index].second;
                }
            }
            else {
                if (skillset->assignedSkillPoints[talent.first] > 0) {
                    iconContent = uiData.iconIndexMap[talent.second->index].first;
                }
                else {
                    iconContent = uiData.iconIndexMapGrayed[talent.second->index].first;
                }
            }
            ImGui::PopID();
            if (talent.second->type != Engine::TalentType::SWITCH || skillset->assignedSkillPoints[talent.first] > 0) {
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

            //mask has to be displayed without alpha
            ImGui::SetCursorPos(ImVec2(posX, posY));
            if (talentDisabled) {
                ImGui::GetCurrentContext()->Style.DisabledAlpha = 0.6f;
                ImGui::EndDisabled();
            }
            ImGui::Image(
                uiData.talentIconMasks[static_cast<int>(uiData.style)][static_cast<int>(talent.second->type)].texture,
                ImVec2(static_cast<float>(talentSize), static_cast<float>(talentSize)), ImVec2(0, 0), ImVec2(1, 1)
            );
            ImGui::SetCursorPos(ImVec2(
                posX - 0.5f * (uiData.treeEditorZoomFactor * uiData.redIconGlow[static_cast<int>(talent.second->type)].width - talentSize),
                posY - 0.5f * (uiData.treeEditorZoomFactor * uiData.redIconGlow[static_cast<int>(talent.second->type)].height - talentSize)));
            if (uiData.enableGlow) {
                if (skillset->assignedSkillPoints[talent.first] == talent.second->maxPoints) {
                    ImGui::Image(
                        uiData.goldIconGlow[static_cast<int>(talent.second->type)].texture,
                        ImVec2(
                            uiData.treeEditorZoomFactor * uiData.goldIconGlow[static_cast<int>(talent.second->type)].width,
                            uiData.treeEditorZoomFactor * uiData.goldIconGlow[static_cast<int>(talent.second->type)].height),
                        ImVec2(0, 0), ImVec2(1, 1),
                        ImVec4(1, 1, 1, 1.0f - 0.5f * (uiData.style == Presets::STYLES::COMPANY_GREY))
                    );
                }
                else if (skillset->assignedSkillPoints[talent.first] > 0) {
                    ImGui::Image(
                        uiData.greenIconGlow[static_cast<int>(talent.second->type)].texture,
                        ImVec2(
                            uiData.treeEditorZoomFactor * uiData.greenIconGlow[static_cast<int>(talent.second->type)].width,
                            uiData.treeEditorZoomFactor * uiData.greenIconGlow[static_cast<int>(talent.second->type)].height),
                        ImVec2(0, 0), ImVec2(1, 1),
                        ImVec4(1, 1, 1, 1.0f - 0.5f * (uiData.style == Presets::STYLES::COMPANY_GREY))
                    );
                }
            }
            if (talentDisabled) {
                ImGui::GetCurrentContext()->Style.DisabledAlpha = 0.20f;
                ImGui::BeginDisabled();
            }

            drawSkillsetPreviewShapeAroundTalent(
                talent.second,
                skillset->assignedSkillPoints[talent.first],
                drawList,
                imStyle.Colors,
                ImVec2(posX, posY),
                talentSize,
                ImGui::GetWindowPos(),
                ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY()),
                uiData,
                talentTreeCollection,
                1.0f - 0.8f * talentDisabled);
            
            if (talentDisabled) {
                ImGui::GetCurrentContext()->Style.DisabledAlpha = 0.6f;
                ImGui::EndDisabled();
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

    void clearSolvingProcess(UIData& uiData, TalentTreeCollection& talentTreeCollection, bool onlyUIData) {
        uiData.loadoutSolverTalentPointSelection = -1;
        uiData.loadoutSolverSkillsetResultPage = -1;
        uiData.loadoutSolverBufferedPage = -1;
        uiData.selectedFilteredSkillset = 0;
        uiData.selectedFilteredSkillsetIndex = -1;
        uiData.loadoutSolverAutoApplyFilter = false;
        if (onlyUIData) {
            return;
        }

        talentTreeCollection.activeTreeData().isTreeSolveProcessed = false;
        talentTreeCollection.activeTreeData().isTreeSolveFiltered = false;
        talentTreeCollection.activeTreeData().safetyGuardTriggered = false;
        talentTreeCollection.activeTreeData().isTreeSolveInProgress = false;
        talentTreeCollection.activeTreeData().skillsetFilter = nullptr;
        talentTreeCollection.activeTreeData().treeDAGInfo = nullptr;
    }

    void clearSolvingProcess(UIData& uiData, TalentTreeData& talentTreeData) {
        uiData.loadoutSolverTalentPointSelection = -1;
        uiData.loadoutSolverSkillsetResultPage = -1;
        uiData.loadoutSolverBufferedPage = -1;
        uiData.selectedFilteredSkillset = 0;
        uiData.selectedFilteredSkillsetIndex = -1;
        uiData.loadoutSolverAutoApplyFilter = false;
        talentTreeData.isTreeSolveProcessed = false;
        talentTreeData.isTreeSolveFiltered = false;
        talentTreeData.safetyGuardTriggered = false;
        talentTreeData.isTreeSolveInProgress = false;
        talentTreeData.skillsetFilter = nullptr;
        talentTreeData.treeDAGInfo = nullptr;
    }

    void clearSimAnalysisProcess(UIData& uiData, TalentTreeCollection& talentTreeCollection, bool onlyUIData) {
        uiData.simAnalysisPage = SimAnalysisPage::Settings;
        uiData.raidbotsInputURL = "";
        uiData.analysisTooltipLastTalentIndex = -1;
        uiData.analysisTooltipTalentRank = -1;
        uiData.analysisBreakdownTalentIndex = -1;
        if (onlyUIData) {
            return;
        }

        talentTreeCollection.activeTree().analysisResult = Engine::AnalysisResult();
        talentTreeCollection.activeTree().selectedSimAnalysisRawResult = -1;
        talentTreeCollection.activeTree().simAnalysisRawResults.clear();
        talentTreeCollection.activeTreeData().simAnalysisTalentColor.clear();
        talentTreeCollection.activeTreeData().simAnalysisButtonRankingText.clear();
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

    void HelperTooltip(std::string hovered, std::string helptext) {
        ImGui::TextUnformattedColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), hovered.c_str());
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 15.0f);
            ImGui::TextUnformatted(helptext.c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
}