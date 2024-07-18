/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorAudioEnvironmentComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace LmbrCentral
{
    //=========================================================================
    void EditorAudioEnvironmentComponent::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorAudioEnvironmentComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("Environment name", &EditorAudioEnvironmentComponent::m_defaultEnvironment)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorAudioEnvironmentComponent>("Audio Environment", "The Audio Environment component provides access to features of the Audio Translation Layer (ATL) environments to apply environmental effects such as reverb or echo")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Audio")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/AudioEnvironment.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/AudioEnvironment.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/audio/environment/")
                    ->DataElement("AudioControl", &EditorAudioEnvironmentComponent::m_defaultEnvironment, "Default Environment", "Name of the default ATL Environment control to use")
                    ;
            }
        }
    }

    //=========================================================================
    EditorAudioEnvironmentComponent::EditorAudioEnvironmentComponent()
    {
        m_defaultEnvironment.m_propertyType = AzToolsFramework::AudioPropertyType::Environment;
    }

    //=========================================================================
    void EditorAudioEnvironmentComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<AudioEnvironmentComponent>(m_defaultEnvironment.m_controlName);
    }

} // namespace LmbrCentral
