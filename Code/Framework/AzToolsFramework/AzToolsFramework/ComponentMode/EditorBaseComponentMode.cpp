/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "EditorBaseComponentMode.h"
#include "ComponentModeViewportUiRequestBus.h"

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

namespace AzToolsFramework
{
    namespace ComponentModeFramework
    {
        AZ_CLASS_ALLOCATOR_IMPL(EditorBaseComponentMode, AZ::SystemAllocator, 0)

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
            ToolsApplicationNotificationBus::Handler::BusConnect();
        }

        EditorBaseComponentMode::~EditorBaseComponentMode()
        {
            ToolsApplicationNotificationBus::Handler::BusDisconnect();
            ComponentModeRequestBus::Handler::BusDisconnect();
        }

        void EditorBaseComponentMode::AfterUndoRedo()
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
            // create the component mode border with the specific name for this component mode
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::CreateComponentModeBorder,
                GetComponentModeName());
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

        void EditorBaseComponentMode::PostHandleMouseInteraction()
        {
            ComponentModeViewportUiRequestBus::Event(
                GetComponentType(),
                &ComponentModeViewportUiRequestBus::Events::SetViewportUiActiveEntityComponentId,
                GetEntityComponentIdPair());
        }
    } // namespace ComponentModeFramework
} // namespace AzToolsFramework
