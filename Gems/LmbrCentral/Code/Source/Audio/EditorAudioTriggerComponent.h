/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
    };

} // namespace LmbrCentral
