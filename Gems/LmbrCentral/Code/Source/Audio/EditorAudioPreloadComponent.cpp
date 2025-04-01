/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorAudioPreloadComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>


namespace LmbrCentral
{
    //=========================================================================
    void EditorAudioPreloadComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorAudioPreloadComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("Preload Name", &EditorAudioPreloadComponent::m_defaultPreload)
                ->Field("Load Type", &EditorAudioPreloadComponent::m_loadType)
                ;

            serializeContext->Enum<AudioPreloadComponent::LoadType>()
                ->Value("Auto", AudioPreloadComponent::LoadType::Auto)
                ->Value("Manual", AudioPreloadComponent::LoadType::Manual)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Enum<AudioPreloadComponent::LoadType>("Load Type", "Automatic or Manual loading and unloading")
                    ->Value("Auto", AudioPreloadComponent::LoadType::Auto)
                    ->Value("Manual", AudioPreloadComponent::LoadType::Manual)
                    ;

                editContext->Class<EditorAudioPreloadComponent>("Audio Preload", "The Audio Preload component is used to load and unload soundbanks contained in Audio Translation Layer (ATL) preloads")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Audio")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/AudioPreload.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/AudioPreload.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                        // Icon todo:
                        //->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/AudioPreload.png")

                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/audio/preload/")

                    ->DataElement("AudioControl", &EditorAudioPreloadComponent::m_defaultPreload, "Preload Name", "The default ATL Preload control to use")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &EditorAudioPreloadComponent::m_loadType, "Load Type", "Automatically when the component activates/deactivates, or Manually at user's request")
                    ;
            }
        }
    }

    //=========================================================================
    EditorAudioPreloadComponent::EditorAudioPreloadComponent()
    {
        m_defaultPreload.m_propertyType = AzToolsFramework::AudioPropertyType::Preload;
    }

    //=========================================================================
    void EditorAudioPreloadComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<AudioPreloadComponent>(m_loadType, m_defaultPreload.m_controlName);
    }

} // namespace LmbrCentral
