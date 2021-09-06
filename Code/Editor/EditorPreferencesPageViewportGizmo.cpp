/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "EditorPreferencesPageViewportGizmo.h"

// Editor
#include "EditorViewportSettings.h"
#include "Settings.h"

void CEditorPreferencesPage_ViewportManipulator::Reflect(AZ::SerializeContext& serialize)
{
    serialize.Class<Manipulators>()->Version(1)->Field("LineBoundWidth", &Manipulators::m_manipulatorLineBoundWidth);

    serialize.Class<CEditorPreferencesPage_ViewportManipulator>()->Version(2)->Field(
        "Manipulators", &CEditorPreferencesPage_ViewportManipulator::m_manipulators);

    if (AZ::EditContext* editContext = serialize.GetEditContext())
    {
        editContext->Class<Manipulators>("Manipulators", "")
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &Manipulators::m_manipulatorLineBoundWidth, "Line Bound Width",
                "Manipulator Line Bound Width")
            ->Attribute(AZ::Edit::Attributes::Min, 0.001f)
            ->Attribute(AZ::Edit::Attributes::Max, 2.0f);

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

QIcon& CEditorPreferencesPage_ViewportManipulator::GetIcon()
{
    return m_icon;
}

void CEditorPreferencesPage_ViewportManipulator::OnApply()
{
    SandboxEditor::SetManipulatorLineBoundWidth(m_manipulators.m_manipulatorLineBoundWidth);
}

void CEditorPreferencesPage_ViewportManipulator::InitializeSettings()
{
    m_manipulators.m_manipulatorLineBoundWidth = SandboxEditor::ManipulatorLineBoundWidth();
}
