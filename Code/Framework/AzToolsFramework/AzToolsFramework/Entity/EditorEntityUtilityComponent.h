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

namespace AzToolsFramework
{
    // This ebus is intended to provide behavior-context friendly APIs to create and manage entities
    struct EditorEntityUtilityTraits : AZ::EBusTraits
    {
        virtual ~EditorEntityUtilityTraits() = default;

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        // Creates an entity with the default editor components attached and initializes the entity
        virtual AZ::EntityId CreateEditorReadyEntity(const AZStd::string& entityName) = 0;
    };

    using EditorEntityUtilityBus = AZ::EBus<EditorEntityUtilityTraits>;

    struct EditorEntityUtilityComponent : AZ::Component
        , EditorEntityUtilityBus::Handler
    {
        inline const static AZ::Uuid UtilityEntityContextId = AZ::Uuid("{9C277B88-E79E-4F8A-BAFF-A4C175BD565F}");

        AZ_COMPONENT(EditorEntityUtilityComponent, "{47205907-A0EA-4FFF-A620-04D20C04A379}");

        AZ::EntityId CreateEditorReadyEntity(const AZStd::string& entityName) override;
        static void Reflect(AZ::ReflectContext* context);

    protected:
        void Activate() override;
        void Deactivate() override;

        // Our own entity context.  This API is intended mostly for use in Asset Builders where there is no editor context
        // Additionally, an entity context is needed when using the Behavior Entity class
        AZStd::unique_ptr<AzFramework::EntityContext> m_entityContext;
    };
}; // namespace AzToolsFramework
