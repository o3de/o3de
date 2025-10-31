/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ComponentModeDelegate.h"

#include <AzCore/RTTI/BehaviorContext.h>

#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegateBus.h>

namespace AzToolsFramework
{
    namespace ComponentModeFramework
    {
        namespace Internal
        {
            struct EditorComponentModeNotificationBusHandler final
                : public EditorComponentModeNotificationBus::Handler
                , public AZ::BehaviorEBusHandler
            {
                AZ_EBUS_BEHAVIOR_BINDER(
                    EditorComponentModeNotificationBusHandler, "{AD2F4204-0913-4FC9-9A10-492538F60C70}", AZ::SystemAllocator, ActiveComponentModeChanged);

                void ActiveComponentModeChanged(const AZ::Uuid& componentType) override
                {
                    Call(FN_ActiveComponentModeChanged, componentType);
                }
            };
        }

        static const char* const s_componentModeEnterDescription =
            "In this mode, you can only edit properties for this component. "
            "All other components on the entity are locked.";

        static const char* const s_componentModeLeaveDescription =
            "Return to normal viewport editing";

        // was the double click on the component or off it (select/deselect)
        enum class DoubleClickOutcome
        {
            OnComponent,
            OffComponent,
            None,
        };

        static bool EnterComponentModeButtonVisible()
        {
            return !InComponentMode();
        }

        static bool LeaveComponentModeButtonVisible()
        {
            return InComponentMode();
        }

        static DoubleClickOutcome DoubleClickedComponent(
            const ViewportInteraction::MouseInteractionEvent& mouseInteraction,
            EditorComponentSelectionRequestsBus::Handler* editorComponentSelection)
        {
            if (mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::DoubleClick &&
                mouseInteraction.m_mouseInteraction.m_mouseButtons.Left())
            {
                if (editorComponentSelection)
                {
                    float distance;
                    if (editorComponentSelection->EditorSelectionIntersectRayViewport(
                        { mouseInteraction.m_mouseInteraction.m_interactionId.m_viewportId },
                        mouseInteraction.m_mouseInteraction.m_mousePick.m_rayOrigin,
                        mouseInteraction.m_mouseInteraction.m_mousePick.m_rayDirection,
                        distance))
                    {
                        return DoubleClickOutcome::OnComponent;
                    }
                }

                return DoubleClickOutcome::OffComponent;
            }

            return DoubleClickOutcome::None;
        }

        static bool ShouldDetectEnterLeaveComponentMode(
            const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            return mouseInteraction.m_mouseInteraction.m_mouseButtons.Left() &&
                mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::DoubleClick;
        }

        static bool EditorRequestingGame()
        {
            bool requestingGame = false;
            AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
                requestingGame, &AzToolsFramework::EditorEntityContextRequestBus::Events::IsEditorRequestingGame);

            return requestingGame;
        }

        static bool EntityHasPendingComponents(const AZ::EntityId entityId)
        {
            AZ::Entity::ComponentArrayType pendingComponents;
            EditorPendingCompositionRequestBus::Event(
                entityId, &EditorPendingCompositionRequestBus::Events::GetPendingComponents,
                pendingComponents);

            return !pendingComponents.empty();
        }

        static bool CanEnterComponentMode(const AZ::EntityId entityId)
        {
            // if the editor is transitioning to game mode or if any entities in the selection are not
            // selectable (invisible/locked) or if any components are in a pending state (a conflict is
            // present), make it impossible to enter Component Mode
            return !EditorRequestingGame() && IsSelectableInViewport(entityId) && !EntityHasPendingComponents(entityId);
        }

        void ComponentModeDelegate::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ComponentModeDelegate>()
                    ->Version(1)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<ComponentModeDelegate>(
                        "Component Mode", "Provides advanced editing of Components.")
                        ->UIElement(AZ::Edit::UIHandlers::Button, "", s_componentModeEnterDescription)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &ComponentModeDelegate::OnComponentModeEnterButtonPressed)
                            ->Attribute(AZ::Edit::Attributes::ButtonText, "Edit")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &EnterComponentModeButtonVisible)
                            ->Attribute(AZ::Edit::Attributes::AcceptsMultiEdit, true)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &ComponentModeDelegate::ComponentModeButtonInactive)
                        ->UIElement(AZ::Edit::UIHandlers::Button, "", s_componentModeLeaveDescription)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &ComponentModeDelegate::OnComponentModeLeaveButtonPressed)
                            ->Attribute(AZ::Edit::Attributes::ButtonText, "Done")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &LeaveComponentModeButtonVisible)
                            ->Attribute(AZ::Edit::Attributes::AcceptsMultiEdit, true)
                            ;
                }
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<ComponentModeSystemRequestBus>("ComponentModeSystemRequestBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Editor")
                    ->Attribute(AZ::Script::Attributes::Module, "editor")
                    ->Event("EnterComponentMode", &ComponentModeSystemRequests::AddSelectedComponentModesOfType)
                    ->Event("EndComponentMode", &ComponentModeSystemRequests::EndComponentMode)
                    ;

                behaviorContext->EBus<EditorComponentModeNotificationBus>("EditorComponentModeNotificationBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Editor")
                    ->Attribute(AZ::Script::Attributes::Module, "editor")
                    ->Handler<Internal::EditorComponentModeNotificationBusHandler>()
                    ->Event("ActiveComponentModeChanged", &EditorComponentModeNotifications::ActiveComponentModeChanged)
                    ;
            }
        }

        bool ComponentModeDelegate::AddedToComponentMode() const
        {
            bool addedToComponentMode = false;
            ComponentModeSystemRequestBus::BroadcastResult(
                addedToComponentMode, &ComponentModeSystemRequests::AddedToComponentMode,
                m_entityComponentIdPair, m_componentType);

            return addedToComponentMode;
        }

        void ComponentModeDelegate::SetAddComponentModeCallback(
            const AZStd::function<void(const AZ::EntityComponentIdPair&)>& addComponentModeCallback)
        {
            m_addComponentMode = addComponentModeCallback;
        }

        void ComponentModeDelegate::OnComponentModeEnterButtonPressed()
        {
            // Check the entity hasn't been deselected but we haven't been told yet.
            if (!IsSelected(m_entityComponentIdPair.GetEntityId()))
            {
                return;
            }

            // ensure we aren't already in ComponentMode and are not also attempting to enter game mode
            if (!InComponentMode() && !EditorRequestingGame())
            {
                // move all selected components into ComponentMode
                ComponentModeSystemRequestBus::Broadcast(
                    &ComponentModeSystemRequests::AddSelectedComponentModesOfType,
                    m_componentType);
            }
        }

        void ComponentModeDelegate::OnComponentModeLeaveButtonPressed()
        {
            if (InComponentMode())
            {
                // move the editor out of ComponentMode
                ComponentModeSystemRequestBus::Broadcast(
                    &ComponentModeSystemRequests::EndComponentMode);
            }
        }

        void ComponentModeDelegate::AddComponentMode()
        {
            if (m_addComponentMode)
            {
                m_addComponentMode(m_entityComponentIdPair);
            }
        }

        void ComponentModeDelegate::ConnectInternal(
            const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid componentType,
            EditorComponentSelectionRequestsBus::Handler* handler)
        {
            m_handler = handler; // could be null
            m_entityComponentIdPair = entityComponentIdPair;
            m_componentType = componentType;

            EntitySelectionEvents::Bus::Handler::BusConnect(entityComponentIdPair.GetEntityId());
            EditorEntityVisibilityNotificationBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());
            EditorEntityLockComponentNotificationBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());

            AzFramework::ComponentModeDelegateNotificationBus::Broadcast(
                &AzFramework::ComponentModeDelegateNotificationBus::Events::OnComponentModeDelegateConnect, m_entityComponentIdPair);
        }

        void ComponentModeDelegate::Disconnect()
        {
            EditorEntityLockComponentNotificationBus::Handler::BusDisconnect();
            EditorEntityVisibilityNotificationBus::Handler::BusDisconnect();
            EntitySelectionEvents::Bus::Handler::BusDisconnect();

            AzFramework::ComponentModeDelegateNotificationBus::Broadcast(
                &AzFramework::ComponentModeDelegateNotificationBus::Events::OnComponentModeDelegateDisconnect, m_entityComponentIdPair);

            m_componentType = AZ::Uuid::CreateNull();
            m_entityComponentIdPair = AZ::EntityComponentIdPair(AZ::EntityId(), AZ::InvalidComponentId);
            m_handler = nullptr;
        }

        void ComponentModeDelegate::OnSelected()
        {
            ComponentModeDelegateRequestBus::Handler::BusConnect(m_entityComponentIdPair);
        }

        void ComponentModeDelegate::OnDeselected()
        {
            ComponentModeDelegateRequestBus::Handler::BusDisconnect();
        }

        bool ComponentModeDelegate::DetectEnterComponentModeInteraction(
            const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            if (!ShouldDetectEnterLeaveComponentMode(mouseInteraction) ||
                DoubleClickedComponent(mouseInteraction, m_handler) != DoubleClickOutcome::OnComponent)
            {
                return false;
            }

            if (!CanEnterComponentMode(m_entityComponentIdPair.GetEntityId()))
            {
                // note: return true here to indicate attempted to enter component mode,
                // we do not want the attempted double-click to deselect the entity
                return true;
            }

            EntityIdList entityIds;
            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
                entityIds, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

            AZStd::vector<AZ::Uuid> componentTypes;
            AZ::Entity::ComponentArrayType components;
            // reserve small initial buffer for common case
            components.reserve(8);
            componentTypes.reserve(8);
            // build a list of all components on each entity in the current selection
            for (AZ::EntityId entityId : entityIds)
            {
                components.clear();

                // get all components related to the entity
                GetAllComponentsForEntity(GetEntity(entityId), components);
                RemoveHiddenComponents(components);

                AZStd::transform(
                    components.begin(), components.end(), AZStd::back_inserter(componentTypes),
                    [](const AZ::Component* component) { return component->GetUnderlyingComponentType(); });
            }

            // count how many components of our type are in the selection
            const size_t componentCount =
                AZStd::count_if(componentTypes.begin(), componentTypes.end(),
                [this](const AZ::Uuid& componentType) { return componentType == m_componentType; });

            // if the count matches the entity selection size, we know each entity has a
            // component of that type, and so it will be displaying in the Entity Outliner
            // if this is the case we know it is safe to enter ComponentMode
            if (componentCount == entityIds.size())
            {
                AddComponentMode();
            }

            // we still want to notify the outside world an attempt was made to enter
            // ComponentMode - ComponentModeCollection::ModesAdded() must be called
            // to determine if ComponentMode was actually entered
            return true;
        }

        bool ComponentModeDelegate::DetectLeaveComponentModeInteraction(
            [[maybe_unused]] const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            // by default, we won't use mouse interactions to leave component mode, so we'll always return false.
            return false;
        }

        void ComponentModeDelegate::AddComponentModeOfType(const AZ::Uuid componentType)
        {
            if (m_componentType == componentType)
            {
                AddComponentMode();
            }
        }

        bool CouldBeginComponentModeWithEntity(const AZ::EntityId entityId)
        {
            EntityIdList selectedEntityIds;
            ToolsApplicationRequests::Bus::BroadcastResult(
                selectedEntityIds, &ToolsApplicationRequests::GetSelectedEntities);

            // handles both having no entities selected and when an entity inspector is
            // pinned on an entity that's not selected and a different entity is selected
            bool canBegin = AZStd::find(
                selectedEntityIds.cbegin(), selectedEntityIds.cend(), entityId) != selectedEntityIds.cend();

            if (canBegin)
            {
                for (auto&& selectedEntityId : selectedEntityIds)
                {
                    if (!CanEnterComponentMode(selectedEntityId))
                    {
                        canBegin = false;
                        break;
                    }
                }
            }

            return canBegin;
        }

        bool ComponentModeDelegate::ComponentModeButtonInactive() const
        {
            return !CouldBeginComponentModeWithEntity(m_entityComponentIdPair.GetEntityId());
        }

        void ComponentModeDelegate::OnEntityVisibilityChanged(bool /*visibility*/)
        {
            ToolsApplicationNotificationBus::Broadcast(
                &ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplayForComponent,
                m_entityComponentIdPair,
                Refresh_AttributesAndValues);
        }

        void ComponentModeDelegate::OnEntityLockChanged(bool /*locked*/)
        {
            ToolsApplicationNotificationBus::Broadcast(
                &ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplayForComponent,
                m_entityComponentIdPair,
                Refresh_AttributesAndValues);
        }
    } // namespace ComponentModeFramework
} // namespace AzToolsFramework
