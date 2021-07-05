/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "EditorPreferencesPageViewportGizmo.h"

// Editor
#include "Settings.h"


void CEditorPreferencesPage_ViewportGizmo::Reflect(AZ::SerializeContext& serialize)
{
    serialize.Class<AxisGizmo>()
        ->Version(1)
        ->Field("Size", &AxisGizmo::m_size)
        ->Field("Text", &AxisGizmo::m_text)
        ->Field("MaxCount", &AxisGizmo::m_maxCount);

    serialize.Class<Helpers>()
        ->Version(1)
        ->Field("LabelsOn", &Helpers::m_helpersGlobalScale)
        ->Field("LabelsOn", &Helpers::m_tagpointScaleMulti)
        ->Field("LabelsOn", &Helpers::m_rulerSphereScale)
        ->Field("LabelsDistance", &Helpers::m_rulerSphereTrans);

    serialize.Class<CEditorPreferencesPage_ViewportGizmo>()
        ->Version(1)
        ->Field("Axis Gizmo", &CEditorPreferencesPage_ViewportGizmo::m_axisGizmo)
        ->Field("Helpers", &CEditorPreferencesPage_ViewportGizmo::m_helpers);

    AZ::EditContext* editContext = serialize.GetEditContext();
    if (editContext)
    {
        editContext->Class<AxisGizmo>("Axis Gizmo", "")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &AxisGizmo::m_size, "Size", "Axis Gizmo Size")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &AxisGizmo::m_text, "Text Labels", "Text Labels on Axis Gizmo")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &AxisGizmo::m_maxCount, "Max Count", "Max Count of Axis Gizmos");

        editContext->Class<Helpers>("Helpers", "")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &Helpers::m_helpersGlobalScale, "Helpers Scale", "Helpers Scale")
                ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &Helpers::m_tagpointScaleMulti, "Tagpoint Scale Multiplier", "Tagpoint Scale Multiplier")
                ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &Helpers::m_rulerSphereScale, "Ruler Sphere Scale", "Ruler Sphere Scale")
                ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
                ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &Helpers::m_rulerSphereTrans, "Ruler Sphere Transparency", "Ruler Sphere Transparency")
                ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                ->Attribute(AZ::Edit::Attributes::Max, 100.0f);

        editContext->Class<CEditorPreferencesPage_ViewportGizmo>("Gizmo Viewport Preferences", "Gizmo Viewport Preferences")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_ViewportGizmo::m_axisGizmo, "Axis Gizmo", "Axis Gizmo")
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_ViewportGizmo::m_helpers, "Helpers", "Helpers");
    }
}

CEditorPreferencesPage_ViewportGizmo::CEditorPreferencesPage_ViewportGizmo()
{
    InitializeSettings();
    m_icon = QIcon(":/res/Gizmos.svg");
}

QIcon& CEditorPreferencesPage_ViewportGizmo::GetIcon()
{
    return m_icon;
}

void CEditorPreferencesPage_ViewportGizmo::OnApply()
{
    gSettings.gizmo.axisGizmoSize = m_axisGizmo.m_size;
    gSettings.gizmo.axisGizmoText = m_axisGizmo.m_text;
    gSettings.gizmo.axisGizmoMaxCount = m_axisGizmo.m_maxCount;

    gSettings.gizmo.helpersScale = m_helpers.m_helpersGlobalScale;
    gSettings.gizmo.tagpointScaleMulti = m_helpers.m_tagpointScaleMulti;
    gSettings.gizmo.rulerSphereScale = m_helpers.m_rulerSphereScale;
    gSettings.gizmo.rulerSphereTrans = m_helpers.m_rulerSphereTrans;
}

void CEditorPreferencesPage_ViewportGizmo::InitializeSettings()
{
    m_axisGizmo.m_size = gSettings.gizmo.axisGizmoSize;
    m_axisGizmo.m_text = gSettings.gizmo.axisGizmoText;
    m_axisGizmo.m_maxCount = gSettings.gizmo.axisGizmoMaxCount;

    m_helpers.m_helpersGlobalScale = gSettings.gizmo.helpersScale;
    m_helpers.m_tagpointScaleMulti = gSettings.gizmo.tagpointScaleMulti;
    m_helpers.m_rulerSphereScale = gSettings.gizmo.rulerSphereScale;
    m_helpers.m_rulerSphereTrans = gSettings.gizmo.rulerSphereTrans;
}
