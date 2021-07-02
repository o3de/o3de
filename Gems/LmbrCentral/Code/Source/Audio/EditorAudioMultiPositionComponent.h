/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include "AudioMultiPositionComponent.h"

namespace LmbrCentral
{
    /*!
     * EditorAudioMultiPositionComponent
     * \ref AudioMultiPositionComponent
     */
    class EditorAudioMultiPositionComponent
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(EditorAudioMultiPositionComponent, "{0991631B-38B5-4CE0-AA51-6CC4448D0A2D}",
            AzToolsFramework::Components::EditorComponentBase);
        EditorAudioMultiPositionComponent() = default;

        void BuildGameEntity(AZ::Entity* gameEntity) override;

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            LmbrCentral::AudioMultiPositionComponent::GetDependentServices(dependent);
        }

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            LmbrCentral::AudioMultiPositionComponent::GetProvidedServices(provided);
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            LmbrCentral::AudioMultiPositionComponent::GetRequiredServices(required);
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            LmbrCentral::AudioMultiPositionComponent::GetIncompatibleServices(incompatible);
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        //! Serialized Data
        AZStd::vector<AZ::EntityId> m_entityRefs;
        Audio::MultiPositionBehaviorType m_behaviorType;
    };

} // namespace LmbrCentral
