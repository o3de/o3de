/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "EditorPreferencesPageViewportManipulator.h"

#include <AzToolsFramework/Viewport/ViewportSettings.h>

// Editor
#include "EditorViewportSettings.h"
#include "Settings.h"

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
        editContext->Class<Manipulators>("Manipulators", "")
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &Manipulators::m_manipulatorLineBoundWidth, "Line Bound Width",
                "Manipulator Line Bound Width")
            ->Attribute(AZ::Edit::Attributes::Min, 0.001f)
            ->Attribute(AZ::Edit::Attributes::Max, 2.0f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &Manipulators::m_manipulatorCircleBoundWidth, "Circle Bound Width",
                "Manipulator Circle Bound Width")
            ->Attribute(AZ::Edit::Attributes::Min, 0.001f)
            ->Attribute(AZ::Edit::Attributes::Max, 2.0f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &Manipulators::m_linearManipulatorAxisLength, "Linear Manipulator Axis Length",
                "Length of default Linear Manipulator (for Translation and Scale Manipulators)")
            ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
            ->Attribute(AZ::Edit::Attributes::Max, 5.0f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &Manipulators::m_planarManipulatorAxisLength, "Planar Manipulator Axis Length",
                "Length of default Planar Manipulator (for Translation Manipulators)")
            ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
            ->Attribute(AZ::Edit::Attributes::Max, 5.0f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &Manipulators::m_surfaceManipulatorRadius, "Surface Manipulator Radius",
                "Radius of default Surface Manipulator (for Translation Manipulators)")
            ->Attribute(AZ::Edit::Attributes::Min, 0.05f)
            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &Manipulators::m_surfaceManipulatorOpacity, "Surface Manipulator Opacity",
                "Opacity of default Surface Manipulator (for Translation Manipulators)")
            ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &Manipulators::m_linearManipulatorConeLength, "Linear Manipulator Cone Length",
                "Length of cone for default Linear Manipulator (for Translation Manipulators)")
            ->Attribute(AZ::Edit::Attributes::Min, 0.05f)
            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &Manipulators::m_linearManipulatorConeRadius, "Linear Manipulator Cone Radius",
                "Radius of cone for default Linear Manipulator (for Translation Manipulators)")
            ->Attribute(AZ::Edit::Attributes::Min, 0.05f)
            ->Attribute(AZ::Edit::Attributes::Max, 0.5f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &Manipulators::m_scaleManipulatorBoxHalfExtent, "Scale Manipulator Box Half Extent",
                "Half extent of box for default Scale Manipulator")
            ->Attribute(AZ::Edit::Attributes::Min, 0.05f)
            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &Manipulators::m_rotationManipulatorRadius, "Rotation Manipulator Radius",
                "Radius of default Angular Manipulators (for Rotation Manipulators)")
            ->Attribute(AZ::Edit::Attributes::Min, 0.5f)
            ->Attribute(AZ::Edit::Attributes::Max, 5.0f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &Manipulators::m_manipulatorViewBaseScale, "Manipulator View Base Scale",
                "The base scale to apply to all Manipulator Views (default is 1.0)")
            ->Attribute(AZ::Edit::Attributes::Min, 0.5f)
            ->Attribute(AZ::Edit::Attributes::Max, 2.0f)
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &Manipulators::m_flipManipulatorAxesTowardsView, "Flip Manipulator Axes Towards View",
                "Determines whether Planar and Linear Manipulators should switch to face the view (camera) in the Editor");

        editContext
            ->Class<CEditorPreferencesPage_ViewportManipulator>("Manipulator Viewport Preferences", "Manipulator Viewport Preferences")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
            ->DataElement(
                AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_ViewportManipulator::m_manipulators, "Manipulators", "Manipulators");
    }
}

CEditorPreferencesPage_ViewportManipulator::CEditorPreferencesPage_ViewportManipulator()
{
    InitializeSettings();
    m_icon = QIcon(":/res/Gizmos.svg");
}

const char* CEditorPreferencesPage_ViewportManipulator::GetCategory()
{
    return "Viewports";
}

const char* CEditorPreferencesPage_ViewportManipulator::GetTitle()
{
    return "Manipulators";
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
