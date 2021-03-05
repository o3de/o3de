/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "LmbrCentral_precompiled.h"
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
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/AudioEnvironment.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/AudioEnvironment.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-audio-environment.html")
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
