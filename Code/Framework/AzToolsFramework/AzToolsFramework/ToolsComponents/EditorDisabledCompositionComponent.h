/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "EditorComponentBase.h"
#include "EditorDisabledCompositionBus.h"

#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace AzToolsFramework
{
    namespace Components
    {
        //! Custom serializer to handle component data in the composition component.
        class EditorDisabledCompositionComponentSerializer
            : public AZ::BaseJsonSerializer
        {
        public:
            AZ_RTTI(EditorDisabledCompositionComponentSerializer, "{D6A44ED5-2B3B-422C-A4F2-DDDAE48ED04C}", BaseJsonSerializer);
            AZ_CLASS_ALLOCATOR_DECL;
            
            AZ::JsonSerializationResult::Result Load(
                void* outputValue,
                const AZ::Uuid& outputValueTypeId,
                const rapidjson::Value& inputValue,
                AZ::JsonDeserializerContext& context) override;
        };

        //! Contains Disabled components to be added to the entity we are attached to.
        class EditorDisabledCompositionComponent
            : public AzToolsFramework::Components::EditorComponentBase
            , public EditorDisabledCompositionRequestBus::Handler
        {
            friend class EditorDisabledCompositionComponentSerializer;

        public:
            AZ_COMPONENT(EditorDisabledCompositionComponent, "{E77AE6AC-897D-4035-8353-637449B6DCFB}", EditorComponentBase);
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            ////////////////////////////////////////////////////////////////////
            // EditorDisabledCompositionRequestBus
            void GetDisabledComponents(AZ::Entity::ComponentArrayType& components) override;
            void AddDisabledComponent(AZ::Component* componentToAdd) override;
            void RemoveDisabledComponent(AZ::Component* componentToRemove) override;
            bool IsComponentDisabled(const AZ::Component* componentToCheck) override;
            ////////////////////////////////////////////////////////////////////

            ~EditorDisabledCompositionComponent() override;
        private:
            ////////////////////////////////////////////////////////////////////
            // AZ::Entity
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            ////////////////////////////////////////////////////////////////////

            // Map that stores a pair of component alias (serialized identifier) and component pointer.
            AZStd::unordered_map<AZStd::string, AZ::Component*> m_disabledComponents;
        };
    } // namespace Components
} // namespace AzToolsFramework
