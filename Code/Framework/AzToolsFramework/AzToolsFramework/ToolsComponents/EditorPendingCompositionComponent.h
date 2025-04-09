/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "EditorComponentBase.h"
#include "EditorPendingCompositionBus.h"

#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace AzToolsFramework
{
    namespace Components
    {
        //! Custom serializer to handle component data in the composition component.
        class EditorPendingCompositionComponentSerializer
            : public AZ::BaseJsonSerializer
        {
        public:
            AZ_RTTI(EditorPendingCompositionComponentSerializer, "{9084611C-7011-4906-9EE6-EF10019ABADD}", BaseJsonSerializer);
            AZ_CLASS_ALLOCATOR_DECL;

            AZ::JsonSerializationResult::Result Load(
                void* outputValue,
                const AZ::Uuid& outputValueTypeId,
                const rapidjson::Value& inputValue,
                AZ::JsonDeserializerContext& context) override;
        };

        //! Contains pending components to be added to the entity we are attached to.
        class EditorPendingCompositionComponent
            : public AzToolsFramework::Components::EditorComponentBase
            , public EditorPendingCompositionRequestBus::Handler
        {
            friend class EditorPendingCompositionComponentSerializer;

        public:
            AZ_COMPONENT(EditorPendingCompositionComponent, "{D40FCB35-153D-45B3-AF6D-7BA576D8AFBB}", EditorComponentBase);
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            ////////////////////////////////////////////////////////////////////
            // EditorPendingCompositionRequestBus
            void GetPendingComponents(AZ::Entity::ComponentArrayType& components) override;
            void AddPendingComponent(AZ::Component* componentToAdd) override;
            void RemovePendingComponent(AZ::Component* componentToRemove) override;
            bool IsComponentPending(const AZ::Component* componentToCheck) override;
            ////////////////////////////////////////////////////////////////////

            ~EditorPendingCompositionComponent() override;
        private:
            ////////////////////////////////////////////////////////////////////
            // AZ::Entity
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            ////////////////////////////////////////////////////////////////////

            // Map that stores a pair of component alias (serialized identifier) and component pointer.
            AZStd::unordered_map<AZStd::string, AZ::Component*> m_pendingComponents;
        };
    } // namespace Components
} // namespace AzToolsFramework
