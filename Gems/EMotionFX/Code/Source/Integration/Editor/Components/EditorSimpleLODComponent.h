/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Asset/AssetCommon.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <Integration/Components/SimpleLODComponent.h>
#include <Integration/Components/ActorComponent.h>


namespace EMotionFX
{
    namespace Integration
    {
        class EditorSimpleLODComponent
            : public AzToolsFramework::Components::EditorComponentBase
            , private AZ::Data::AssetBus::Handler
            , private AZ::TickBus::Handler
            , private ActorComponentNotificationBus::Handler
        {
        public:

            AZ_EDITOR_COMPONENT(EditorSimpleLODComponent, "{2A78936A-FA43-41C5-89C4-B588ED45DE2F}");

            EditorSimpleLODComponent();
            ~EditorSimpleLODComponent() override;

            // AZ::Component interface implementation
            void Activate() override;
            void Deactivate() override;

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                SimpleLODComponent::GetProvidedServices(provided);
            }

            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
            {
                SimpleLODComponent::GetDependentServices(dependent);
            }

            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
            {
                SimpleLODComponent::GetRequiredServices(required);
            }

            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
            {
                SimpleLODComponent::GetIncompatibleServices(incompatible);
            }

            static void Reflect(AZ::ReflectContext* context);
            static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

        private:
            EditorSimpleLODComponent(const EditorSimpleLODComponent&) = delete;
            // ActorComponentNotificationBus::Handler
            void OnActorInstanceCreated(EMotionFX::ActorInstance* actorInstance) override;
            void OnActorInstanceDestroyed(EMotionFX::ActorInstance* actorInstance) override;

            // AZ::TickBus::Handler
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            void BuildGameEntity(AZ::Entity* gameEntity) override;

            EMotionFX::ActorInstance*                   m_actorInstance;        // Associated actor instance (retrieved from Actor Component).
            SimpleLODComponent::Configuration           m_configuration;
        };
    }
}
