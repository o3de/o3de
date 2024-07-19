/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorLevelSettingsComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace Vegetation
{
    void EditorLevelSettingsComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClassType::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<EditorLevelSettingsComponent, BaseClassType>()
                ->Version(1, &LmbrCentral::EditorWrappedComponentBaseVersionConverter<typename BaseClassType::WrappedComponentType, typename BaseClassType::WrappedConfigType, 1>)
                ->Field("UseEditorMaxInstanceProcessTimeMicroseconds", &EditorLevelSettingsComponent::m_useEditorMaxInstanceProcessTimeMicroseconds)
                ->Field("EditorMaxInstanceProcessTimeMicroseconds", &EditorLevelSettingsComponent::m_editorMaxInstanceProcessTimeMicroseconds)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();

            if (editContext)
            {
                editContext->Class<EditorLevelSettingsComponent>(s_componentName, s_componentDescription)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, s_categoryName)
                        ->Attribute(AZ::Edit::Attributes::Icon, s_icon)
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, s_viewportIcon)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, s_helpUrl)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Level"))
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &EditorLevelSettingsComponent::m_useEditorMaxInstanceProcessTimeMicroseconds, "Override Instance Time Slicing", "Use a max instance process time (in microseconds) for the Editor")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorLevelSettingsComponent::ConfigurationChanged)
                    ->DataElement(0, &EditorLevelSettingsComponent::m_editorMaxInstanceProcessTimeMicroseconds, "Editor Instance Time Slicing", "The timeout maximum number of microseconds to process the vegetation instance construction & removal operations while in the Editor.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorLevelSettingsComponent::ConfigurationChanged)
                        ->Attribute(AZ::Edit::Attributes::Min, 0)
                        ->Attribute(AZ::Edit::Attributes::Max, 33000)
                    ;
            }
        }
    }

    AZ::u32 EditorLevelSettingsComponent::ConfigurationChanged()
    {
        m_component.Deactivate();
        if (m_useEditorMaxInstanceProcessTimeMicroseconds)
        {
            LevelSettingsConfig editorConfig = m_configuration;
            editorConfig.m_instanceSystemConfig.m_maxInstanceProcessTimeMicroseconds = m_editorMaxInstanceProcessTimeMicroseconds;
            m_component.ReadInConfig(&editorConfig);
        }
        else
        {
            m_component.ReadInConfig(&m_configuration);
        }

        if (m_visible && m_component.GetEntity())
        {
            m_component.Activate();
        }

        return AZ::Edit::PropertyRefreshLevels::None;
    }
} // namespace Vegetation
