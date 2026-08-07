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

#include <AzCore/Serialization/EditContext.h>

#include <AzQtComponents/Components/StyleManager.h>
#include <AzFramework/Translation/TranslationDef.h>

// Editor
#include "DisplaySettings.h"
#include "Settings.h"

namespace EditorPreferencesViewportGeneralStrings
{
    static const char* GeneralViewportSettingsClassName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "General Viewport Settings");
    static const char* Sync2DViewportsName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Synchronize 2D Viewports");
    static const char* PerspectiveViewFOVName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Perspective View FOV");
    static const char* PerspectiveNearPlaneName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Perspective Near Plane");
    static const char* PerspectiveFarPlaneName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Perspective Far Plane");
    static const char* PerspectiveAspectRatioName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Perspective View Aspect Ratio");
    static const char* EnableContextMenuName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Enable Right-Click Context Menu");
    static const char* EnableStickySelectName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Enable Sticky Select");

    static const char* ViewportDisplaySettingsClassName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Viewport Display Settings");
    static const char* HighlightSelectedGeometryName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Highlight Selected Geometry");
    static const char* HighlightSelectedVegetationName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Highlight Selected Vegetation");
    static const char* HighlightOnMouseOverName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Highlight Geometry On Mouse Over");
    static const char* HideCursorWhenCapturedName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Hide Cursor When Captured");
    static const char* HideCursorWhenCapturedDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Hide Mouse Cursor When Captured");
    static const char* DragSquareSizeName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Drag Square Size");
    static const char* DisplayObjectLinksName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Display Object Links");
    static const char* DisplayAnimationTracksName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Display Animation Tracks");
    static const char* AlwaysShowRadiiName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Always Show Radii");
    static const char* ShowBoundingBoxesName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Show Bounding Boxes");
    static const char* AlwaysDrawEntityLabelsName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Always Draw Entity Labels");
    static const char* AlwaysShowTriggerBoundsName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Always Show Trigger Bounds");
    static const char* ShowFrozenHelpersName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Show Helpers of Frozen Objects");
    static const char* FillSelectedShapesName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Fill Selected Shapes");
    static const char* ShowSnappingGridGuideName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Show Snapping Grid Guide");
    static const char* DisplayDimensionFiguresName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Display Dimension Figures");

    static const char* MapViewportSettingsClassName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Map Viewport Settings");
    static const char* SwapXYAxisName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Swap X/Y Axis");
    static const char* MapTextureResolutionName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Map Texture Resolution");

    static const char* TextLabelSettingsClassName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Text Label Settings");
    static const char* EnabledName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Enabled");
    static const char* DistanceName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Distance");

    static const char* SelectionPreviewColorClassName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Selection Preview Color Settings");
    static const char* GroupBoundingBoxName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Group Bounding Box");
    static const char* EntityBoundingBoxName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Entity Bounding Box");
    static const char* BBoxHighlightAlphaName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Bounding Box Highlight Alpha");
    static const char* GeometryColorName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Geometry Color");
    static const char* SolidBrushGeometryColorName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Solid Brush Geometry Color");
    static const char* GeometryHighlightAlphaName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Geometry Highlight Alpha");
    static const char* ChildGeometryHighlightAlphaName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "Child Geometry Highlight Alpha");

    static const char* GeneralViewportPreferencesClassName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportGeneral", "General Viewport Preferences");
}

void CEditorPreferencesPage_ViewportGeneral::Reflect(AZ::SerializeContext& serialize)
{
    serialize.Class<General>()
        ->Version(1)
        ->Field("Sync2DViews", &General::m_sync2DViews)
        ->Field("DefaultFOV", &General::m_defaultFOV)
        ->Field("DefaultNearPlane", &General::m_defaultNearPlane)
        ->Field("DefaultFarPlane", &General::m_defaultFarPlane)
        ->Field("DefaultAspectRatio", &General::m_defaultAspectRatio)
        ->Field("EnableContextMenu", &General::m_contextMenuEnabled)
        ->Field("StickySelect", &General::m_stickySelectEnabled);

    serialize.Class<Display>()
        ->Version(1)
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
        using namespace EditorPreferencesViewportGeneralStrings;

        editContext->Class<General>(GeneralViewportSettingsClassName, "")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &General::m_sync2DViews, Sync2DViewportsName, Sync2DViewportsName)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &General::m_defaultFOV, PerspectiveViewFOVName, PerspectiveViewFOVName)
            ->Attribute(AZ::Edit::Attributes::Min, 1.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 120.0f)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &General::m_defaultNearPlane, PerspectiveNearPlaneName, PerspectiveNearPlaneName)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &General::m_defaultFarPlane, PerspectiveFarPlaneName, PerspectiveFarPlaneName)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &General::m_defaultAspectRatio, PerspectiveAspectRatioName, PerspectiveAspectRatioName)
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &General::m_contextMenuEnabled, EnableContextMenuName, EnableContextMenuName)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &General::m_stickySelectEnabled, EnableStickySelectName, EnableStickySelectName);

        editContext->Class<Display>(ViewportDisplaySettingsClassName, "")
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &Display::m_highlightSelGeom, HighlightSelectedGeometryName, HighlightSelectedGeometryName)
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &Display::m_highlightSelVegetation, HighlightSelectedVegetationName, HighlightSelectedVegetationName)
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &Display::m_highlightOnMouseOver, HighlightOnMouseOverName, HighlightOnMouseOverName)
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &Display::m_hideMouseCursorWhenCaptured, HideCursorWhenCapturedName, HideCursorWhenCapturedDesc)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &Display::m_dragSquareSize, DragSquareSizeName, DragSquareSizeName)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Display::m_displayLinks, DisplayObjectLinksName, DisplayObjectLinksName)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Display::m_displayTracks, DisplayAnimationTracksName, DisplayAnimationTracksName)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Display::m_alwaysShowRadii, AlwaysShowRadiiName, AlwaysShowRadiiName)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Display::m_showBBoxes, ShowBoundingBoxesName, ShowBoundingBoxesName)
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &Display::m_drawEntityLabels, AlwaysDrawEntityLabelsName, AlwaysDrawEntityLabelsName)
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &Display::m_showTriggerBounds, AlwaysShowTriggerBoundsName, AlwaysShowTriggerBoundsName)
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &Display::m_showFrozenHelpers, ShowFrozenHelpersName, ShowFrozenHelpersName)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Display::m_fillSelectedShapes, FillSelectedShapesName, FillSelectedShapesName)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Display::m_showGridGuide, ShowSnappingGridGuideName, ShowSnappingGridGuideName)
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &Display::m_displayDimension, DisplayDimensionFiguresName, DisplayDimensionFiguresName);

        editContext->Class<MapViewport>(MapViewportSettingsClassName, "")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &MapViewport::m_swapXY, SwapXYAxisName, SwapXYAxisName)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &MapViewport::m_resolution, MapTextureResolutionName, MapTextureResolutionName);

        editContext->Class<TextLabels>(TextLabelSettingsClassName, "")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &TextLabels::m_labelsOn, EnabledName, EnabledName)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &TextLabels::m_labelsDistance, DistanceName, DistanceName)
            ->Attribute(AZ::Edit::Attributes::Min, 0.f)
            ->Attribute(AZ::Edit::Attributes::Max, 100000.f);

        editContext->Class<SelectionPreviewColor>(SelectionPreviewColorClassName, "")
            ->DataElement(AZ::Edit::UIHandlers::Color, &SelectionPreviewColor::m_colorGroupBBox, GroupBoundingBoxName, GroupBoundingBoxName)
            ->DataElement(
                AZ::Edit::UIHandlers::Color, &SelectionPreviewColor::m_colorEntityBBox, EntityBoundingBoxName, EntityBoundingBoxName)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &SelectionPreviewColor::m_fBBoxAlpha, BBoxHighlightAlphaName, BBoxHighlightAlphaName)
            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
            ->DataElement(AZ::Edit::UIHandlers::Color, &SelectionPreviewColor::m_geometryHighlightColor, GeometryColorName, GeometryColorName)
            ->DataElement(
                AZ::Edit::UIHandlers::Color, &SelectionPreviewColor::m_solidBrushGeometryColor, SolidBrushGeometryColorName, SolidBrushGeometryColorName)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &SelectionPreviewColor::m_fgeomAlpha, GeometryHighlightAlphaName, GeometryHighlightAlphaName)
            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &SelectionPreviewColor::m_childObjectGeomAlpha, ChildGeometryHighlightAlphaName, ChildGeometryHighlightAlphaName)
            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 1.0f);

        editContext->Class<CEditorPreferencesPage_ViewportGeneral>(GeneralViewportPreferencesClassName, GeneralViewportPreferencesClassName)
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC_CE("PropertyVisibility_ShowChildrenOnly"))
            ->DataElement(
                AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_ViewportGeneral::m_general, GeneralViewportSettingsClassName, GeneralViewportSettingsClassName)
            ->DataElement(
                AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_ViewportGeneral::m_display, ViewportDisplaySettingsClassName, ViewportDisplaySettingsClassName)
            ->DataElement(
                AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_ViewportGeneral::m_map, MapViewportSettingsClassName, MapViewportSettingsClassName)
            ->DataElement(
                AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_ViewportGeneral::m_textLabels, TextLabelSettingsClassName, TextLabelSettingsClassName)
            ->DataElement(
                AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_ViewportGeneral::m_selectionPreviewColor, SelectionPreviewColorClassName, SelectionPreviewColorClassName);
    }
}

CEditorPreferencesPage_ViewportGeneral::CEditorPreferencesPage_ViewportGeneral()
{
    InitializeSettings();
    m_icon = QIcon(":/res/Viewport.svg");
}

const char* CEditorPreferencesPage_ViewportGeneral::GetCategory()
{
    return QT_TRANSLATE_NOOP("EditorPreferencesDialog", "Viewports");
}

const char* CEditorPreferencesPage_ViewportGeneral::GetTitle()
{
    return QT_TRANSLATE_NOOP("EditorPreferencesDialog", "Viewport");
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
    gSettings.viewports.bEnableContextMenu = m_general.m_contextMenuEnabled;
    gSettings.viewports.bSync2DViews = m_general.m_sync2DViews;
    SandboxEditor::SetStickySelectEnabled(m_general.m_stickySelectEnabled);

    SandboxEditor::SetCameraDefaultFovDegrees(m_general.m_defaultFOV);
    SandboxEditor::SetCameraDefaultNearPlaneDistance(m_general.m_defaultNearPlane);
    SandboxEditor::SetCameraDefaultFarPlaneDistance(m_general.m_defaultFarPlane);

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
    m_general.m_defaultNearPlane = SandboxEditor::CameraDefaultNearPlaneDistance();
    m_general.m_defaultFarPlane = SandboxEditor::CameraDefaultFarPlaneDistance();
    m_general.m_defaultFOV = SandboxEditor::CameraDefaultFovDegrees();

    m_general.m_contextMenuEnabled = gSettings.viewports.bEnableContextMenu;
    m_general.m_sync2DViews = gSettings.viewports.bSync2DViews;
    m_general.m_stickySelectEnabled = SandboxEditor::StickySelectEnabled();

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
