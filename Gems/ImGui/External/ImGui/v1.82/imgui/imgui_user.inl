/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "imgui_user.h"

#define IMGUI_USER_TEXT_MASK 0x55000000

int EditorCurveFunction(ImGui::CurveGraphDataParams& dataParams, ImGui::CurveEditorWindowParams& windowParams)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(windowParams.padding, windowParams.padding));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, windowParams.edgeColor); // child window / edge of graph colour
    ImGui::PushStyleColor(ImGuiCol_FrameBg, windowParams.backgroundColor); // graph background colour
    return 0;
}

// take the minimum position for X and Y and then adding the proportional increase in the maximum range based on the point values
// converting from actual point value to screen space
ImVec2 ImGui::TransformToEditorSpace(const ImVec2& pos, ImGui::CurveGraphDataParams& dataParams)
{

    // moved this as the ImRect type is stored in imgui_internal not available in the imgui.h and hence chicken and egg struct definition
    ImGuiWindow* window = GetCurrentWindow();
    const ImRect innerBB = window->InnerRect;

    float xProportion = (pos.x - dataParams.minPoint.x) / dataParams.width;
    float yProportion = (pos.y - dataParams.minPoint.y) / dataParams.height;

    float rangeX = innerBB.Max.x - innerBB.Min.x;
    float rangeY = innerBB.Max.y - innerBB.Min.y;

    float resultX = (innerBB.Min.x) + (rangeX * xProportion);
    float resultY = (innerBB.Max.y) - (rangeY * yProportion);

    return ImVec2(resultX, resultY);
}

// this is converting a point in screen space back to actual point value.
ImVec2 ImGui::InverseTransformToEditorSpace(const ImVec2& pos, ImGui::CurveGraphDataParams& dataParams)
{

    ImGuiWindow* window = GetCurrentWindow();
    const ImRect innerBB = window->InnerRect;

    float x = (pos.x - innerBB.Min.x) / (innerBB.Max.x - innerBB.Min.x);
    float y = (innerBB.Max.y - pos.y) / (innerBB.Max.y - innerBB.Min.y);

    float resultX = dataParams.minPoint.x + dataParams.width * x;
    float resultY = dataParams.minPoint.y + dataParams.height * y;

    return ImVec2(resultX, resultY);
};

bool EditorLinearPoint(ImVec2& p, ImGui::CurveGraphDataParams& dataParams)
{
    using namespace ImGui;

    ImGuiWindow* window = GetCurrentWindow();
    // this needs to move to window / user preferences..
    static const float SIZE = 10;
    bool pointUpdated = false;

    ImVec2 cursorScreenPosition = GetCursorScreenPos(); // this appears to be a constant!
    ImVec2 pointPosition = TransformToEditorSpace(p, dataParams);

    ImGui::SetCursorScreenPos(pointPosition - ImVec2(SIZE, SIZE));
    PushID(dataParams.curvePointState.pointIndex);

    //changed from //InvisibleButton to Button to support Debuggin;
    Button("", ImVec2(SIZE * 2, SIZE * 2));

    bool pointSelected = dataParams.curvePointState.selectedIndex == dataParams.curvePointState.pointIndex;

    // should be user property!
    float pointThickness = pointSelected ? 2.0f : 1.0f;

    // fix to show currently selected point regardless of being active/hover!
    ImU32 pointColor = pointSelected ? GetColorU32(ImGuiCol_PlotHistogramHovered /*ImGuiCol_PlotLinesHovered*/) : GetColorU32(ImGuiCol_PlotLines);

    if (IsItemActive() || IsItemHovered())
    {
        // highlight the currently active one too... a different colour...
        pointColor = GetColorU32(ImGuiCol_PlotHistogramHovered /*ImGuiCol_PlotLinesHovered*/);
    }

    window->DrawList->AddLine(pointPosition + ImVec2(-SIZE, 0), pointPosition + ImVec2(0, SIZE), pointColor, pointThickness);
    window->DrawList->AddLine(pointPosition + ImVec2(SIZE, 0), pointPosition + ImVec2(0, SIZE), pointColor, pointThickness);
    window->DrawList->AddLine(pointPosition + ImVec2(SIZE, 0), pointPosition + ImVec2(0, -SIZE), pointColor, pointThickness);
    window->DrawList->AddLine(pointPosition + ImVec2(-SIZE, 0), pointPosition + ImVec2(0, -SIZE), pointColor, pointThickness);

    if (IsItemHovered())
    {
        dataParams.curvePointState.hoverIndex = dataParams.curvePointState.pointIndex;
    }

    if (IsItemActive() && ImGui::IsMouseClicked(0))
    {
        dataParams.curvePointState.selectedIndex = dataParams.curvePointState.pointIndex;

        // record the drag start position.
        dataParams.curvePointState.startDragPoint = pointPosition;
    }

    if (IsItemHovered() || IsItemActive() && IsMouseDragging(0))
    {
        char tmp[64];

        //tmp is the string formatting at the position of the hovered point
        ImFormatString(tmp, sizeof(tmp), "(%0.2f %0.2f)", p.x, p.y);

        // buffer to format the selected point values beneath the graph
        ImFormatString(dataParams.curvePointState.pointDebugBuffer, sizeof(tmp), "p:%0.2f,%0.2f", p.x, p.y);

        window->DrawList->AddText({ pointPosition.x, pointPosition.y - GetTextLineHeight() }, 0xff000000, tmp);
    }

    if (IsItemActive() && IsMouseDragging(0))
    {
        pointPosition = dataParams.curvePointState.startDragPoint;
        pointPosition += ImGui::GetMouseDragDelta();

        ImVec2 v = InverseTransformToEditorSpace(pointPosition, dataParams);

        // force bounds to minimum and maximum values allowed.
        if (v.x>dataParams.valueMax)
            v.x = dataParams.valueMax;
        if (v.y>dataParams.valueMax)
            v.y = dataParams.valueMax;
        if (v.x<dataParams.valueMin)
            v.x = dataParams.valueMin;
        if (v.y <= dataParams.valueMin)
            v.y = dataParams.valueMin;

        p = v; // assign new value based on dragging

        pointUpdated = true;
    }

    PopID();

    SetCursorScreenPos(cursorScreenPosition);
    return pointUpdated;
};


void CurveTypeSelectionBox(ImGui::CurveType curveType)
{
    const char* items[] = { "Linear", "Bezier" };
    ImGui::CurveType curves[] = { ImGui::CurveType::LINEAR, ImGui::CurveType::BEZIER };
    static const char* current_item = items[0];

    if (ImGui::BeginCombo("Curve Type", current_item)) // The second parameter is the label previewed before opening the combo.
    {
        for (int n = 0; n < IM_ARRAYSIZE(items); n++)
        {
            bool is_selected = (current_item == items[n]); // You can store your selection however you want, outside or inside your objects

            if (ImGui::Selectable(items[n], is_selected))
            {
                curveType = curves[n];
                current_item = items[n];
            }

            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
            }
        }
        ImGui::EndCombo();
    }
}

//for the buffer in AddGridLines
const int   bufsize = 10;

const float paddingOffsetMultiplier = 12.0f;
const float paddingOffset = .1f;

void AddAxisLabels(ImGuiWindow* window, ImGui::CurveEditorWindowParams& curveParams)
{
    //y-axis label
    ImVec2 axisText = TransformToEditorSpace({
        curveParams.graphParams.minPoint.x + paddingOffset / 10.0f,
        ((curveParams.graphParams.maxPoint.y - curveParams.graphParams.minPoint.y) / 2.0f) - paddingOffset / 2.0f
    },
        curveParams.graphParams);
    window->DrawList->AddText(axisText, IMGUI_USER_TEXT_MASK, curveParams.yAxisLabel);

    //x-axis label
    axisText = TransformToEditorSpace({
        (curveParams.graphParams.maxPoint.x - curveParams.graphParams.minPoint.x) / 2.0f - paddingOffset,
        curveParams.graphParams.minPoint.y + paddingOffset / 2.0f
    },
        curveParams.graphParams);
    window->DrawList->AddText(axisText, IMGUI_USER_TEXT_MASK, curveParams.xAxisLabel);
}

void AddGridLines(ImGuiWindow* window,
    int columns,
    int rows,
    ImGui::CurveEditorWindowParams& curveParams)
{
    //for printing the text of the rows and columns
    char buf[bufsize];

    //column lines
    float horizontalPadding = (paddingOffsetMultiplier * curveParams.padding) / curveParams.width;
    float step_x = (curveParams.graphParams.width - horizontalPadding) / columns;

    for (int i = 0; i < columns + 1; ++i)
    {
        ImVec2 xBot = TransformToEditorSpace({ i * step_x, curveParams.graphParams.minPoint.y }, curveParams.graphParams);
        ImVec2 xTop = TransformToEditorSpace({ i * step_x, curveParams.graphParams.maxPoint.y }, curveParams.graphParams);
        ImVec2 xText = TransformToEditorSpace({ i * step_x, curveParams.graphParams.minPoint.y + paddingOffset }, curveParams.graphParams);

        //set the value of the buffer
        snprintf(buf, bufsize, "%.1f", (curveParams.xAxisMaxValue / columns) * i);

        window->DrawList->AddLine(xBot, xTop, IM_COL32_A_MASK);
        window->DrawList->AddText(xText, IMGUI_USER_TEXT_MASK, buf);  //add line value label
    }

    //row lines
    float verticalPadding = (paddingOffsetMultiplier * curveParams.padding) / curveParams.height;
    float step_y = (curveParams.graphParams.height - verticalPadding) / rows;

    for (int i = 0; i < rows + 1; ++i)
    {
        ImVec2 yLeft = TransformToEditorSpace({ curveParams.graphParams.minPoint.x, i * step_y }, curveParams.graphParams);
        ImVec2 yRight = TransformToEditorSpace({ curveParams.graphParams.maxPoint.x, i * step_y }, curveParams.graphParams);
        ImVec2 yText = TransformToEditorSpace({ curveParams.graphParams.minPoint.x + paddingOffset / 10.0f, i * step_y }, curveParams.graphParams);

        snprintf(buf, bufsize, "%.1f", (curveParams.yAxisMaxValue / rows) * i);

        window->DrawList->AddLine(yLeft, yRight, IM_COL32_A_MASK);

        //skip duplicate zero
        if (i != 0)
        {
            window->DrawList->AddText(yText, IMGUI_USER_TEXT_MASK, buf);
        }

    }
}

void DrawGraphLines(ImGui::PointEncapsulation* peValues,
    unsigned& numPoints,
    ImGui::CurveGraphDataParams& graphParams,
    ImGui::CurveType& curveType,
    ImGuiWindow* window)
{
    for (unsigned pointIndex = 0; pointIndex < numPoints; ++pointIndex)
    {
        ImVec2& point = peValues[pointIndex].point;
        ImVec2* previousPoint = nullptr;
        ImVec2* nextPoint = nullptr;

        if (pointIndex > 0)
        {
            previousPoint = &peValues[pointIndex - 1].point;
        }

        if (pointIndex < numPoints - 1) // final point has no next (numPoints-1)
        {
            nextPoint = &peValues[pointIndex + 1].point;
        }

        ImGui::PushID(pointIndex);

        graphParams.curvePointState.pointIndex = pointIndex;

        // We draw the line between the current and next point if applicable.
        if (nextPoint)
        {
            ImVec2 from = TransformToEditorSpace(point, graphParams);
            ImVec2 to = TransformToEditorSpace(*nextPoint, graphParams);

            switch (curveType)
            {
            case ImGui::CurveType::BEZIER:
            {
                ImVec2 cp0(0.1f, 0.5f);
                ImVec2 cp1(0.1f, 0.5f);
                window->DrawList->AddBezierCurve(from, from*0.9, from*0.9, to, ImGui::GetColorU32(ImGuiCol_PlotLines), 1.0f, 10);
                break;
            }
            case ImGui::CurveType::LINEAR:
            {
                window->DrawList->AddLine(from, to, ImGui::GetColorU32(ImGuiCol_PlotLines), 1.0f);
                break;
            }
            }
        }

        if (EditorLinearPoint(point, graphParams))
        {
            if (nextPoint && point.x >= nextPoint->x)
            {
                // 0.001f needs to be a setting / user preference / configuration in the params.
                point.x = nextPoint->x - 0.001f;
            }

            if (previousPoint && point.x <= previousPoint->x)
            {
                // 0.001f needs to be a setting / user preference / configuration in the params.
                point.x = previousPoint->x + 0.001f;
            }

            graphParams.curvePointState.changedPointIndex = pointIndex + 1;
        }

        ImGui::PopID();
    }
}

void AddPoint(ImGui::PointEncapsulation* peValues,
    unsigned& numPoints,
    ImGui::CurveGraphDataParams& graphParams)
{
    ImVec2 mousePosition = ImGui::GetMousePos();
    ImVec2 newPoint = ImGui::InverseTransformToEditorSpace(mousePosition, graphParams);
    ImGui::PointEncapsulation pe;
    pe.point = newPoint;

    peValues[numPoints] = pe;

    auto compare = [](const void* a, const void * b) -> int
    {
        const ImGui::PointEncapsulation* point1 = static_cast<const ImGui::PointEncapsulation *>(a);
        const ImGui::PointEncapsulation* point2 = static_cast<const ImGui::PointEncapsulation *>(b);

        float x1 = point1->point.x;
        float x2 = point2->point.x;

        return x1 < x2 ? -1 : (x1 > x2) ? 1 : 0;
    };

    //qsort(values, *numPoints+1, sizeof(ImVec2), compare);
    qsort(peValues, numPoints + 1, sizeof(ImVec2), compare);

    ++numPoints;

    // reset point states
    graphParams.curvePointState.selectedIndex = -1;
    graphParams.curvePointState.hoverIndex = -1;

    // -999 is significant to return a value as any value other than -1 represents a change... and something is changing here... It's adding a point.
    graphParams.curvePointState.changedPointIndex = -999;
}

void RemovePoint(ImGui::PointEncapsulation* peValues,
    unsigned& numPoints,
    ImGui::CurveGraphDataParams& graphParams)
{
    for (int j = graphParams.curvePointState.hoverIndex; j < (static_cast<int>(numPoints) - 1); ++j)
    {
        peValues[j] = peValues[j + 1];
    }

    --numPoints;

    // reset point states
    graphParams.curvePointState.selectedIndex = -1;
    graphParams.curvePointState.hoverIndex = -1;

    // -999 is significant to return a value as any value other than -1 represents a change... and something is changing here... It's adding a point.
    // this might be able to be removed if the index state is kept in the original params and propogated correctly.
    graphParams.curvePointState.changedPointIndex = -999;
}


int ImGui::EditorCurve(
    PointEncapsulation* peValues,
    unsigned& numPoints,
    CurveEditorWindowParams& curveEditorWindowParams,
    CurveType& curveType,
    CurveEditorFlags curveEditorFlags,
    CurveDataRangeType curveDataRangeType,
    CurveOptions& options)
{
    CurveGraphDataParams& graphParams = curveEditorWindowParams.graphParams;

    ImGuiWindow* parent_window = GetCurrentWindow();
    const ImGuiID id = parent_window->GetID(curveEditorWindowParams.label);
    const ImGuiStyle& style = ImGui::GetStyle();

    EditorCurveFunction(graphParams, curveEditorWindowParams);

    //validate that the user supplied configuration is a valid one or bomb out!! needs innerBB, frameBB, height / width, min / max etc
    if (!graphParams.ValidateCurveGraphDataParams())
    {
        
    }

    // Options for Types of Curves --
    CurveTypeSelectionBox(curveType);

    curveEditorWindowParams.editorSize.x = curveEditorWindowParams.editorSize.x < 0 ? curveEditorWindowParams.width + (style.FramePadding.x * 2) : curveEditorWindowParams.editorSize.x;
    curveEditorWindowParams.editorSize.y = curveEditorWindowParams.editorSize.y  < 0 ? curveEditorWindowParams.height + (style.FramePadding.x * 2) : curveEditorWindowParams.editorSize.y;

    if (!BeginChildFrame(id, curveEditorWindowParams.editorSize, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        EndChildFrame();
        return -1;
    }

    ImGuiWindow* window = GetCurrentWindow(); //must happen after BeginChildFrame
    if (window->SkipItems)
    {
        EndChildFrame();
        return -1;
    }

    for (int pointId = 0; pointId < curveEditorWindowParams.graphParams.curvePointState.numPoints; ++pointId)
    {
        ImVec2 point = peValues[pointId].point;

        // update minimum and maximum point values found so far.
        graphParams.minPoint = ImMin(graphParams.minPoint, point);
        graphParams.maxPoint = ImMax(graphParams.maxPoint, point);
    }

    graphParams.width = graphParams.maxPoint.x - graphParams.minPoint.x;
    graphParams.height = graphParams.maxPoint.y - graphParams.minPoint.y;

    ImVec2 startPosition = GetCursorScreenPos();

    const ImRect innerBB = window->InnerRect;
    const ImRect frameBB(innerBB.Min - style.FramePadding, innerBB.Max + style.FramePadding);


    AddGridLines(window, 5, 5, curveEditorWindowParams);
    AddAxisLabels(window, curveEditorWindowParams);

    DrawGraphLines(peValues, numPoints, graphParams, curveType, window);

    SetCursorScreenPos(innerBB.Min);

    InvisibleButton("bg", innerBB.Max - innerBB.Min);

    if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0) && numPoints < graphParams.maximumPoints)
    {
        AddPoint(peValues, numPoints, graphParams);
    }

    if (graphParams.curvePointState.hoverIndex >= 0 && ImGui::IsMouseDoubleClicked(0) && numPoints > 2)
    {
        RemovePoint(peValues, numPoints, graphParams);
    }

    EndChildFrame();

    RenderText(ImVec2(frameBB.Max.x + style.ItemInnerSpacing.x, innerBB.Min.y), curveEditorWindowParams.label);

    // Preview the current X,Y value being selected / adjusted
    Text(graphParams.curvePointState.pointDebugBuffer);

    // some help around position of cursor and transform/inverse transform
    char tBuffer[64];
    ImFormatString(tBuffer, IM_ARRAYSIZE(tBuffer), "selected index %i", graphParams.curvePointState.selectedIndex);

    InputText("debug", tBuffer, IM_ARRAYSIZE(tBuffer), 0, 0, 0);

    //if (ImGui::Button("Delete point"))
    //{
    // perform the delete! :P
    //}

    if (ImGui::Button("Save curve"))
    {
        options.saveData = true;
    }

    graphParams.curvePointState.numPoints = numPoints;

    return graphParams.curvePointState.changedPointIndex;
}