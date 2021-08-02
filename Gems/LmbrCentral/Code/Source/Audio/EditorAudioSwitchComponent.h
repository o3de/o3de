/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <IAudioInterfacesCommonData.h>

#include <AzToolsFramework/UI/PropertyEditor/PropertyAudioCtrlTypes.h>

#include "AudioSwitchComponent.h"

namespace LmbrCentral
{
    /*!
     * EditorAudioSwitchComponent
     */
    class EditorAudioSwitchComponent
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(EditorAudioSwitchComponent, "{804E49F9-BD9A-402A-96D1-C666E81984E6}",
            AzToolsFramework::Components::EditorComponentBase);

        EditorAudioSwitchComponent();

        void BuildGameEntity(AZ::Entity* gameEntity) override;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            LmbrCentral::AudioSwitchComponent::GetProvidedServices(provided);
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            LmbrCentral::AudioSwitchComponent::GetRequiredServices(required);
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            LmbrCentral::AudioSwitchComponent::GetIncompatibleServices(incompatible);
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        //! Serialized data
        AzToolsFramework::CReflectedVarAudioControl m_defaultSwitch;
        AzToolsFramework::CReflectedVarAudioControl m_defaultState;
    };

} // namespace LmbrCentral
