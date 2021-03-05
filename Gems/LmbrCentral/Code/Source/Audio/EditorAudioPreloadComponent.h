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
#pragma once

#include <AzCore/std/string/string.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyAudioCtrlTypes.h>

#include <IAudioInterfacesCommonData.h>

#include "AudioPreloadComponent.h"

namespace LmbrCentral
{
    /*!
     * EditorAudioPreloadComponent
     */
    class EditorAudioPreloadComponent
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(EditorAudioPreloadComponent, "{58E20F92-2228-4A90-97AB-28DB34BAF0EE}",
            AzToolsFramework::Components::EditorComponentBase);

        EditorAudioPreloadComponent();

        void BuildGameEntity(AZ::Entity* gameEntity) override;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            LmbrCentral::AudioPreloadComponent::GetProvidedServices(provided);
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            LmbrCentral::AudioPreloadComponent::GetRequiredServices(required);
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            LmbrCentral::AudioPreloadComponent::GetIncompatibleServices(incompatible);
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        //! Serialized data
        AzToolsFramework::CReflectedVarAudioControl m_defaultPreload;
        AudioPreloadComponent::LoadType m_loadType = AudioPreloadComponent::LoadType::Auto;
    };

} // namespace LmbrCentral
