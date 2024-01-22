/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/Entity.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>

namespace AzToolsFramework
{
    namespace Components
    {
        /**
        * A system component for handling component-related actions on entities
        *
        * Used to perform things such as add/remove components, cut/copy/paste, etc.
        */
        class EditorEntityActionComponent
            : public AZ::Component
            , public EntityCompositionRequestBus::Handler
        {
        public:
            AZ_COMPONENT(EditorEntityActionComponent, "{2E26C7DF-544E-4A2A-8D0D-D7A6595C8BBD}");

            EditorEntityActionComponent() = default;
            virtual ~EditorEntityActionComponent() = default;

            //////////////////////////////////////////////////////////////////////////
            // AZ::Component overrides
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // EntityCompositionRequestBus::Handler
            AddComponentsOutcome AddComponentsToEntities(const EntityIdList& entityIds, const AZ::ComponentTypeList& componentsToAdd) override;
            AddExistingComponentsOutcome AddExistingComponentsToEntityById(const AZ::EntityId& entityId, AZStd::span<AZ::Component* const> componentsToAdd) override;
            RemoveComponentsOutcome RemoveComponents(AZStd::span<AZ::Component* const> componentsToRemove) override;
            ScrubEntitiesOutcome ScrubEntities(const EntityList& entities) override;

            void CutComponents(AZStd::span<AZ::Component* const> components) override;
            void CopyComponents(AZStd::span<AZ::Component* const> components) override;
            void PasteComponentsToEntity(AZ::EntityId entityId) override;
            bool HasComponentsToPaste() override;

            void EnableComponents(AZStd::span<AZ::Component* const> components) override;
            void DisableComponents(AZStd::span<AZ::Component* const> components) override;

            PendingComponentInfo GetPendingComponentInfo(const AZ::Component* component) override;

            AZStd::string GetComponentName(const AZ::Component* component) override;

        private:

            bool RemoveComponentFromEntityAndContainers(AZ::Entity* entity, AZ::Component* componentToRemove);

            ScrubEntityResults ScrubEntity(AZ::Entity* entity);

            /*!
            * Result of add component operations
            * Can either succeed and return a vector of the new components added
            * or fail with a string containing the reason for failure
            */
            using AddPendingComponentsOutcome = AZ::Outcome<AZ::Entity::ComponentArrayType, AZStd::string>;
            AddPendingComponentsOutcome AddPendingComponentsToEntity(AZ::Entity* entity);

            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        };
    } // namespace Components
} // namespace AzToolsFramework
