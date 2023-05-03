/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorBaseComponentMode.h"
#include "ComponentModeViewportUiRequestBus.h"

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

DECLARE_EBUS_INSTANTIATION(AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests);
DECLARE_EBUS_INSTANTIATION_WITH_TRAITS(AzToolsFramework::ComponentModeFramework::ComponentModeDelegateRequests, AzToolsFramework::ComponentModeFramework::ComponentModeMouseViewportRequests)

namespace AzToolsFramework
{
    namespace ComponentModeFramework
    {
        AZ_CLASS_ALLOCATOR_IMPL(EditorBaseComponentMode, AZ::SystemAllocator)

        EditorBaseComponentMode::EditorBaseComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid componentType)
            : m_entityComponentIdPair(entityComponentIdPair), m_componentType(componentType)
        {
            const AZ::EntityId entityId = entityComponentIdPair.GetEntityId();
            AZ_Assert(entityId.IsValid(), "Attempting to create a Component Mode with an invalid EntityId");

            if (const AZ::Entity* entity = AzToolsFramework::GetEntity(entityId))
            {
                AZ_Assert(entity->GetState() == AZ::Entity::State::Active,
                    "Attempting to create a Component Mode for an Entity which is not currently active. "
                    "Its current state is %u", entity->GetState());
            }

            ComponentModeRequestBus::Handler::BusConnect(m_entityComponentIdPair);
            Prefab::PrefabPublicNotificationBus::Handler::BusConnect();
        }

        EditorBaseComponentMode::~EditorBaseComponentMode()
        {
            Prefab::PrefabPublicNotificationBus::Handler::BusDisconnect();
            ComponentModeRequestBus::Handler::BusDisconnect();
        }

        void EditorBaseComponentMode::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorBaseComponentMode>();
            }
        }

        void EditorBaseComponentMode::OnPrefabInstancePropagationEnd()
        {
            Refresh();
        }

        AZStd::vector<ViewportUi::ClusterId> EditorBaseComponentMode::PopulateViewportUi()
        {
            auto elementIdsToDisplay = PopulateViewportUiImpl();
            // register all of the elements in ComponentModeViewportUi system with corresponding EntityComponentIdPair
            ComponentModeViewportUiRequestBus::Event(
                GetComponentType(), &ComponentModeViewportUiRequestBus::Events::RegisterViewportElementGroup,
                GetEntityComponentIdPair(), elementIdsToDisplay);

            bool borderVisible = false;
            ViewportUi::ViewportUiRequestBus::EventResult(
                borderVisible,
                ViewportUi::DefaultViewportId,
                &ViewportUi::ViewportUiRequestBus::Events::GetViewportBorderVisible);

            // if the border is visible, change the border title
            // else create the component mode border with the specific name for this component mode
            if (borderVisible)
            {
                ViewportUi::ViewportUiRequestBus::Event(
                    ViewportUi::DefaultViewportId,
                    &ViewportUi::ViewportUiRequestBus::Events::ChangeViewportBorderText,
                    GetComponentModeName());
            }
            else
            {
                ViewportUi::ViewportUiRequestBus::Event(
                    ViewportUi::DefaultViewportId,
                    &ViewportUi::ViewportUiRequestBus::Events::CreateViewportBorder,
                    GetComponentModeName(),
                    []
                    {
                        ComponentModeSystemRequestBus::Broadcast(&ComponentModeSystemRequests::EndComponentMode);
                    });
            }

            // set the EntityComponentId for this ComponentMode to active in the ComponentModeViewportUi system
            ComponentModeViewportUiRequestBus::Event(
                GetComponentType(), &ComponentModeViewportUiRequestBus::Events::SetViewportUiActiveEntityComponentId,
                GetEntityComponentIdPair());
            return elementIdsToDisplay;
        }

        AZStd::vector<ViewportUi::ClusterId> EditorBaseComponentMode::PopulateViewportUiImpl()
        {
            return AZStd::vector<ViewportUi::ClusterId>();
        }

        AZStd::vector<ActionOverride> EditorBaseComponentMode::PopulateActionsImpl()
        {
            return AZStd::vector<ActionOverride>{};
        }

        AZStd::vector<ActionOverride> EditorBaseComponentMode::PopulateActions()
        {
            return PopulateActionsImpl();
        }

        AZ::Uuid EditorBaseComponentMode::GetComponentModeType() const
        {
            AZ_Assert(
                false,
                "Classes derived from EditorBaseComponentMode need to override this function and return their typeid."
                "Example: \"return azrtti_typeid<DerivedComponentMode>();\"");
            return AZ::Uuid::CreateNull();
        }

        void EditorBaseComponentMode::PostHandleMouseInteraction()
        {
            ComponentModeViewportUiRequestBus::Event(
                GetComponentType(),
                &ComponentModeViewportUiRequestBus::Events::SetViewportUiActiveEntityComponentId,
                GetEntityComponentIdPair());
        }
    } // namespace ComponentModeFramework
} // namespace AzToolsFramework
