/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefaultSelection.h"

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/API/ViewportEditorModeTrackerInterface.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <Entity/EditorEntityHelpers.h>
#include <QGuiApplication>

namespace AzToolsFramework
{
    AZ_CLASS_ALLOCATOR_IMPL(EditorDefaultSelection, AZ::SystemAllocator, 0)

    EditorDefaultSelection::EditorDefaultSelection(
        const EditorVisibleEntityDataCache* entityDataCache, ViewportEditorModeTrackerInterface* viewportEditorModeTracker)
        : m_phantomWidget(nullptr)
        , m_entityDataCache(entityDataCache)
        , m_viewportEditorModeTracker(viewportEditorModeTracker)
        , m_componentModeCollection(viewportEditorModeTracker)
    {
        AZ_Assert(
            AZ::Interface<ComponentModeCollectionInterface>::Get() == nullptr, "Unexpected registration of component mode collection.")
        AZ::Interface<ComponentModeCollectionInterface>::Register(&m_componentModeCollection);

        ActionOverrideRequestBus::Handler::BusConnect(GetEntityContextId());
        ComponentModeFramework::ComponentModeSystemRequestBus::Handler::BusConnect();

        m_manipulatorManager = AZStd::make_shared<AzToolsFramework::ManipulatorManager>(AzToolsFramework::g_mainManipulatorManagerId);
        m_transformComponentSelection = AZStd::make_unique<EditorTransformComponentSelection>(entityDataCache);
        m_viewportEditorModeTracker->ActivateMode({ GetEntityContextId() }, ViewportEditorMode::Default);
    }

    EditorDefaultSelection::~EditorDefaultSelection()
    {
        ComponentModeFramework::ComponentModeSystemRequestBus::Handler::BusDisconnect();
        ActionOverrideRequestBus::Handler::BusDisconnect();
        m_viewportEditorModeTracker->DeactivateMode({ GetEntityContextId() }, ViewportEditorMode::Default);

        AZ_Assert(
            AZ::Interface<ComponentModeCollectionInterface>::Get() != nullptr,
            "Unexpected unregistration of component mode collection.")
        AZ::Interface<ComponentModeCollectionInterface>::Unregister(&m_componentModeCollection);
    }

    void EditorDefaultSelection::SetOverridePhantomWidget(QWidget* phantomOverrideWidget)
    {
        m_phantomOverrideWidget = phantomOverrideWidget;
    }

    QWidget& EditorDefaultSelection::PhantomWidget()
    {
        if (m_phantomOverrideWidget)
        {
            return *m_phantomOverrideWidget;
        }
        else
        {
            return m_phantomWidget;
        }
    }

    void EditorDefaultSelection::BeginComponentMode(
        const AZStd::vector<ComponentModeFramework::EntityAndComponentModeBuilders>& entityAndComponentModeBuilders)
    {
        for (const auto& componentModeBuilder : entityAndComponentModeBuilders)
        {
            AddComponentModes(componentModeBuilder);
        }

        TransitionToComponentMode();
    }

    void EditorDefaultSelection::AddComponentModes(
        const ComponentModeFramework::EntityAndComponentModeBuilders& entityAndComponentModeBuilders)
    {
        for (const auto& componentModeBuilder : entityAndComponentModeBuilders.m_componentModeBuilders)
        {
            m_componentModeCollection.AddComponentMode(
                AZ::EntityComponentIdPair(entityAndComponentModeBuilders.m_entityId, componentModeBuilder.m_componentId),
                componentModeBuilder.m_componentType, componentModeBuilder.m_componentModeBuilder);
        }
    }

    void EditorDefaultSelection::TransitionToComponentMode()
    {
        // entering ComponentMode - disable all default actions in the ActionManager
        EditorActionRequestBus::Broadcast(&EditorActionRequests::DisableDefaultActions);

        // attach widget to store ComponentMode specific actions
        EditorActionRequestBus::Broadcast(&EditorActionRequests::AttachOverride, &PhantomWidget());

        if (m_transformComponentSelection)
        {
            // hide manipulators
            m_transformComponentSelection->UnregisterManipulator();
        }

        m_componentModeCollection.BeginComponentMode();

        // refresh button ui
        ToolsApplicationEvents::Bus::Broadcast(
            &ToolsApplicationEvents::Bus::Events::InvalidatePropertyDisplay, PropertyModificationRefreshLevel::Refresh_EntireTree);
    }

    void EditorDefaultSelection::TransitionFromComponentMode()
    {
        m_componentModeCollection.EndComponentMode();

        if (m_transformComponentSelection)
        {
            // safe to show manipulators again
            m_transformComponentSelection->RegisterManipulator();
        }

        EditorActionRequestBus::Broadcast(&EditorActionRequests::DetachOverride);

        ClearActionOverrides();

        // leaving ComponentMode - enable all default actions in ActionManager
        EditorActionRequestBus::Broadcast(&EditorActionRequests::EnableDefaultActions);

        // refresh button ui
        ToolsApplicationEvents::Bus::Broadcast(
            &ToolsApplicationEvents::Bus::Events::InvalidatePropertyDisplay, PropertyModificationRefreshLevel::Refresh_EntireTree);
    }

    void EditorDefaultSelection::EndComponentMode()
    {
        TransitionFromComponentMode();
    }

    void EditorDefaultSelection::Refresh(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        m_componentModeCollection.Refresh(entityComponentIdPair);
    }

    bool EditorDefaultSelection::AddedToComponentMode(const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid& componentType)
    {
        return m_componentModeCollection.AddedToComponentMode(entityComponentIdPair, componentType);
    }

    void EditorDefaultSelection::AddSelectedComponentModesOfType(const AZ::Uuid& componentType)
    {
        ComponentModeFramework::ComponentModeDelegateRequestBus::EnumerateHandlers(
            [componentType](ComponentModeFramework::ComponentModeDelegateRequestBus::InterfaceType* componentModeMouseRequests)
            {
                componentModeMouseRequests->AddComponentModeOfType(componentType);
                return true;
            });

        TransitionToComponentMode();
    }

    bool EditorDefaultSelection::SelectNextActiveComponentMode()
    {
        return m_componentModeCollection.SelectNextActiveComponentMode();
    }

    bool EditorDefaultSelection::SelectPreviousActiveComponentMode()
    {
        return m_componentModeCollection.SelectPreviousActiveComponentMode();
    }

    bool EditorDefaultSelection::SelectActiveComponentMode(const AZ::Uuid& componentType)
    {
        return m_componentModeCollection.SelectActiveComponentMode(componentType);
    }

    AZ::Uuid EditorDefaultSelection::ActiveComponentMode()
    {
        return m_componentModeCollection.ActiveComponentMode();
    }

    bool EditorDefaultSelection::ComponentModeInstantiated(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        return m_componentModeCollection.ComponentModeInstantiated(entityComponentIdPair);
    }

    bool EditorDefaultSelection::HasMultipleComponentTypes()
    {
        return m_componentModeCollection.HasMultipleComponentTypes();
    }

    void EditorDefaultSelection::RefreshActions()
    {
        m_componentModeCollection.RefreshActions();
    }

    bool EditorDefaultSelection::InternalHandleMouseManipulatorInteraction(
        const ViewportInteraction::MouseInteractionEvent& mouseInteractionEvent)
    {
        if (!m_manipulatorManager)
        {
            return false;
        }

        using AzToolsFramework::ViewportInteraction::MouseEvent;
        const auto& mouseInteraction = mouseInteractionEvent.m_mouseInteraction;
        // store the current interaction for use in DrawManipulators
        m_currentInteraction = mouseInteraction;

        switch (mouseInteractionEvent.m_mouseEvent)
        {
        case MouseEvent::Down:
            return m_manipulatorManager->ConsumeViewportMousePress(mouseInteraction);
        case MouseEvent::DoubleClick:
            return false;
        case MouseEvent::Move:
            {
                const AzToolsFramework::ManipulatorManager::ConsumeMouseMoveResult mouseMoveResult =
                    m_manipulatorManager->ConsumeViewportMouseMove(mouseInteraction);
                return mouseMoveResult == AzToolsFramework::ManipulatorManager::ConsumeMouseMoveResult::Interacting;
            }
        case MouseEvent::Up:
            return m_manipulatorManager->ConsumeViewportMouseRelease(mouseInteraction);
        case MouseEvent::Wheel:
            return m_manipulatorManager->ConsumeViewportMouseWheel(mouseInteraction);
        default:
            return false;
        }
    }

    bool EditorDefaultSelection::InternalHandleMouseViewportInteraction(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        bool enterComponentModeAttempted = false;
        const bool componentModeBefore = InComponentMode();

        bool handled = false;
        if (!componentModeBefore)
        {
            // enumerate all ComponentModeDelegateRequestBus and check if any triggered AddComponentModes
            ComponentModeFramework::ComponentModeDelegateRequestBus::EnumerateHandlers(
                [&mouseInteraction, &enterComponentModeAttempted](
                    ComponentModeFramework::ComponentModeDelegateRequestBus::InterfaceType* componentModeMouseRequests)
                {
                    // detect if a double click happened on any Component in the viewport, attempting
                    // to move it into ComponentMode (note: this is not guaranteed to succeed as an
                    // incompatible multi-selection may prevent it)
                    enterComponentModeAttempted = componentModeMouseRequests->DetectEnterComponentModeInteraction(mouseInteraction);
                    return !enterComponentModeAttempted;
                });

            // here we know ComponentMode was entered successfully and was not prohibited
            if (m_componentModeCollection.ModesAdded())
            {
                // for other entities in current selection, if they too support the same
                // ComponentMode, add them as well (same effect as pressing Component
                // Mode button in the Property Grid)
                m_componentModeCollection.AddOtherSelectedEntityModes();
                TransitionToComponentMode();
            }
        }
        else
        {
            ComponentModeFramework::ComponentModeRequestBus::EnumerateHandlers(
                [&mouseInteraction, &handled](ComponentModeFramework::ComponentModeRequestBus::InterfaceType* componentModeRequest)
                {
                    if (componentModeRequest->HandleMouseInteraction(mouseInteraction))
                    {
                        handled = true;
                    }

                    return true;
                });

            if (!handled)
            {
                ComponentModeFramework::ComponentModeDelegateRequestBus::EnumerateHandlers(
                    [&mouseInteraction](
                        ComponentModeFramework::ComponentModeDelegateRequestBus::InterfaceType* componentModeDelegateRequests)
                    {
                        return !componentModeDelegateRequests->DetectLeaveComponentModeInteraction(mouseInteraction);
                    });
            }
        }

        // we do not want a double click on a Component while attempting to enter ComponentMode
        // to fall through to normal input handling (as this will cause a deselect to happen).
        // a double click on a Component that prevents entering ComponentMode due to an invalid
        // multi-selection will be a noop
        if (!componentModeBefore && !InComponentMode() && !enterComponentModeAttempted)
        {
            if (m_transformComponentSelection)
            {
                // no components being edited (not in ComponentMode), use standard selection
                return m_transformComponentSelection->HandleMouseInteraction(mouseInteraction);
            }
        }

        return handled;
    }

    void EditorDefaultSelection::DisplayViewportSelection(
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (m_transformComponentSelection)
        {
            m_transformComponentSelection->DisplayViewportSelection(viewportInfo, debugDisplay);
        }

        // poll and set the keyboard modifiers to ensure the mouse interaction is up to date
        m_currentInteraction.m_keyboardModifiers = AzToolsFramework::ViewportInteraction::QueryKeyboardModifiers();

        // draw the manipulators
        const AzFramework::CameraState cameraState = GetCameraState(viewportInfo.m_viewportId);
        debugDisplay.DepthTestOff();
        m_manipulatorManager->DrawManipulators(debugDisplay, cameraState, m_currentInteraction);
        debugDisplay.DepthTestOn();
    }

    void EditorDefaultSelection::DisplayViewportSelection2d(
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (m_transformComponentSelection)
        {
            m_transformComponentSelection->DisplayViewportSelection2d(viewportInfo, debugDisplay);
        }
    }

    void EditorDefaultSelection::SetupActionOverrideHandler(QWidget* parent)
    {
        PhantomWidget().setParent(parent);
        // note: widget must be 'visible' for actions to fire
        // hide by setting size to zero dimensions
        PhantomWidget().setFixedSize(0, 0);
    }

    void EditorDefaultSelection::TeardownActionOverrideHandler()
    {
        PhantomWidget().setParent(nullptr);
    }

    void EditorDefaultSelection::AddActionOverride(const ActionOverride& actionOverride)
    {
        // check if an action with this uri is already added
        const auto actionIt = AZStd::find_if(
            m_actions.begin(), m_actions.end(),
            [actionOverride](const AZStd::shared_ptr<ActionOverrideMapping>& actionOverrideMapping)
            {
                return actionOverride.m_uri == actionOverrideMapping->m_uri;
            });

        // if an action with the same uri is already added, store the callback for this action
        if (actionIt != m_actions.end())
        {
            (*actionIt)->m_callbacks.push_back(actionOverride.m_callback);
        }
        else
        {
            // create a new action with the override widget as the parent
            AZStd::unique_ptr<QAction> action = AZStd::make_unique<QAction>(&PhantomWidget());

            // setup action specific data for the editor
            action->setShortcut(actionOverride.m_keySequence);
            action->setStatusTip(actionOverride.m_statusTip);
            action->setText(actionOverride.m_title);

            // bind action to widget
            PhantomWidget().addAction(action.get());

            // set callbacks that should happen when this action is triggered
            auto index = static_cast<int>(m_actions.size());
            QObject::connect(
                action.get(), &QAction::triggered,
                [this, index]()
                {
                    const auto vec = m_actions; // increment ref count of shared_ptr, callback may clear actions
                    for (auto& callback : vec[index]->m_callbacks)
                    {
                        callback();
                    }
                });

            m_actions.emplace_back(AZStd::make_shared<ActionOverrideMapping>(
                actionOverride.m_uri, AZStd::vector<AZStd::function<void()>>{ actionOverride.m_callback }, AZStd::move(action)));

            // register action with edit menu
            EditorMenuRequestBus::Broadcast(&EditorMenuRequests::AddEditMenuAction, m_actions.back()->m_action.get());
        }
    }

    void EditorDefaultSelection::ClearActionOverrides()
    {
        AZStd::for_each(
            m_actions.begin(), m_actions.end(),
            [this](const AZStd::shared_ptr<ActionOverrideMapping>& actionMapping)
            {
                PhantomWidget().removeAction(actionMapping->m_action.get());
            });

        m_actions.clear();
    }

    void EditorDefaultSelection::RemoveActionOverride(const AZ::Crc32 actionOverrideUri)
    {
        const auto it = AZStd::find_if(
            m_actions.begin(), m_actions.end(),
            [actionOverrideUri](const AZStd::shared_ptr<ActionOverrideMapping>& actionMapping)
            {
                return actionMapping->m_uri == actionOverrideUri;
            });

        if (it != m_actions.end())
        {
            PhantomWidget().removeAction((*it)->m_action.get());
            m_actions.erase(it);
        }
    }
} // namespace AzToolsFramework
