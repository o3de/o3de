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

#pragma once

namespace ImGui
{

struct PointEncapsulation
{
    ImVec2 point = ImVec2(0.0f, 0.0f);
    ImVec2 controlPoint1 = ImVec2(0.0f, 0.0f);
    ImVec2 controlPoint2 = ImVec2(0.0f, 0.0f);
};

struct CurvePointState
{
    int numPoints = -1;

    int pointIndex = -1;
    int hoverIndex = -1;

    int selectedIndex = -1;
    int changedPointIndex = -1;

    ImVec2 startDragPoint = ImVec2(100.0f, 0.0f);

    char pointDebugBuffer[64];

    ImVec2 minPoint = ImVec2(FLT_MAX, FLT_MAX);
    ImVec2 maxPoint = ImVec2(-FLT_MAX, -FLT_MAX);
};


struct CurveGraphDataParams
{
    CurvePointState curvePointState;

    ImVec2 minPoint = ImVec2(FLT_MAX, FLT_MAX);
    ImVec2 maxPoint = ImVec2(-FLT_MAX, -FLT_MAX);

    float valueMax = 1.00f;
    float valueMin = 0.00f;

    unsigned maximumPoints = 1;

    float width = 0.0f;
    float height = 0.0f;

    bool ValidateCurveGraphDataParams()
    {
        return true;
    }
};

struct CurveEditorWindowParams
{
    const char* label = "unknown";

    ImVec2 editorSize = ImVec2(-1, -1);

    float height = 600;
    float width = 600;
    float padding = 10;

    //these values vary from type of curve to curve
    const char* xAxisLabel = "x-axis";
    float xAxisMaxValue = 1.0f;

    const char* yAxisLabel = "y-axis";
    float yAxisMaxValue = 1.0f;

    ImVec4 edgeColor = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 backgroundColor = ImVec4(0.5f, 0.5f, 0.5f, 1.00f);

    int horizontalGridDivisions = 5;
    int verticalGridDivisions = 5;

    int pointSize = 10;

    CurveGraphDataParams graphParams; 
};

enum class CurveEditorFlags
{
    SHOW_GRID = 1 << 1
};

enum class CurveType
{
    LINEAR = 1 << 1,
    BEZIER = 1 << 2
};

enum class CurveDataRangeType
{
    ABS = 1 << 1,// ABSOLUTE appears to be reserved in this scope
    NORMALIZED = 1 << 2,
    DATARANGETYPE = 1 << 3
};

struct CurveOptions
{
    bool saveData = false;
};

// points_count and new_count could probably be combined, just give curve editor the ownership
ImVec2 TransformToEditorSpace(const ImVec2& pos, ImGui::CurveGraphDataParams& dataParams);
ImVec2 InverseTransformToEditorSpace(const ImVec2& pos, ImGui::CurveGraphDataParams& dataParams);

IMGUI_API int EditorCurve(
    PointEncapsulation* peValues,
    unsigned& pointCount,
    CurveEditorWindowParams& curveEditorWindowParams,
    CurveType& curveType,
    CurveEditorFlags curveEditorFlags,
    CurveDataRangeType curveDataRangeType,
    CurveOptions& options
);

}
/*CurveType& curveType
, const char* label
, float* values
, int* points_count
, const ImVec2& size= ImVec2(-1, -1)
, ImU32 flags = 0
, int* new_count = nullptr
, int* selected_index = nullptr
, int max_count);*/

//bool EditorLinearPoint(ImVec2& p, int index, int* selectedIndex, int pointIndex, int hovered_idx, char* pointBuffer, CurveGraphDataParams dataParams);

