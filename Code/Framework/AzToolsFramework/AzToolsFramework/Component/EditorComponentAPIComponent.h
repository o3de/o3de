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
#include <AzToolsFramework/Component/EditorComponentAPIBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

namespace AzToolsFramework
{
    namespace Components
    {
        //! A System Component to reflect Editor operations on Components to Behavior Context
        class EditorComponentAPIComponent
            : public AZ::Component
            , public EditorComponentAPIBus::Handler
        {
        public:
            AZ_COMPONENT(EditorComponentAPIComponent, "{AC1A53C9-25BE-47D8-B9B5-60199AC73C2B}");

            EditorComponentAPIComponent() = default;
            ~EditorComponentAPIComponent() = default;

            static void Reflect(AZ::ReflectContext* context);

            // Component ...
            void Activate() override;
            void Deactivate() override;

            // EditorComponentAPIBus ...
            AZStd::vector<AZ::Uuid> FindComponentTypeIdsByEntityType(const AZStd::vector<AZStd::string>& componentTypeNames, EditorComponentAPIRequests::EntityType entityType) override;
            AZStd::vector<AZ::Uuid> FindComponentTypeIdsByService(const AZStd::vector<AZ::ComponentServiceType>& serviceFilter, const AZStd::vector<AZ::ComponentServiceType>& incompatibleServiceFilter) override;
            AZStd::vector<AZStd::string> FindComponentTypeNames(const AZ::ComponentTypeList& componentTypeIds) override;
            AZStd::vector<AZStd::string> BuildComponentTypeNameListByEntityType(EditorComponentAPIRequests::EntityType entityType) override;

            AddComponentsOutcome AddComponentsOfType(AZ::EntityId entityId, const AZ::ComponentTypeList& componentTypeIds) override;
            AddComponentsOutcome AddComponentOfType(AZ::EntityId entityId, const AZ::Uuid& componentTypeId) override;
            bool HasComponentOfType(AZ::EntityId entityId, AZ::Uuid componentTypeId) override;
            size_t CountComponentsOfType(AZ::EntityId entityId, AZ::Uuid componentTypeId) override;
            GetComponentOutcome GetComponentOfType(AZ::EntityId entityId, AZ::Uuid componentTypeId) override;
            GetComponentsOutcome GetComponentsOfType(AZ::EntityId entityId, AZ::Uuid componentTypeId) override;

            bool IsValid(AZ::EntityComponentIdPair componentInstance) override;
            bool EnableComponents(const AZStd::vector<AZ::EntityComponentIdPair>& componentInstances) override;
            bool IsComponentEnabled(const AZ::EntityComponentIdPair& componentInstance) override;
            bool DisableComponents(const AZStd::vector<AZ::EntityComponentIdPair>& componentInstances) override;
            bool RemoveComponents(const AZStd::vector<AZ::EntityComponentIdPair>& componentInstances) override;

            PropertyTreeOutcome BuildComponentPropertyTreeEditor(const AZ::EntityComponentIdPair& componentInstance) override;
            PropertyOutcome GetComponentProperty(const AZ::EntityComponentIdPair& componentInstance, const AZStd::string_view propertyPath) override;
            PropertyOutcome SetComponentProperty(const AZ::EntityComponentIdPair& componentInstance, const AZStd::string_view propertyPath, const AZStd::any& value) override;
            bool CompareComponentProperty(const AZ::EntityComponentIdPair& componentInstance, const AZStd::string_view propertyPath, const AZStd::any& value) override;
            const AZStd::vector<AZStd::string> BuildComponentPropertyList(const AZ::EntityComponentIdPair& componentInstance) override;
            void SetVisibleEnforcement(bool enforceVisiblity) override;

        private:
            AZ::Entity* FindEntity(AZ::EntityId entityId);
            AZ::Component* FindComponent(AZ::EntityId entityId, AZ::ComponentId componentId);
            AZ::Component* FindComponent(AZ::EntityId entityId, AZ::Uuid componentType);
            AZStd::vector<AZ::Component*> FindComponents(AZ::EntityId entityId, AZ::Uuid componentType);

            bool m_usePropertyVisibility = false;
            AZ::SerializeContext* m_serializeContext = nullptr;
        };
    } // Components
} // AzToolsFramework
