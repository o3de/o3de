/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "EditorPreferencesPageViewportDebug.h"

// AzCore
#include <AzCore/Serialization/EditContext.h>

// Editor
#include "Settings.h"

void CEditorPreferencesPage_ViewportDebug::Reflect(AZ::SerializeContext& serialize)
{
    serialize.Class<Profiling>()
        ->Version(1)
        ->Field("ShowMeshStatsOnMouseOver", &Profiling::m_showMeshStatsOnMouseOver);

    serialize.Class<Warnings>()
        ->Version(1)
        ->Field("WarningIconsDrawDistance", &Warnings::m_warningIconsDrawDistance)
        ->Field("ShowScaleWarnings", &Warnings::m_showScaleWarnings)
        ->Field("ShowRotationWarnings", &Warnings::m_showRotationWarnings);

    serialize.Class<CEditorPreferencesPage_ViewportDebug>()
        ->Version(1)
        ->Field("Profiling", &CEditorPreferencesPage_ViewportDebug::m_profiling)
        ->Field("Warnings", &CEditorPreferencesPage_ViewportDebug::m_warnings);


    AZ::EditContext* editContext = serialize.GetEditContext();
    if (editContext)
    {
        editContext->Class<Profiling>("Profiling", "Profiling")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Profiling::m_showMeshStatsOnMouseOver, "Show Mesh Statistics", "Show Mesh Statistics on Mouse Over");

        editContext->Class<Warnings>("Viewport Warning Settings", "")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &Warnings::m_warningIconsDrawDistance, "Warning Icons Draw Distance", "Warning Icons Draw Distance")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Warnings::m_showScaleWarnings, "Show Scale Warnings", "Show Scale Warnings")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Warnings::m_showRotationWarnings, "Show Rotation Warnings", "Show Rotation Warnings");

        editContext->Class<CEditorPreferencesPage_ViewportDebug>("Viewport Debug Preferences", "Viewport Debug Preferences")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC_CE("PropertyVisibility_ShowChildrenOnly"))
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_ViewportDebug::m_profiling, "Profiling", "Profiling")
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_ViewportDebug::m_warnings, "Viewport Warning Settings", "Viewport Warning Settings");
    }
}


CEditorPreferencesPage_ViewportDebug::CEditorPreferencesPage_ViewportDebug()
{
    InitializeSettings();
    m_icon = QIcon(":/res/Debug.svg");
}

QIcon& CEditorPreferencesPage_ViewportDebug::GetIcon()
{
    return m_icon;
}

void CEditorPreferencesPage_ViewportDebug::OnApply()
{
    gSettings.viewports.bShowMeshStatsOnMouseOver = m_profiling.m_showMeshStatsOnMouseOver;
    gSettings.viewports.bShowRotationWarnings = m_warnings.m_showRotationWarnings;
    gSettings.viewports.bShowScaleWarnings = m_warnings.m_showScaleWarnings;
    gSettings.viewports.fWarningIconsDrawDistance = m_warnings.m_warningIconsDrawDistance;
}

void CEditorPreferencesPage_ViewportDebug::InitializeSettings()
{
    m_profiling.m_showMeshStatsOnMouseOver = gSettings.viewports.bShowMeshStatsOnMouseOver;
    m_warnings.m_showRotationWarnings = gSettings.viewports.bShowRotationWarnings;
    m_warnings.m_showScaleWarnings = gSettings.viewports.bShowScaleWarnings;
    m_warnings.m_warningIconsDrawDistance = gSettings.viewports.fWarningIconsDrawDistance;
}
