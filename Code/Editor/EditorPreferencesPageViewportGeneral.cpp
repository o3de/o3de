/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "EditorPreferencesPageViewportGeneral.h"
#include "EditorViewportSettings.h"

#include <AzQtComponents/Components/StyleManager.h>

// Editor
#include "DisplaySettings.h"
#include "Settings.h"

void CEditorPreferencesPage_ViewportGeneral::Reflect(AZ::SerializeContext& serialize)
{
    serialize.Class<General>()
        ->Version(1)
        ->Field("Sync2DViews", &General::m_sync2DViews)
        ->Field("DefaultFOV", &General::m_defaultFOV)
        ->Field("DefaultAspectRatio", &General::m_defaultAspectRatio)
        ->Field("EnableContextMenu", &General::m_contextMenuEnabled)
        ->Field("StickySelect", &General::m_stickySelectEnabled);

    serialize.Class<Display>()
        ->Version(1)
        ->Field("ShowSafeFrame", &Display::m_showSafeFrame)
        ->Field("HighlightSelGeom", &Display::m_highlightSelGeom)
        ->Field("HighlightSelVegetation", &Display::m_highlightSelVegetation)
        ->Field("HighlightOnMouseOver", &Display::m_highlightOnMouseOver)
        ->Field("HideMouseCursorWhenCaptured", &Display::m_hideMouseCursorWhenCaptured)
        ->Field("DragSquareSize", &Display::m_dragSquareSize)
        ->Field("DisplayLinks", &Display::m_displayLinks)
        ->Field("DisplayTracks", &Display::m_displayTracks)
        ->Field("AlwaysShowRadii", &Display::m_alwaysShowRadii)
        ->Field("ShowBBoxes", &Display::m_showBBoxes)
        ->Field("DrawEntityLabels", &Display::m_drawEntityLabels)
        ->Field("ShowTriggerBounds", &Display::m_showTriggerBounds)
        ->Field("ShowIcons", &Display::m_showIcons)
        ->Field("DistanceScaleIcons", &Display::m_distanceScaleIcons)
        ->Field("ShowFrozenHelpers", &Display::m_showFrozenHelpers)
        ->Field("FillSelectedShapes", &Display::m_fillSelectedShapes)
        ->Field("ShowGridGuide", &Display::m_showGridGuide)
        ->Field("DisplayDimensions", &Display::m_displayDimension);

    // clang-format off
    serialize.Class<MapViewport>()
        ->Version(1)
        ->Field("SwapXY", &MapViewport::m_swapXY)
        ->Field("Resolution", &MapViewport::m_resolution);
    // clang-format on

    serialize.Class<TextLabels>()
        ->Version(1)
        ->Field("LabelsOn", &TextLabels::m_labelsOn)
        ->Field("LabelsDistance", &TextLabels::m_labelsDistance);

    serialize.Class<SelectionPreviewColor>()
        ->Version(1)
        ->Field("ColorGroupBBox", &SelectionPreviewColor::m_colorGroupBBox)
        ->Field("ColorEntityBBox", &SelectionPreviewColor::m_colorEntityBBox)
        ->Field("BBoxAlpha", &SelectionPreviewColor::m_fBBoxAlpha)
        ->Field("GeometryHighlihgtColor", &SelectionPreviewColor::m_geometryHighlightColor)
        ->Field("SolidBrushGeometryColor", &SelectionPreviewColor::m_solidBrushGeometryColor)
        ->Field("GeomAlpha", &SelectionPreviewColor::m_fgeomAlpha)
        ->Field("ChildObjectGeomAlpha", &SelectionPreviewColor::m_childObjectGeomAlpha);

    serialize.Class<CEditorPreferencesPage_ViewportGeneral>()
        ->Version(1)
        ->Field("General Viewport Settings", &CEditorPreferencesPage_ViewportGeneral::m_general)
        ->Field("Viewport Displaying", &CEditorPreferencesPage_ViewportGeneral::m_display)
        ->Field("Map Viewport", &CEditorPreferencesPage_ViewportGeneral::m_map)
        ->Field("Test Labels", &CEditorPreferencesPage_ViewportGeneral::m_textLabels)
        ->Field("Selection Preview Color", &CEditorPreferencesPage_ViewportGeneral::m_selectionPreviewColor);

    AZ::EditContext* editContext = serialize.GetEditContext();
    if (editContext)
    {
        editContext->Class<General>("General Viewport Settings", "")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &General::m_sync2DViews, "Synchronize 2D Viewports", "Synchronize 2D Viewports")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &General::m_defaultFOV, "Perspective View FOV", "Perspective View FOV")
            ->Attribute("Multiplier", RAD2DEG(1))
            ->Attribute(AZ::Edit::Attributes::Min, 1.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 120.0f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &General::m_defaultAspectRatio, "Perspective View Aspect Ratio",
                "Perspective View Aspect Ratio")
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &General::m_contextMenuEnabled, "Enable Right-Click Context Menu",
                "Enable Right-Click Context Menu")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &General::m_stickySelectEnabled, "Enable Sticky Select", "Enable Sticky Select");

        editContext->Class<Display>("Viewport Display Settings", "")
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &Display::m_showSafeFrame, "Show 4:3 Aspect Ratio Frame", "Show 4:3 Aspect Ratio Frame")
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &Display::m_highlightSelGeom, "Highlight Selected Geometry", "Highlight Selected Geometry")
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &Display::m_highlightSelVegetation, "Highlight Selected Vegetation",
                "Highlight Selected Vegetation")
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &Display::m_highlightOnMouseOver, "Highlight Geometry On Mouse Over",
                "Highlight Geometry On Mouse Over")
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &Display::m_hideMouseCursorWhenCaptured, "Hide Cursor When Captured",
                "Hide Mouse Cursor When Captured")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &Display::m_dragSquareSize, "Drag Square Size", "Drag Square Size")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Display::m_displayLinks, "Display Object Links", "Display Object Links")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Display::m_displayTracks, "Display Animation Tracks", "Display Animation Tracks")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Display::m_alwaysShowRadii, "Always Show Radii", "Always Show Radii")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Display::m_showBBoxes, "Show Bounding Boxes", "Show Bounding Boxes")
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &Display::m_drawEntityLabels, "Always Draw Entity Labels", "Always Draw Entity Labels")
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &Display::m_showTriggerBounds, "Always Show Trigger Bounds", "Always Show Trigger Bounds")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Display::m_showIcons, "Show Object Icons", "Show Object Icons")
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &Display::m_distanceScaleIcons, "Scale Object Icons with Distance",
                "Scale Object Icons with Distance")
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &Display::m_showFrozenHelpers, "Show Helpers of Frozen Objects",
                "Show Helpers of Frozen Objects")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Display::m_fillSelectedShapes, "Fill Selected Shapes", "Fill Selected Shapes")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Display::m_showGridGuide, "Show Snapping Grid Guide", "Show Snapping Grid Guide")
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &Display::m_displayDimension, "Display Dimension Figures", "Display Dimension Figures");

        editContext->Class<MapViewport>("Map Viewport Settings", "")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &MapViewport::m_swapXY, "Swap X/Y Axis", "Swap X/Y Axis")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &MapViewport::m_resolution, "Map Texture Resolution", "Map Texture Resolution");

        editContext->Class<TextLabels>("Text Label Settings", "")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &TextLabels::m_labelsOn, "Enabled", "Enabled")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &TextLabels::m_labelsDistance, "Distance", "Distance")
            ->Attribute(AZ::Edit::Attributes::Min, 0.f)
            ->Attribute(AZ::Edit::Attributes::Max, 100000.f);

        editContext->Class<SelectionPreviewColor>("Selection Preview Color Settings", "")
            ->DataElement(AZ::Edit::UIHandlers::Color, &SelectionPreviewColor::m_colorGroupBBox, "Group Bounding Box", "Group Bounding Box")
            ->DataElement(
                AZ::Edit::UIHandlers::Color, &SelectionPreviewColor::m_colorEntityBBox, "Entity Bounding Box", "Entity Bounding Box")
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &SelectionPreviewColor::m_fBBoxAlpha, "Bounding Box Highlight Alpha",
                "Bounding Box Highlight Alpha")
            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
            ->DataElement(AZ::Edit::UIHandlers::Color, &SelectionPreviewColor::m_geometryHighlightColor, "Geometry Color", "Geometry Color")
            ->DataElement(
                AZ::Edit::UIHandlers::Color, &SelectionPreviewColor::m_solidBrushGeometryColor, "Solid Brush Geometry Color",
                "Solid Brush Geometry Color")
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &SelectionPreviewColor::m_fgeomAlpha, "Geometry Highlight Alpha", "Geometry Highlight Alpha")
            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &SelectionPreviewColor::m_childObjectGeomAlpha, "Child Geometry Highlight Alpha",
                "Child Geometry Highlight Alpha")
            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 1.0f);

        editContext->Class<CEditorPreferencesPage_ViewportGeneral>("General Viewport Preferences", "General Viewport Preferences")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
            ->DataElement(
                AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_ViewportGeneral::m_general, "General Viewport Settings",
                "General Viewport Settings")
            ->DataElement(
                AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_ViewportGeneral::m_display, "Viewport Display Settings",
                "Viewport Display Settings")
            ->DataElement(
                AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_ViewportGeneral::m_map, "Map Viewport Settings",
                "Map Viewport Settings")
            ->DataElement(
                AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_ViewportGeneral::m_textLabels, "Text Label Settings",
                "Text Label Settings")
            ->DataElement(
                AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_ViewportGeneral::m_selectionPreviewColor,
                "Selection Preview Color Settings", "Selection Preview Color Settings");
    }
}

CEditorPreferencesPage_ViewportGeneral::CEditorPreferencesPage_ViewportGeneral()
{
    InitializeSettings();
    m_icon = QIcon(":/res/Viewport.svg");
}

const char* CEditorPreferencesPage_ViewportGeneral::GetCategory()
{
    return "Viewports";
}

const char* CEditorPreferencesPage_ViewportGeneral::GetTitle()
{
    return "Viewport";
}

QIcon& CEditorPreferencesPage_ViewportGeneral::GetIcon()
{
    return m_icon;
}

void CEditorPreferencesPage_ViewportGeneral::OnCancel()
{
    // noop
}

bool CEditorPreferencesPage_ViewportGeneral::OnQueryCancel()
{
    return true;
}

void CEditorPreferencesPage_ViewportGeneral::OnApply()
{
    CDisplaySettings* ds = GetIEditor()->GetDisplaySettings();

    gSettings.viewports.fDefaultAspectRatio = m_general.m_defaultAspectRatio;
    gSettings.viewports.fDefaultFov = m_general.m_defaultFOV;
    gSettings.viewports.bEnableContextMenu = m_general.m_contextMenuEnabled;
    gSettings.viewports.bSync2DViews = m_general.m_sync2DViews;
    SandboxEditor::SetStickySelectEnabled(m_general.m_stickySelectEnabled);

    gSettings.viewports.bShowSafeFrame = m_display.m_showSafeFrame;
    gSettings.viewports.bHighlightSelectedGeometry = m_display.m_highlightSelGeom;
    gSettings.viewports.bHighlightSelectedVegetation = m_display.m_highlightSelVegetation;
    gSettings.viewports.bHighlightMouseOverGeometry = m_display.m_highlightOnMouseOver;
    gSettings.viewports.bHideMouseCursorWhenCaptured = m_display.m_hideMouseCursorWhenCaptured;
    gSettings.viewports.nDragSquareSize = m_display.m_dragSquareSize;
    ds->DisplayLinks(m_display.m_displayLinks);
    ds->DisplayTracks(m_display.m_displayTracks);
    gSettings.viewports.bAlwaysShowRadiuses = m_display.m_alwaysShowRadii;
    if (m_display.m_showBBoxes)
    {
        ds->SetRenderFlags(ds->GetRenderFlags() | RENDER_FLAG_BBOX);
    }
    else
    {
        ds->SetRenderFlags(ds->GetRenderFlags() & (~RENDER_FLAG_BBOX));
    }
    gSettings.viewports.bDrawEntityLabels = m_display.m_drawEntityLabels;
    gSettings.viewports.bShowTriggerBounds = m_display.m_showTriggerBounds;
    gSettings.viewports.bShowIcons = m_display.m_showIcons;
    gSettings.viewports.bDistanceScaleIcons = m_display.m_distanceScaleIcons;
    gSettings.viewports.nShowFrozenHelpers = m_display.m_showFrozenHelpers;
    gSettings.viewports.bFillSelectedShapes = m_display.m_fillSelectedShapes;
    gSettings.viewports.bShowGridGuide = m_display.m_showGridGuide;
    ds->DisplayDimensionFigures(m_display.m_displayDimension);

    gSettings.viewports.nTopMapTextureResolution = m_map.m_resolution;
    gSettings.viewports.bTopMapSwapXY = m_map.m_swapXY;

    ds->DisplayLabels(m_textLabels.m_labelsOn);
    ds->SetLabelsDistance(m_textLabels.m_labelsDistance);

    gSettings.objectColorSettings.fChildGeomAlpha = m_selectionPreviewColor.m_childObjectGeomAlpha;
    gSettings.objectColorSettings.entityHighlight = QColor(
        static_cast<int>(m_selectionPreviewColor.m_colorEntityBBox.GetR() * 255.0f),
        static_cast<int>(m_selectionPreviewColor.m_colorEntityBBox.GetG() * 255.0f),
        static_cast<int>(m_selectionPreviewColor.m_colorEntityBBox.GetB() * 255.0f));
    gSettings.objectColorSettings.groupHighlight = QColor(
        static_cast<int>(m_selectionPreviewColor.m_colorGroupBBox.GetR() * 255.0f),
        static_cast<int>(m_selectionPreviewColor.m_colorGroupBBox.GetG() * 255.0f),
        static_cast<int>(m_selectionPreviewColor.m_colorGroupBBox.GetB() * 255.0f));
    gSettings.objectColorSettings.fBBoxAlpha = m_selectionPreviewColor.m_fBBoxAlpha;
    gSettings.objectColorSettings.fGeomAlpha = m_selectionPreviewColor.m_fgeomAlpha;
    gSettings.objectColorSettings.geometryHighlightColor = QColor(
        static_cast<int>(m_selectionPreviewColor.m_geometryHighlightColor.GetR() * 255.0f),
        static_cast<int>(m_selectionPreviewColor.m_geometryHighlightColor.GetG() * 255.0f),
        static_cast<int>(m_selectionPreviewColor.m_geometryHighlightColor.GetB() * 255.0f));
    gSettings.objectColorSettings.solidBrushGeometryColor = QColor(
        static_cast<int>(m_selectionPreviewColor.m_solidBrushGeometryColor.GetR() * 255.0f),
        static_cast<int>(m_selectionPreviewColor.m_solidBrushGeometryColor.GetG() * 255.0f),
        static_cast<int>(m_selectionPreviewColor.m_solidBrushGeometryColor.GetB() * 255.0f));
}

void CEditorPreferencesPage_ViewportGeneral::InitializeSettings()
{
    CDisplaySettings* ds = GetIEditor()->GetDisplaySettings();

    m_general.m_defaultAspectRatio = gSettings.viewports.fDefaultAspectRatio;
    m_general.m_defaultFOV = gSettings.viewports.fDefaultFov;
    m_general.m_contextMenuEnabled = gSettings.viewports.bEnableContextMenu;
    m_general.m_sync2DViews = gSettings.viewports.bSync2DViews;
    m_general.m_stickySelectEnabled = SandboxEditor::StickySelectEnabled();

    m_display.m_showSafeFrame = gSettings.viewports.bShowSafeFrame;
    m_display.m_highlightSelGeom = gSettings.viewports.bHighlightSelectedGeometry;
    m_display.m_highlightSelVegetation = gSettings.viewports.bHighlightSelectedVegetation;
    m_display.m_highlightOnMouseOver = gSettings.viewports.bHighlightMouseOverGeometry;
    m_display.m_hideMouseCursorWhenCaptured = gSettings.viewports.bHideMouseCursorWhenCaptured;
    m_display.m_dragSquareSize = gSettings.viewports.nDragSquareSize;
    m_display.m_displayLinks = ds->IsDisplayLinks();
    m_display.m_displayTracks = ds->IsDisplayTracks();
    m_display.m_alwaysShowRadii = gSettings.viewports.bAlwaysShowRadiuses;
    m_display.m_showBBoxes = (ds->GetRenderFlags() & RENDER_FLAG_BBOX) == RENDER_FLAG_BBOX;
    m_display.m_drawEntityLabels = gSettings.viewports.bDrawEntityLabels;
    m_display.m_showTriggerBounds = gSettings.viewports.bShowTriggerBounds;
    m_display.m_showIcons = gSettings.viewports.bShowIcons;
    m_display.m_distanceScaleIcons = gSettings.viewports.bDistanceScaleIcons;
    m_display.m_showFrozenHelpers = gSettings.viewports.nShowFrozenHelpers;
    m_display.m_fillSelectedShapes = gSettings.viewports.bFillSelectedShapes;
    m_display.m_showGridGuide = gSettings.viewports.bShowGridGuide;
    m_display.m_displayDimension = ds->IsDisplayDimensionFigures();

    m_map.m_resolution = gSettings.viewports.nTopMapTextureResolution;
    m_map.m_swapXY = gSettings.viewports.bTopMapSwapXY;

    m_textLabels.m_labelsOn = ds->IsDisplayLabels();
    m_textLabels.m_labelsDistance = ds->GetLabelsDistance();

    m_selectionPreviewColor.m_childObjectGeomAlpha = gSettings.objectColorSettings.fChildGeomAlpha;
    m_selectionPreviewColor.m_colorEntityBBox.Set(
        static_cast<float>(gSettings.objectColorSettings.entityHighlight.redF()),
        static_cast<float>(gSettings.objectColorSettings.entityHighlight.greenF()),
        static_cast<float>(gSettings.objectColorSettings.entityHighlight.blueF()), 1.0f);
    m_selectionPreviewColor.m_colorGroupBBox.Set(
        static_cast<float>(gSettings.objectColorSettings.groupHighlight.redF()),
        static_cast<float>(gSettings.objectColorSettings.groupHighlight.greenF()),
        static_cast<float>(gSettings.objectColorSettings.groupHighlight.blueF()), 1.0f);
    m_selectionPreviewColor.m_fBBoxAlpha = gSettings.objectColorSettings.fBBoxAlpha;
    m_selectionPreviewColor.m_fgeomAlpha = gSettings.objectColorSettings.fGeomAlpha;
    m_selectionPreviewColor.m_geometryHighlightColor.Set(
        static_cast<float>(gSettings.objectColorSettings.geometryHighlightColor.redF()),
        static_cast<float>(gSettings.objectColorSettings.geometryHighlightColor.greenF()),
        static_cast<float>(gSettings.objectColorSettings.geometryHighlightColor.blueF()), 1.0f);
    m_selectionPreviewColor.m_solidBrushGeometryColor.Set(
        static_cast<float>(gSettings.objectColorSettings.solidBrushGeometryColor.redF()),
        static_cast<float>(gSettings.objectColorSettings.solidBrushGeometryColor.greenF()),
        static_cast<float>(gSettings.objectColorSettings.solidBrushGeometryColor.blueF()), 1.0f);
}
