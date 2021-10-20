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
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorDisabledCompositionBus.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionBus.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzFramework/Entity/BehaviorEntity.h>

namespace AzToolsFramework
{
    struct ComponentDetails
    {
        AZ_TYPE_INFO(AzToolsFramework::ComponentDetails, "{107D8379-4AD4-4547-BEE1-184B120F23E9}");

        static void Reflect(AZ::ReflectContext* context);

        AZStd::string m_typeInfo;
        AZStd::vector<AZStd::string> m_baseClasses;
    };

    // This ebus is intended to provide behavior-context friendly APIs to create and manage entities
    struct EntityUtilityTraits : AZ::EBusTraits
    {
        AZ_RTTI(AzToolsFramework::EntityUtilityTraits, "{A6305CAE-C825-43F9-A44D-E503910912AF}");

        virtual ~EntityUtilityTraits() = default;

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        // Creates an entity with the default editor components attached and initializes the entity
        virtual AZ::EntityId CreateEditorReadyEntity(const AZStd::string& entityName) = 0;

        virtual AzFramework::BehaviorComponentId GetOrAddComponentByTypeName(AZ::EntityId entity, const AZStd::string& typeName) = 0;

        virtual bool UpdateComponentForEntity(AZ::EntityId entity, AzFramework::BehaviorComponentId component, const AZStd::string& json) = 0;

        // Gets a JSON string containing describing the default serialization state of the specified component
        virtual AZStd::string GetComponentDefaultJson(const AZStd::string& typeName) = 0;

        // Returns a list of matching component type names.  Supports wildcard search terms
        virtual AZStd::vector<ComponentDetails> FindMatchingComponents(const AZStd::string& searchTerm) = 0;

        virtual void ResetEntityContext() = 0;
    };

    using EntityUtilityBus = AZ::EBus<EntityUtilityTraits>;

    struct EntityUtilityComponent : AZ::Component
        , EntityUtilityBus::Handler
    {
        inline const static AZ::Uuid UtilityEntityContextId = AZ::Uuid("{9C277B88-E79E-4F8A-BAFF-A4C175BD565F}");

        AZ_COMPONENT(EntityUtilityComponent, "{47205907-A0EA-4FFF-A620-04D20C04A379}");

        AZ::EntityId CreateEditorReadyEntity(const AZStd::string& entityName) override;
        AzFramework::BehaviorComponentId GetOrAddComponentByTypeName(AZ::EntityId entity, const AZStd::string& typeName) override;
        bool UpdateComponentForEntity(AZ::EntityId entity, AzFramework::BehaviorComponentId component, const AZStd::string& json) override;
        AZStd::string GetComponentDefaultJson(const AZStd::string& typeName) override;
        AZStd::vector<ComponentDetails> FindMatchingComponents(const AZStd::string& searchTerm) override;
        void ResetEntityContext() override;

        static void Reflect(AZ::ReflectContext* context);

    protected:
        void Activate() override;
        void Deactivate() override;

        // Our own entity context.  This API is intended mostly for use in Asset Builders where there is no editor context
        // Additionally, an entity context is needed when using the Behavior Entity class
        AZStd::unique_ptr<AzFramework::EntityContext> m_entityContext;

        // TypeId, TypeName, Vector<BaseClassName>
        AZStd::vector<AZStd::tuple<AZ::TypeId, AZStd::string, AZStd::vector<AZStd::string>>> m_typeInfo;

        // Keep track of the entities we create so they can be reset
        AZStd::vector<AZ::EntityId> m_createdEntities;
    };
}; // namespace AzToolsFramework
