/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ComponentModeCollection.h"

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/API/ViewportEditorModeTrackerInterface.h>
#include <AzToolsFramework/Commands/ComponentModeCommand.h>

namespace AzToolsFramework
{
    namespace ComponentModeFramework
    {
        AZ_CLASS_ALLOCATOR_IMPL(ComponentModeCollection, AZ::SystemAllocator, 0)

        static const char* const s_nextActiveComponentModeTitle = "Edit Next";
        static const char* const s_previousActiveComponentModeTitle = "Edit Previous";
        static const char* const s_nextActiveComponentModeDesc = "Move to the next component";
        static const char* const s_prevActiveComponentModeDesc = "Move to the previous component";
        static const char* const s_leaveCompoenentModeTitle = "Done";
        static const char* const s_leaveCompoenentModeDesc = "Return to normal viewport editing";
        static const char* const s_enteringComponentModeUndoRedoDesc = "Editing Component";
        static const char* const s_leavingComponentModeUndoRedoDesc = "Stopped Editing Component";

        /// Predicate to search for a Component in list of Entities and Component Modes.
        struct EntityAndComponentModePred
        {
            explicit EntityAndComponentModePred(
                const AZ::EntityComponentIdPair& entityComponentIdPair)
                : m_entityComponentIdPair(entityComponentIdPair) {}

            bool operator()(const EntityAndComponentMode& entityAndComponentMode) const
            {
                // find the entity and specific component in this mode
                return entityAndComponentMode.m_entityId == m_entityComponentIdPair.GetEntityId()
                    && entityAndComponentMode.m_componentMode->GetComponentId() == m_entityComponentIdPair.GetComponentId();
            }

        private:
            AZ::EntityComponentIdPair m_entityComponentIdPair;
        };

        /// Predicate to search for contained Component in list of Entities and Component Mode Builders.
        struct EntityAndComponentModeBuildersPred
        {
            explicit EntityAndComponentModeBuildersPred(
                const AZ::EntityComponentIdPair& entityComponentIdPair)
                : m_entityComponentIdPair(entityComponentIdPair) {}

            bool operator()(const EntityAndComponentModeBuilders& entityAndComponentModeBuilders) const
            {
                // find the entity this builder is associated with
                if (entityAndComponentModeBuilders.m_entityId == m_entityComponentIdPair.GetEntityId())
                {
                    // find the specific component on the entity this builder is associated with
                    for (const ComponentModeBuilder& componentModeBuilder : entityAndComponentModeBuilders.m_componentModeBuilders)
                    {
                        if (m_entityComponentIdPair.GetComponentId() == componentModeBuilder.m_componentId)
                        {
                            return true;
                        }
                    }
                }

                return false;
            }

        private:
            AZ::EntityComponentIdPair m_entityComponentIdPair;
        };

        ComponentModeCollection::ComponentModeCollection(ViewportEditorModeTrackerInterface* viewportEditorModeTracker)
            : m_viewportEditorModeTracker(viewportEditorModeTracker)
        {
        }

        void ComponentModeCollection::AddComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid componentType,
            const ComponentModeFactoryFunction& componentModeBuilder)
        {
            // check if we already have a ComponentMode for this component type
            const auto componentTypeIt = AZStd::find(
                m_activeComponentTypes.begin(), m_activeComponentTypes.end(), componentType);

            // if not, store it to notify other system what types of components are in ComponentMode
            if (componentTypeIt == m_activeComponentTypes.end())
            {
                m_activeComponentTypes.push_back(componentType);
                m_viewportUiHandlers.emplace_back(componentType);
            }

            // see if we already have a ComponentModeBuilder for the specific component on this entity
            const auto builderEntityIt = AZStd::find_if(
                m_entitiesAndComponentModeBuilders.begin(), m_entitiesAndComponentModeBuilders.end(),
                EntityAndComponentModeBuildersPred(entityComponentIdPair));

            // if we do not have a ComponentModeBuilder, create the ComponentMode from the builder, store it,
            // and also store the builder to be later recorded for the undo/redo step
            if (builderEntityIt == m_entitiesAndComponentModeBuilders.end())
            {
                // see if we are already storing a ComponentMode for this entity
                const auto entityWithComponentMode = AZStd::find_if(
                    m_entitiesAndComponentModes.begin(), m_entitiesAndComponentModes.end(),
                    [entityComponentIdPair](const EntityAndComponentMode& entityAndComponentMode)
                    {
                        return entityAndComponentMode.m_entityId == entityComponentIdPair.GetEntityId();
                    });

                // we do not already have a component mode for this entity
                if (entityWithComponentMode == m_entitiesAndComponentModes.end())
                {
                    // instantiate the component mode from the builder
                    m_entitiesAndComponentModes.emplace_back(entityComponentIdPair.GetEntityId(), componentModeBuilder());
                }

                // see if we are already storing a ComponentModeBuilder for this entity
                const auto entityWithComponentBuilder = AZStd::find_if(
                    m_entitiesAndComponentModeBuilders.begin(), m_entitiesAndComponentModeBuilders.end(),
                    [entityComponentIdPair](const EntityAndComponentModeBuilders& entityAndComponentModeBuilder)
                    {
                        return entityAndComponentModeBuilder.m_entityId == entityComponentIdPair.GetEntityId();
                    });

                // if we are, add the new ComponentModeBuilder to this entities list of ComponentModeBuilders
                if (entityWithComponentBuilder != m_entitiesAndComponentModeBuilders.end())
                {
                    entityWithComponentBuilder->m_componentModeBuilders.push_back(
                        ComponentModeBuilder(entityComponentIdPair.GetComponentId(), componentType, componentModeBuilder));

                    // if any of the already instantiated Component Modes are the same type as the one we're
                    // adding now, make sure we instantiate it as well
                    if (AZStd::any_of(m_entitiesAndComponentModes.begin(), m_entitiesAndComponentModes.end(),
                        [componentType](const EntityAndComponentMode& entityAndComponentMode)
                        {
                            return entityAndComponentMode.m_componentMode->GetComponentType() == componentType;
                        }))
                    {
                        m_entitiesAndComponentModes.emplace_back(
                            entityComponentIdPair.GetEntityId(), componentModeBuilder());
                    }
                }
                else
                {
                    // otherwise create new EntityAndComponentModeBuilder entry
                    m_entitiesAndComponentModeBuilders.emplace_back(
                        entityComponentIdPair.GetEntityId(), AZStd::vector<ComponentModeBuilder>(
                            1, ComponentModeBuilder(entityComponentIdPair.GetComponentId(), componentType, componentModeBuilder)));
                }

                // notify the ComponentModeCollection ComponentModes are being added
                // (we are transitioning to editor-wide ComponentMode)
                m_adding = true;
            }
        }

        AZStd::vector<AZ::Uuid> ComponentModeCollection::GetComponentTypes() const
        {
            // If in component mode, return the active component types, otherwise return an empty vector
            return InComponentMode() ? m_activeComponentTypes : AZStd::vector<AZ::Uuid>{};
        }

        void ComponentModeCollection::BeginComponentMode()
        {
            m_selectedComponentModeIndex = 0;
            m_componentMode = true;
            m_adding = false;

            // notify listeners the editor has entered ComponentMode - listeners may
            // wish to modify state to indicate this (e.g. appearance, functionality etc.)
            m_viewportEditorModeTracker->ActivateMode({ GetEntityContextId() }, ViewportEditorMode::Component);

            // enable actions for the first/primary ComponentMode
            // note: if multiple ComponentModes are activated at the same time, actions
            // are not available together, the 'active' mode will bind its actions one at a time
            if (!m_entitiesAndComponentModes.empty())
            {
                RefreshActions();
                PopulateViewportUi();
            }

            // if entering ComponentMode not as an undo/redo step (an action was
            // taken to initiate), record it as an undo step
            if (!UndoRedoOperationInProgress())
            {
                ScopedUndoBatch undoBatch(s_enteringComponentModeUndoRedoDesc);

                auto componentModeCommand = AZStd::make_unique<ComponentModeCommand>(
                    ComponentModeCommand::Transition::Enter, AZStd::string(s_enteringComponentModeUndoRedoDesc),
                    m_entitiesAndComponentModeBuilders);

                // componentModeCommand managed by undoBatch
                componentModeCommand->SetParent(undoBatch.GetUndoBatch());
                componentModeCommand.release();
            }
        }

        void ComponentModeCollection::AddOtherSelectedEntityModes()
        {
            for (const auto& componentType : m_activeComponentTypes)
            {
                ComponentModeDelegateRequestBus::Broadcast(
                    &ComponentModeDelegateRequests::AddComponentModeOfType,
                    componentType);
            }
        }

        bool ComponentModeCollection::AddedToComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid& componentType)
        {
            // if we already have a builder for this entity, we will have
            // created a mode from it, find it for the component id on this entity
            const auto modeEntityIt = AZStd::find_if(
                m_entitiesAndComponentModes.begin(), m_entitiesAndComponentModes.end(),
                EntityAndComponentModePred(entityComponentIdPair));

            if (modeEntityIt == m_entitiesAndComponentModes.end())
            {
                return false;
            }

            return componentType == modeEntityIt->m_componentMode->GetComponentType();
        }

        void ComponentModeCollection::EndComponentMode()
        {
            if (!UndoRedoOperationInProgress())
            {
                ScopedUndoBatch undoBatch(s_leavingComponentModeUndoRedoDesc);

                auto componentModeCommand = AZStd::make_unique<ComponentModeCommand>(
                    ComponentModeCommand::Transition::Leave, AZStd::string(s_leavingComponentModeUndoRedoDesc),
                    m_entitiesAndComponentModeBuilders);

                // componentModeCommand managed by undoBatch
                componentModeCommand->SetParent(undoBatch.GetUndoBatch());
                componentModeCommand.release();
            }

            // remove the component mode viewport border
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::RemoveViewportBorder);

            // notify listeners the editor has left ComponentMode - listeners may
            // wish to modify state to indicate this (e.g. appearance, functionality etc.)
            m_viewportEditorModeTracker->DeactivateMode({ GetEntityContextId() }, ViewportEditorMode::Component);

            // clear stored modes and builders for this ComponentMode
            // TLDR: avoid 'use after free' error
            // note: it is important to use pop_back() here to remove elements one at a
            // time as opposed to clear() because there is a possibility a callback in a
            // ComponentMode destructor may query state in ComponentModeCollection and the
            // internal owned ComponentMode instance may have been destroyed while iterating.
            while (!m_entitiesAndComponentModes.empty())
            {
                m_entitiesAndComponentModes.pop_back();
            }
            m_entitiesAndComponentModeBuilders.clear();
            m_activeComponentTypes.clear();
            m_viewportUiHandlers.clear();

            m_componentMode = false;
            m_selectedComponentModeIndex = 0;
        }

        void ComponentModeCollection::Refresh(const AZ::EntityComponentIdPair& entityComponentIdPair)
        {
            for (auto& entityAndComponentModes : m_entitiesAndComponentModes)
            {
                if (entityAndComponentModes.m_entityId == entityComponentIdPair.GetEntityId() &&
                    entityAndComponentModes.m_componentMode->GetComponentId() == entityComponentIdPair.GetComponentId())
                {
                    entityAndComponentModes.m_componentMode->Refresh();
                }
            }
        }

        bool ComponentModeCollection::SelectNextActiveComponentMode()
        {
            const AZ::Uuid previousComponentType =
                m_activeComponentTypes[m_selectedComponentModeIndex];

            m_selectedComponentModeIndex =
                (m_selectedComponentModeIndex + 1) % m_activeComponentTypes.size();

            return ActiveComponentModeChanged(previousComponentType);
        }

        bool ComponentModeCollection::SelectPreviousActiveComponentMode()
        {
            const AZ::Uuid previousComponentType =
                m_activeComponentTypes[m_selectedComponentModeIndex];

            m_selectedComponentModeIndex =
                (m_selectedComponentModeIndex + m_activeComponentTypes.size() - 1) % m_activeComponentTypes.size();

            return ActiveComponentModeChanged(previousComponentType);
        }

        bool ComponentModeCollection::SelectActiveComponentMode(const AZ::Uuid& componentType)
        {
            // is this ComponentMode in the active set
            const auto it = AZStd::find_if(m_activeComponentTypes.begin(), m_activeComponentTypes.end(),
                [componentType](const AZ::Uuid& activeComponentType)
                {
                    return componentType == activeComponentType;
                });

            if (it != m_activeComponentTypes.end())
            {
                const AZ::Uuid previousComponentType =
                    m_activeComponentTypes[m_selectedComponentModeIndex];

                // calculate the selected index
                m_selectedComponentModeIndex = it - m_activeComponentTypes.begin();

                return ActiveComponentModeChanged(previousComponentType);
            }

            return false;
        }

        AZ::Uuid ComponentModeCollection::ActiveComponentMode() const
        {
            return m_activeComponentTypes[m_selectedComponentModeIndex];
        }

        bool ComponentModeCollection::ComponentModeInstantiated(const AZ::EntityComponentIdPair& entityComponentIdPair) const
        {
            // search through all instantiated Component Modes to see if we have one
            // matching the requested Entity/Component pair
            return AZStd::any_of(m_entitiesAndComponentModes.begin(), m_entitiesAndComponentModes.end(),
                [entityComponentIdPair](const EntityAndComponentMode& entityAndComponentMode)
                {
                    return  entityAndComponentMode.m_entityId == entityComponentIdPair.GetEntityId() &&
                        entityAndComponentMode.m_componentMode->GetComponentId() == entityComponentIdPair.GetComponentId();
                });
        }

        bool ComponentModeCollection::HasMultipleComponentTypes() const
        {
            return m_activeComponentTypes.size() > 1;
        }

        static ComponentModeViewportUi* FindViewportUiHandlerForType(
            AZStd::vector<ComponentModeViewportUi>& viewportUiHandlers, const AZ::Uuid& componentType)
        {
            auto handler = AZStd::find_if(
                viewportUiHandlers.begin(), viewportUiHandlers.end(),
                [componentType](const ComponentModeViewportUi& handler)
                {
                    return handler.GetComponentType() == componentType;
                });

            if (handler == viewportUiHandlers.end())
            {
                return nullptr;
            }

            return handler;
        }

        bool ComponentModeCollection::ActiveComponentModeChanged(const AZ::Uuid& previousComponentType)
        {
            if (m_activeComponentTypes[m_selectedComponentModeIndex] != previousComponentType)
            {
                // for each entity and its 'active' Component Mode
                for (auto& componentMode : m_entitiesAndComponentModes)
                {
                    // find the builders for this entity and component
                    const auto builderEntityIt = AZStd::find_if(
                        m_entitiesAndComponentModeBuilders.begin(), m_entitiesAndComponentModeBuilders.end(),
                        EntityAndComponentModeBuildersPred(AZ::EntityComponentIdPair(
                            componentMode.m_entityId, componentMode.m_componentMode->GetComponentId())));

                    // find the builder for the active component type
                    const auto& componentModeBuilder = AZStd::find_if(
                        builderEntityIt->m_componentModeBuilders.begin(),
                        builderEntityIt->m_componentModeBuilders.end(),
                        [this](const ComponentModeBuilder& componentModeBuilder)
                        {
                            return componentModeBuilder.m_componentType == m_activeComponentTypes[m_selectedComponentModeIndex];
                        });

                    // replace the current component mode by invoking the builder
                    // for the new 'active' component mode
                    componentMode.m_componentMode = componentModeBuilder->m_componentModeBuilder();

                    // populate the viewport UI with the new component mode
                    PopulateViewportUi();

                    // set the appropriate viewportUiHandler to active
                    if (auto viewportUiHandler =
                            FindViewportUiHandlerForType(m_viewportUiHandlers, m_activeComponentTypes[m_selectedComponentModeIndex]))
                    {
                        viewportUiHandler->SetComponentModeViewportUiActive(true);
                    }

                    ViewportUi::ViewportUiRequestBus::Event(
                        ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::CreateViewportBorder,
                        componentMode.m_componentMode->GetComponentModeName().c_str(),
                        []
                        {
                            ComponentModeSystemRequestBus::Broadcast(&ComponentModeSystemRequests::EndComponentMode);
                        });
                }

                RefreshActions();

                if (m_selectedComponentModeIndex < m_activeComponentTypes.size())
                {
                    // notify other systems ComponentMode actions have changed
                    EditorComponentModeNotificationBus::Event(
                        GetEntityContextId(), &EditorComponentModeNotifications::ActiveComponentModeChanged,
                        m_activeComponentTypes[m_selectedComponentModeIndex]);

                    return true;
                }
            }

            return false;
        }

        void ComponentModeCollection::RefreshActions()
        {
        }

        void ComponentModeCollection::PopulateViewportUi()
        {
            // update viewport UI for new component type
            if (m_selectedComponentModeIndex < m_activeComponentTypes.size())
            {
                // iterate over all entities and their active Component Mode, populate viewport UI for the new mode
                for (auto& entityAndComponentMode : m_entitiesAndComponentModes)
                {
                    // build viewport UI based on current state
                    entityAndComponentMode.m_componentMode->PopulateViewportUi();
                }
            }
        }
    } // namespace ComponentModeFramework
} // namespace AzToolsFramework
