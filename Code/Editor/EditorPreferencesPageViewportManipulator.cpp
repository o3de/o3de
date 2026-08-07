/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "EditorPreferencesPageViewportManipulator.h"

#include <AzCore/Serialization/EditContext.h>

#include <AzToolsFramework/Viewport/ViewportSettings.h>
#include <AzFramework/Translation/TranslationDef.h>

// Editor
#include "EditorViewportSettings.h"
#include "Settings.h"

namespace EditorPreferencesViewportManipulatorStrings
{
    static const char* ManipulatorsClassName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportManipulator", "Manipulators");
    static const char* LineBoundWidthName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportManipulator", "Line Bound Width");
    static const char* LineBoundWidthDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportManipulator", "Manipulator Line Bound Width");
    static const char* CircleBoundWidthName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportManipulator", "Circle Bound Width");
    static const char* CircleBoundWidthDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportManipulator", "Manipulator Circle Bound Width");
    static const char* LinearAxisLengthName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportManipulator", "Linear Manipulator Axis Length");
    static const char* LinearAxisLengthDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportManipulator", "Length of default Linear Manipulator (for Translation and Scale Manipulators)");
    static const char* PlanarAxisLengthName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportManipulator", "Planar Manipulator Axis Length");
    static const char* PlanarAxisLengthDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportManipulator", "Length of default Planar Manipulator (for Translation Manipulators)");
    static const char* SurfaceRadiusName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportManipulator", "Surface Manipulator Radius");
    static const char* SurfaceRadiusDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportManipulator", "Radius of default Surface Manipulator (for Translation Manipulators)");
    static const char* SurfaceOpacityName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportManipulator", "Surface Manipulator Opacity");
    static const char* SurfaceOpacityDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportManipulator", "Opacity of default Surface Manipulator (for Translation Manipulators)");
    static const char* LinearConeLengthName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportManipulator", "Linear Manipulator Cone Length");
    static const char* LinearConeLengthDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportManipulator", "Length of cone for default Linear Manipulator (for Translation Manipulators)");
    static const char* LinearConeRadiusName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportManipulator", "Linear Manipulator Cone Radius");
    static const char* LinearConeRadiusDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportManipulator", "Radius of cone for default Linear Manipulator (for Translation Manipulators)");
    static const char* ScaleBoxHalfExtentName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportManipulator", "Scale Manipulator Box Half Extent");
    static const char* ScaleBoxHalfExtentDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportManipulator", "Half extent of box for default Scale Manipulator");
    static const char* RotationRadiusName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportManipulator", "Rotation Manipulator Radius");
    static const char* RotationRadiusDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportManipulator", "Radius of default Angular Manipulators (for Rotation Manipulators)");
    static const char* ViewBaseScaleName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportManipulator", "Manipulator View Base Scale");
    static const char* ViewBaseScaleDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportManipulator", "The base scale to apply to all Manipulator Views (default is 1.0)");
    static const char* FlipAxesTowardsViewName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportManipulator", "Flip Manipulator Axes Towards View");
    static const char* FlipAxesTowardsViewDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportManipulator", "Determines whether Planar and Linear Manipulators should switch to face the view (camera) in the Editor");

    static const char* ManipulatorViewportPreferencesClassName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportManipulator", "Manipulator Viewport Preferences");
}

void CEditorPreferencesPage_ViewportManipulator::Reflect(AZ::SerializeContext& serialize)
{
    serialize.Class<Manipulators>()
        ->Version(1)
        ->Field("LineBoundWidth", &Manipulators::m_manipulatorLineBoundWidth)
        ->Field("CircleBoundWidth", &Manipulators::m_manipulatorCircleBoundWidth)
        ->Field("LinearManipulatorAxisLength", &Manipulators::m_linearManipulatorAxisLength)
        ->Field("PlanarManipulatorAxisLength", &Manipulators::m_planarManipulatorAxisLength)
        ->Field("SurfaceManipulatorRadius", &Manipulators::m_surfaceManipulatorRadius)
        ->Field("SurfaceManipulatorOpacity", &Manipulators::m_surfaceManipulatorOpacity)
        ->Field("LinearManipulatorConeLength", &Manipulators::m_linearManipulatorConeLength)
        ->Field("LinearManipulatorConeRadius", &Manipulators::m_linearManipulatorConeRadius)
        ->Field("ScaleManipulatorBoxHalfExtent", &Manipulators::m_scaleManipulatorBoxHalfExtent)
        ->Field("RotationManipulatorRadius", &Manipulators::m_rotationManipulatorRadius)
        ->Field("ManipulatorViewBaseScale", &Manipulators::m_manipulatorViewBaseScale)
        ->Field("FlipManipulatorAxesTowardsView", &Manipulators::m_flipManipulatorAxesTowardsView);

    serialize.Class<CEditorPreferencesPage_ViewportManipulator>()->Version(2)->Field(
        "Manipulators", &CEditorPreferencesPage_ViewportManipulator::m_manipulators);

    if (AZ::EditContext* editContext = serialize.GetEditContext())
    {
        using namespace EditorPreferencesViewportManipulatorStrings;

        editContext->Class<Manipulators>(ManipulatorsClassName, "")
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &Manipulators::m_manipulatorLineBoundWidth, LineBoundWidthName, LineBoundWidthDesc)
            ->Attribute(AZ::Edit::Attributes::Min, 0.001f)
            ->Attribute(AZ::Edit::Attributes::Max, 2.0f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &Manipulators::m_manipulatorCircleBoundWidth, CircleBoundWidthName, CircleBoundWidthDesc)
            ->Attribute(AZ::Edit::Attributes::Min, 0.001f)
            ->Attribute(AZ::Edit::Attributes::Max, 2.0f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &Manipulators::m_linearManipulatorAxisLength, LinearAxisLengthName, LinearAxisLengthDesc)
            ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
            ->Attribute(AZ::Edit::Attributes::Max, 5.0f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &Manipulators::m_planarManipulatorAxisLength, PlanarAxisLengthName, PlanarAxisLengthDesc)
            ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
            ->Attribute(AZ::Edit::Attributes::Max, 5.0f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &Manipulators::m_surfaceManipulatorRadius, SurfaceRadiusName, SurfaceRadiusDesc)
            ->Attribute(AZ::Edit::Attributes::Min, 0.05f)
            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &Manipulators::m_surfaceManipulatorOpacity, SurfaceOpacityName, SurfaceOpacityDesc)
            ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &Manipulators::m_linearManipulatorConeLength, LinearConeLengthName, LinearConeLengthDesc)
            ->Attribute(AZ::Edit::Attributes::Min, 0.05f)
            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &Manipulators::m_linearManipulatorConeRadius, LinearConeRadiusName, LinearConeRadiusDesc)
            ->Attribute(AZ::Edit::Attributes::Min, 0.05f)
            ->Attribute(AZ::Edit::Attributes::Max, 0.5f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &Manipulators::m_scaleManipulatorBoxHalfExtent, ScaleBoxHalfExtentName, ScaleBoxHalfExtentDesc)
            ->Attribute(AZ::Edit::Attributes::Min, 0.05f)
            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &Manipulators::m_rotationManipulatorRadius, RotationRadiusName, RotationRadiusDesc)
            ->Attribute(AZ::Edit::Attributes::Min, 0.5f)
            ->Attribute(AZ::Edit::Attributes::Max, 5.0f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &Manipulators::m_manipulatorViewBaseScale, ViewBaseScaleName, ViewBaseScaleDesc)
            ->Attribute(AZ::Edit::Attributes::Min, AzToolsFramework::MinManipulatorViewBaseScale)
            ->Attribute(AZ::Edit::Attributes::Max, AzToolsFramework::MaxManipulatorViewBaseScale)
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &Manipulators::m_flipManipulatorAxesTowardsView, FlipAxesTowardsViewName, FlipAxesTowardsViewDesc);

        editContext
            ->Class<CEditorPreferencesPage_ViewportManipulator>(ManipulatorViewportPreferencesClassName, ManipulatorViewportPreferencesClassName)
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC_CE("PropertyVisibility_ShowChildrenOnly"))
            ->DataElement(
                AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_ViewportManipulator::m_manipulators, ManipulatorsClassName, ManipulatorsClassName);
    }
}

CEditorPreferencesPage_ViewportManipulator::CEditorPreferencesPage_ViewportManipulator()
{
    InitializeSettings();
    m_icon = QIcon(":/res/Gizmos.svg");
}

const char* CEditorPreferencesPage_ViewportManipulator::GetCategory()
{
    return QT_TRANSLATE_NOOP("EditorPreferencesDialog", "Viewports");
}

const char* CEditorPreferencesPage_ViewportManipulator::GetTitle()
{
    return QT_TRANSLATE_NOOP("EditorPreferencesDialog", "Manipulators");
}

QIcon& CEditorPreferencesPage_ViewportManipulator::GetIcon()
{
    return m_icon;
}

void CEditorPreferencesPage_ViewportManipulator::OnCancel()
{
    // noop
}

bool CEditorPreferencesPage_ViewportManipulator::OnQueryCancel()
{
    return true;
}

void CEditorPreferencesPage_ViewportManipulator::OnApply()
{
    SandboxEditor::SetManipulatorLineBoundWidth(m_manipulators.m_manipulatorLineBoundWidth);
    SandboxEditor::SetManipulatorCircleBoundWidth(m_manipulators.m_manipulatorCircleBoundWidth);

    AzToolsFramework::SetLinearManipulatorAxisLength(m_manipulators.m_linearManipulatorAxisLength);
    AzToolsFramework::SetPlanarManipulatorAxisLength(m_manipulators.m_planarManipulatorAxisLength);
    AzToolsFramework::SetSurfaceManipulatorRadius(m_manipulators.m_surfaceManipulatorRadius);
    AzToolsFramework::SetSurfaceManipulatorOpacity(m_manipulators.m_surfaceManipulatorOpacity);
    AzToolsFramework::SetLinearManipulatorConeLength(m_manipulators.m_linearManipulatorConeLength);
    AzToolsFramework::SetLinearManipulatorConeRadius(m_manipulators.m_linearManipulatorConeRadius);
    AzToolsFramework::SetScaleManipulatorBoxHalfExtent(m_manipulators.m_scaleManipulatorBoxHalfExtent);
    AzToolsFramework::SetRotationManipulatorRadius(m_manipulators.m_rotationManipulatorRadius);
    AzToolsFramework::SetFlipManipulatorAxesTowardsView(m_manipulators.m_flipManipulatorAxesTowardsView);
    AzToolsFramework::SetManipulatorViewBaseScale(m_manipulators.m_manipulatorViewBaseScale);
}

void CEditorPreferencesPage_ViewportManipulator::InitializeSettings()
{
    m_manipulators.m_manipulatorLineBoundWidth = SandboxEditor::ManipulatorLineBoundWidth();
    m_manipulators.m_manipulatorCircleBoundWidth = SandboxEditor::ManipulatorCircleBoundWidth();

    m_manipulators.m_linearManipulatorAxisLength = AzToolsFramework::LinearManipulatorAxisLength();
    m_manipulators.m_planarManipulatorAxisLength = AzToolsFramework::PlanarManipulatorAxisLength();
    m_manipulators.m_surfaceManipulatorRadius = AzToolsFramework::SurfaceManipulatorRadius();
    m_manipulators.m_surfaceManipulatorOpacity = AzToolsFramework::SurfaceManipulatorOpacity();
    m_manipulators.m_linearManipulatorConeLength = AzToolsFramework::LinearManipulatorConeLength();
    m_manipulators.m_linearManipulatorConeRadius = AzToolsFramework::LinearManipulatorConeRadius();
    m_manipulators.m_scaleManipulatorBoxHalfExtent = AzToolsFramework::ScaleManipulatorBoxHalfExtent();
    m_manipulators.m_rotationManipulatorRadius = AzToolsFramework::RotationManipulatorRadius();
    m_manipulators.m_flipManipulatorAxesTowardsView = AzToolsFramework::FlipManipulatorAxesTowardsView();
    m_manipulators.m_manipulatorViewBaseScale = AzToolsFramework::ManipulatorViewBaseScale();
}
