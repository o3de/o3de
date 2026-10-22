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
#include <AzFramework/Translation/TranslationDef.h>

// Editor
#include "Settings.h"

namespace EditorPreferencesViewportDebugStrings
{
    static const char* ProfilingClassName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportDebug", "Profiling");
    static const char* ShowMeshStatsName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportDebug", "Show Mesh Statistics");
    static const char* ShowMeshStatsDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportDebug", "Show Mesh Statistics on Mouse Over");

    static const char* ViewportWarningSettingsClassName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportDebug", "Viewport Warning Settings");
    static const char* WarningIconsDistanceName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportDebug", "Warning Icons Draw Distance");
    static const char* ShowScaleWarningsName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportDebug", "Show Scale Warnings");
    static const char* ShowRotationWarningsName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportDebug", "Show Rotation Warnings");

    static const char* ViewportDebugPreferencesClassName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportDebug", "Viewport Debug Preferences");
}

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
        using namespace EditorPreferencesViewportDebugStrings;

        editContext->Class<Profiling>(ProfilingClassName, ProfilingClassName)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Profiling::m_showMeshStatsOnMouseOver, ShowMeshStatsName, ShowMeshStatsDesc);

        editContext->Class<Warnings>(ViewportWarningSettingsClassName, "")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &Warnings::m_warningIconsDrawDistance, WarningIconsDistanceName, WarningIconsDistanceName)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Warnings::m_showScaleWarnings, ShowScaleWarningsName, ShowScaleWarningsName)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Warnings::m_showRotationWarnings, ShowRotationWarningsName, ShowRotationWarningsName);

        editContext->Class<CEditorPreferencesPage_ViewportDebug>(ViewportDebugPreferencesClassName, ViewportDebugPreferencesClassName)
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC_CE("PropertyVisibility_ShowChildrenOnly"))
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_ViewportDebug::m_profiling, ProfilingClassName, ProfilingClassName)
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_ViewportDebug::m_warnings, ViewportWarningSettingsClassName, ViewportWarningSettingsClassName);
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
