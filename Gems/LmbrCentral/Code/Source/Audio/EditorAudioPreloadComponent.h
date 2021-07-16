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
