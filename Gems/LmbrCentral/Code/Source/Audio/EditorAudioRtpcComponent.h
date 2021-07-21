/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <IAudioInterfacesCommonData.h>

#include <AzToolsFramework/UI/PropertyEditor/PropertyAudioCtrlTypes.h>

#include "AudioRtpcComponent.h"

namespace LmbrCentral
{
    /*!
     * EditorAudioRtpcComponent
     */
    class EditorAudioRtpcComponent
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(EditorAudioRtpcComponent, "{3942E34A-01EC-4EA3-8A83-7555323160B3}",
            AzToolsFramework::Components::EditorComponentBase);

        EditorAudioRtpcComponent();

        void BuildGameEntity(AZ::Entity* gameEntity) override;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            LmbrCentral::AudioRtpcComponent::GetProvidedServices(provided);
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            LmbrCentral::AudioRtpcComponent::GetRequiredServices(required);
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            LmbrCentral::AudioRtpcComponent::GetIncompatibleServices(incompatible);
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        //! Serialized data
        AzToolsFramework::CReflectedVarAudioControl m_defaultRtpc;
    };

} // namespace LmbrCentral
