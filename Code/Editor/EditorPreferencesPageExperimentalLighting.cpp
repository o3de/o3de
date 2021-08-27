/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "EditorPreferencesPageExperimentalLighting.h"

// AzCore
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzQtComponents/Components/StyleManager.h>

// Editor
#include "Settings.h"


void CEditorPreferencesPage_ExperimentalLighting::Reflect(AZ::SerializeContext& serialize)
{
    serialize.Class<Options>()
        ->Version(1)
        ->Field("TotalIlluminationEnabled", &Options::m_totalIlluminationEnabled);

    serialize.Class<CEditorPreferencesPage_ExperimentalLighting>()
        ->Version(1)
        ->Field("Options", &CEditorPreferencesPage_ExperimentalLighting::m_options);

    AZ::EditContext* editContext = serialize.GetEditContext();
    if (editContext)
    {
        editContext->Class<Options>("Options", "")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Options::m_totalIlluminationEnabled, "Total Illumination", "Enable Total Illumination");

        editContext->Class<CEditorPreferencesPage_ExperimentalLighting>("Experimental Features Preferences", "Experimental Features Preferences")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_ExperimentalLighting::m_options, "Options", "Experimental Features Options");
    }
}


CEditorPreferencesPage_ExperimentalLighting::CEditorPreferencesPage_ExperimentalLighting()
{
    InitializeSettings();

    m_icon = QIcon(":/res/Experimental.svg");
}

const char* CEditorPreferencesPage_ExperimentalLighting::GetTitle()
{
    return "Experimental Features";
}

QIcon& CEditorPreferencesPage_ExperimentalLighting::GetIcon()
{
    return m_icon;
}

void CEditorPreferencesPage_ExperimentalLighting::OnApply()
{
    gSettings.sExperimentalFeaturesSettings.bTotalIlluminationEnabled = m_options.m_totalIlluminationEnabled;
}

void CEditorPreferencesPage_ExperimentalLighting::InitializeSettings()
{
    m_options.m_totalIlluminationEnabled = gSettings.sExperimentalFeaturesSettings.bTotalIlluminationEnabled;
}
