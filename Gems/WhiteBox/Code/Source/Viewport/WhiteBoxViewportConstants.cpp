/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxViewportConstants.h"

#include <QSettings>

namespace WhiteBox
{

    AZ_CVAR(
        float, cl_whiteBoxVertexManipulatorSize, 0.08f, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Radius used to pick a vertex handle, in screen-scaled units. Larger than the drawn dot so it stays easy to hit.");
    AZ_CVAR(
        float, cl_whiteBoxVertexHoverSize, 0.05f, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Radius of a vertex handle's dot while the cursor is over it, in screen-scaled units.");
    AZ_CVAR(
        float, cl_whiteBoxVertexDrawSize, 0.03f, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Radius of the dot drawn for a vertex handle, in screen-scaled units.");
    AZ_CVAR(float, cl_whiteBoxMouseClickDeltaThreshold, 0.001f, nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(float, cl_whiteBoxModifierMidpointEpsilon, 0.01f, nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(float, cl_whiteBoxEdgeVisualWidth, 5.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(float, cl_whiteBoxEdgeSelectionWidth, 0.075f, nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(float, cl_whiteBoxSelectedEdgeVisualWidth, 6.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "");

    AZ_CVAR(float, ed_whiteBoxPolygonViewOverlapOffset, 0.004f, nullptr, AZ::ConsoleFunctorFlags::Null, "The offset highlighted polygon");

    AZ_CVAR(AZ::Color, ed_whiteBoxVertexHiddenColor, AZ::Color::CreateFromRgba(200, 200, 200, 255), nullptr, AZ::ConsoleFunctorFlags::Null, "Color of the polygon when hovered over");
    AZ_CVAR(AZ::Color, ed_whiteBoxVertexRestoredColor,AZ::Color::CreateFromRgba(50, 50, 50, 150), nullptr, AZ::ConsoleFunctorFlags::Null, "Color of the outline when hovered over");   

    AZ_CVAR(AZ::Color, ed_whiteBoxVertexUnselected, AZ::Color::CreateFromRgba(0, 100, 255, 150), nullptr, AZ::ConsoleFunctorFlags::Null, "Color of the verticies when selected");
    AZ_CVAR(AZ::Color, ed_whiteBoxEdgeUnselected, AZ::Color::CreateFromRgba(127, 127, 127, 76), nullptr, AZ::ConsoleFunctorFlags::Null, "Color of the verticies when selected");
    AZ_CVAR(AZ::Color, ed_whiteBoxEdgeDefault, AZ::Color::CreateFromRgba(0, 0, 0, 76), nullptr, AZ::ConsoleFunctorFlags::Null, "Color of the verticies when selected");
    
    AZ_CVAR(AZ::Color, ed_whiteBoxVertexHover, AZ::Color::CreateFromRgba(0, 150, 255, 200), nullptr, AZ::ConsoleFunctorFlags::Null, "Color of the verticies when selected");
    AZ_CVAR(AZ::Color, ed_whiteBoxPolygonHover, AZ::Color::CreateFromRgba(255, 175, 0, 127), nullptr, AZ::ConsoleFunctorFlags::Null, "Color of the polygon when hovered over");
    AZ_CVAR(AZ::Color, ed_whiteBoxOutlineHover, AZ::Color::CreateFromRgba(255, 100, 0, 255), nullptr, AZ::ConsoleFunctorFlags::Null, "Color of the outline when hovered over");
   
    AZ_CVAR(AZ::Color, ed_whiteBoxPolygonSelection, AZ::Color::CreateFromRgba(0, 150, 255, 127), nullptr, AZ::ConsoleFunctorFlags::Null, "Color of the polygon when selected");
    AZ_CVAR(AZ::Color, ed_whiteBoxVertexSelection, AZ::Color::CreateFromRgba(0, 150, 255, 200), nullptr, AZ::ConsoleFunctorFlags::Null, "Color of the verticies when selected");
    AZ_CVAR(AZ::Color, ed_whiteBoxOutlineSelection, AZ::Color::CreateFromRgba(0, 150, 255, 255), nullptr, AZ::ConsoleFunctorFlags::Null, "Color of the outline when selected");

    namespace
    {
        constexpr const char* HandleSettingsOrganization = "O3DE";
        constexpr const char* HandleSettingsApplication = "WhiteBox";

        const ViewportHandleSetting HandleSettings[] = {
            { "VertexDrawSize", "Vertex Dot Size",
              "Radius of the dot drawn on each vertex. It keeps the same size on screen at any distance.", 0.005f, 0.2f,
              0.005f, 0.03f, []() { return static_cast<float>(cl_whiteBoxVertexDrawSize); },
              [](const float value) { cl_whiteBoxVertexDrawSize = value; } },
            { "VertexHoverSize", "Vertex Hover Size",
              "Radius of a vertex dot while the cursor is over it, so the vertex under the cursor stands out.", 0.005f, 0.2f,
              0.005f, 0.05f, []() { return static_cast<float>(cl_whiteBoxVertexHoverSize); },
              [](const float value) { cl_whiteBoxVertexHoverSize = value; } },
            { "VertexPickSize", "Vertex Pick Radius",
              "How close the cursor has to be to a vertex to pick it. Larger than the dot, so small dots stay easy to hit.",
              0.005f, 0.3f, 0.005f, 0.08f, []() { return static_cast<float>(cl_whiteBoxVertexManipulatorSize); },
              [](const float value) { cl_whiteBoxVertexManipulatorSize = value; } },
            { "EdgeLineWidth", "Edge Line Width", "Width in pixels of the edge lines drawn in edit mode.", 1.0f, 12.0f, 0.5f,
              5.0f, []() { return static_cast<float>(cl_whiteBoxEdgeVisualWidth); },
              [](const float value) { cl_whiteBoxEdgeVisualWidth = value; } },
            { "SelectedEdgeLineWidth", "Selected Edge Line Width", "Width in pixels of a selected or hovered edge.", 1.0f,
              12.0f, 0.5f, 6.0f, []() { return static_cast<float>(cl_whiteBoxSelectedEdgeVisualWidth); },
              [](const float value) { cl_whiteBoxSelectedEdgeVisualWidth = value; } },
            { "EdgePickWidth", "Edge Pick Width", "How close the cursor has to be to an edge to pick it.", 0.005f, 0.3f, 0.005f,
              0.075f, []() { return static_cast<float>(cl_whiteBoxEdgeSelectionWidth); },
              [](const float value) { cl_whiteBoxEdgeSelectionWidth = value; } },
        };
    } // namespace

    AZStd::span<const ViewportHandleSetting> ViewportHandleSettings()
    {
        return HandleSettings;
    }

    void LoadViewportHandleSettings()
    {
        const QSettings settings(HandleSettingsOrganization, HandleSettingsApplication);
        for (const ViewportHandleSetting& setting : HandleSettings)
        {
            if (const QString key = QString::fromUtf8(setting.m_key); settings.contains(key))
            {
                bool ok = false;
                const float value = settings.value(key).toFloat(&ok);
                if (ok)
                {
                    setting.m_set(AZStd::clamp(value, setting.m_min, setting.m_max));
                }
            }
        }
    }

    void SaveViewportHandleSettings()
    {
        QSettings settings(HandleSettingsOrganization, HandleSettingsApplication);
        for (const ViewportHandleSetting& setting : HandleSettings)
        {
            settings.setValue(QString::fromUtf8(setting.m_key), setting.m_get());
        }
    }

    void ResetViewportHandleSettings()
    {
        QSettings settings(HandleSettingsOrganization, HandleSettingsApplication);
        for (const ViewportHandleSetting& setting : HandleSettings)
        {
            setting.m_set(setting.m_default);
            settings.remove(QString::fromUtf8(setting.m_key));
        }
    }
} // namespace WhiteBox
