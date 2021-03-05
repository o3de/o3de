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

#include "EditorInteractionSystemComponent.h"

#include <AzToolsFramework/ViewportSelection/EditorDefaultSelection.h>
#include <AzToolsFramework/ViewportSelection/EditorVisibleEntityDataCache.h>

namespace AzToolsFramework
{
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

    void EditorInteractionSystemComponent::SetHandler(
        const ViewportSelectionRequestsBuilderFn& interactionRequestsBuilder)
    {
        // when setting a handler, make sure we're connected to the ViewportDebugDisplayEventBus so we
        // can forward calls to the specific type implementing ViewportSelectionRequests
        if (!AzFramework::ViewportDebugDisplayEventBus::Handler::BusIsConnected())
        {
            AzFramework::ViewportDebugDisplayEventBus::Handler::BusConnect(GetEntityContextId());
        }

        m_entityDataCache = AZStd::make_unique<EditorVisibleEntityDataCache>();

        m_interactionRequests.reset(); // BusConnect/Disconnect in constructor/destructor, 
                                       // so have to reset before assigning the new one
        m_interactionRequests = interactionRequestsBuilder(m_entityDataCache.get());
    }

    void EditorInteractionSystemComponent::SetDefaultHandler()
    {
        SetHandler([](const EditorVisibleEntityDataCache* entityDataCache)
        {
            return AZStd::make_unique<EditorDefaultSelection>(entityDataCache);
        });
    }

    void EditorInteractionSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorInteractionSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void EditorInteractionSystemComponent::DisplayViewport(
        const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // calculate which entities are in the view and can be interacted with
        // and cache that data to make iterating/looking it up much faster
        m_entityDataCache->CalculateVisibleEntityDatas(viewportInfo);
        m_interactionRequests->DisplayViewportSelection(viewportInfo, debugDisplay);
    }

    void EditorInteractionSystemComponent::DisplayViewport2d(
        const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        m_interactionRequests->DisplayViewportSelection2d(viewportInfo, debugDisplay);
    }

    void EditorInteractionSystemComponent::NotifyCentralWidgetInitialized()
    {
        // when first launching the editor set the default editor selection interface
        SetDefaultHandler();
    }
} // namespace AzToolsFramework
