#include "TalentTreeManagerDefinitions.h"

#include "imgui_internal.h"

namespace TTM {
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
        UIData& uiData)
    {
        //Arrow constants
        float thickness = 2.0f * uiData.treeEditorZoomFactor;
        float relArrowSpace = 0.15f; //how much space should be between arrow and connecting talents in terms of relative to talentSize
        float relArrowHeadSize = 0.15f; //how long should each side of the the arrow head triangle be in terms of relative to talentSize
        float relArrowHeadAngle = 2.1f;
        ImU32 color = ImColor(185, 166, 72, 255);

        float p1X, p1Y, p2X, p2Y;
        if (t1->column < t2->column) {
            if (t1->row < t2->row) {
                //Arrow to the bottom right
                p1X = talentWindowPadding.x + (t1->column - 1) * 2 * talentHalfSpacing + talentPadding + talentSize;
                p1Y = talentWindowPadding.y + (t1->row - 1) * 2 * talentHalfSpacing + talentPadding + talentSize;
                p2X = talentWindowPadding.x + (t2->column - 1) * 2 * talentHalfSpacing + talentPadding;
                p2Y = talentWindowPadding.y + (t2->row - 1) * 2 * talentHalfSpacing + talentPadding;
            }
            else if (t1->row == t2->row) {
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
        else if (t1->column == t2->column) {
            if (t1->row < t2->row) {
                //Arrow straight down
                p1X = talentWindowPadding.x + (t1->column - 1) * 2 * talentHalfSpacing + talentPadding + 0.5f * talentSize;
                p1Y = talentWindowPadding.y + (t1->row - 1) * 2 * talentHalfSpacing + talentPadding + talentSize;
                p2X = talentWindowPadding.x + (t2->column - 1) * 2 * talentHalfSpacing + talentPadding + 0.5f * talentSize;
                p2Y = talentWindowPadding.y + (t2->row - 1) * 2 * talentHalfSpacing + talentPadding;
            }
            else if (t1->row == t2->row) {
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
            if (t1->row < t2->row) {
                //Arrow to the bottom left
                p1X = talentWindowPadding.x + (t1->column - 1) * 2 * talentHalfSpacing + talentPadding;
                p1Y = talentWindowPadding.y + (t1->row - 1) * 2 * talentHalfSpacing + talentPadding + talentSize;
                p2X = talentWindowPadding.x + (t2->column - 1) * 2 * talentHalfSpacing + talentPadding + talentSize;
                p2Y = talentWindowPadding.y + (t2->row - 1) * 2 * talentHalfSpacing + talentPadding;
            }
            else if (t1->row == t2->row) {
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

    void clearSolvingProcess(UIData& uiData, TalentTreeCollection& talentTreeCollection) {
        uiData.loadoutSolverAllCombinationsAdded = 0;
        uiData.loadoutSolverTalentPointSelection = -1;
        uiData.loadoutSolverSkillsetResultPage = -1;
        uiData.loadoutSolverBufferedPage = -1;
        uiData.selectedFilteredSkillset = 0;
        uiData.selectedFilteredSkillsetIndex = -1;
        talentTreeCollection.activeTreeData().isTreeSolveProcessed = false;
        talentTreeCollection.activeTreeData().isTreeSolveFiltered = false;
        talentTreeCollection.activeTreeData().skillsetFilter = nullptr;
        talentTreeCollection.activeTreeData().treeDAGInfo = nullptr;
    }
}