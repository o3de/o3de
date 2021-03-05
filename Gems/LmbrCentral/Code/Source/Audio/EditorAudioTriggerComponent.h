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

#include <IAudioSystem.h>
#include <AzCore/std/string/string.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <AzToolsFramework/UI/PropertyEditor/PropertyAudioCtrlTypes.h>

#include "AudioTriggerComponent.h"

namespace LmbrCentral
{
    /*!
     * EditorAudioTriggerComponent
     */
    class EditorAudioTriggerComponent
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(EditorAudioTriggerComponent, "{E8A7656C-6146-427C-B592-25514EEEF841}",
            AzToolsFramework::Components::EditorComponentBase);

        EditorAudioTriggerComponent();

        void BuildGameEntity(AZ::Entity* gameEntity) override;

        void SetObstructionType(Audio::ObstructionType) {}

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            LmbrCentral::AudioTriggerComponent::GetProvidedServices(provided);
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            LmbrCentral::AudioTriggerComponent::GetRequiredServices(required);
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            LmbrCentral::AudioTriggerComponent::GetIncompatibleServices(incompatible);
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        //! Serialized data
        AzToolsFramework::CReflectedVarAudioControl m_defaultPlayTrigger;
        AzToolsFramework::CReflectedVarAudioControl m_defaultStopTrigger;
        Audio::ObstructionType m_obstructionType = Audio::ObstructionType::Ignore;
        bool m_playsImmediately = false;
        bool m_notifyWhenTriggerFinishes = false;
    };

} // namespace LmbrCentral
