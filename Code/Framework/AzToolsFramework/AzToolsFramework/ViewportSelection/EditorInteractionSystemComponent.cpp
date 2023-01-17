/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorInteractionSystemComponent.h"

#include <AzToolsFramework/ViewportSelection/EditorDefaultSelection.h>
#include <AzToolsFramework/ViewportSelection/EditorVisibleEntityDataCache.h>
#include <AzToolsFramework/ViewportSelection/ViewportEditorModeTracker.h>

namespace AzToolsFramework
{
    EditorInteractionSystemComponent::EditorInteractionSystemComponent()
        : m_viewportEditorMode(AZStd::make_unique<ViewportEditorModeTracker>())
    {
        AZ_Assert(AZ::Interface<ViewportEditorModeTrackerInterface>::Get() == nullptr, "Unexpected registration of viewport editor mode tracker.")
        AZ::Interface<ViewportEditorModeTrackerInterface>::Register(m_viewportEditorMode.get());
    }

    EditorInteractionSystemComponent::~EditorInteractionSystemComponent()
    {
        m_interactionRequests.reset();
        AZ_Assert(AZ::Interface<ViewportEditorModeTrackerInterface>::Get() != nullptr, "Unexpected unregistration of viewport editor mode tracker.")
        AZ::Interface<ViewportEditorModeTrackerInterface>::Unregister(m_viewportEditorMode.get());
    }

    void EditorInteractionSystemComponent::Activate()
    {
        EditorInteractionSystemViewportSelectionRequestBus::Handler::BusConnect(GetEntityContextId());
        EditorEventsBus::Handler::BusConnect();
    }

    void EditorInteractionSystemComponent::Deactivate()
    {
        // EditorVisibleEntityDataCache does BusDisconnect in the destructor, so have to reset here
        m_entityDataCache.reset();

        AzFramework::ViewportDebugDisplayEventBus::Handler::BusDisconnect();
        EditorEventsBus::Handler::BusDisconnect();
        EditorInteractionSystemViewportSelectionRequestBus::Handler::BusDisconnect();
    }

    bool EditorInteractionSystemComponent::InternalHandleMouseViewportInteraction(
        const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        return m_interactionRequests->InternalHandleMouseViewportInteraction(mouseInteraction);
    }

    bool EditorInteractionSystemComponent::InternalHandleMouseManipulatorInteraction(
        const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        return m_interactionRequests->InternalHandleMouseManipulatorInteraction(mouseInteraction);
    }

    const EditorVisibleEntityDataCacheInterface* EditorInteractionSystemComponent::GetEntityDataCache() const
    {
        return m_entityDataCache.get();
    }

    void EditorInteractionSystemComponent::SetHandler(
        const ViewportSelectionRequestsBuilderFn& interactionRequestsBuilder)
    {
        // when setting a handler, make sure we're connected to the ViewportDebugDisplayEventBus so we
        // can forward calls to the specific type implementing ViewportSelectionRequests
        if (!AzFramework::ViewportDebugDisplayEventBus::Handler::BusIsConnected())
        {
            AzFramework::ViewportDebugDisplayEventBus::Handler::BusConnect(GetEntityContextId());
        }

        // temporarily disconnect from EditorInteractionSystemViewportSelectionRequestBus in case during the creation of
        // m_interactionRequests (see interactionRequestsBuilder below) an event is propagated to the handler, if this happens then
        // m_interactionRequests will be null as it will not have finished being created yet so we ensure no events are forwarded to it
        EditorInteractionSystemViewportSelectionRequestBus::Handler::BusDisconnect();

        {
            m_entityDataCache = AZStd::make_unique<EditorVisibleEntityDataCache>();
            m_interactionRequests.reset(); // BusConnect/Disconnect in constructor/destructor,
                                           // so have to reset before assigning the new one
            m_interactionRequests = interactionRequestsBuilder(m_entityDataCache.get(), m_viewportEditorMode.get());
        }

        EditorInteractionSystemViewportSelectionRequestBus::Handler::BusConnect(GetEntityContextId());
    }

    void EditorInteractionSystemComponent::SetDefaultHandler()
    {
        SetHandler(
            [](const EditorVisibleEntityDataCacheInterface* entityDataCache, ViewportEditorModeTrackerInterface* viewportEditorModeTracker)
            {
                return AZStd::make_unique<EditorDefaultSelection>(entityDataCache, viewportEditorModeTracker);
            });
    }

    void EditorInteractionSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorInteractionSystemComponent, AZ::Component>()->Version(0);
        }
    }

    void EditorInteractionSystemComponent::DisplayViewport(
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // calculate which entities are in the view and can be interacted with
        // and cache that data to make iterating/looking it up much faster
        m_entityDataCache->CalculateVisibleEntityDatas(viewportInfo);
        m_interactionRequests->DisplayViewportSelection(viewportInfo, debugDisplay);
    }

    void EditorInteractionSystemComponent::DisplayViewport2d(
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        m_interactionRequests->DisplayViewportSelection2d(viewportInfo, debugDisplay);
    }

    void EditorInteractionSystemComponent::NotifyCentralWidgetInitialized()
    {
        // when first launching the editor set the default editor selection interface
        SetDefaultHandler();
    }
} // namespace AzToolsFramework
