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

#pragma once

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>
#include <AzToolsFramework/ViewportSelection/EditorVisibleEntityDataCache.h>

namespace AzToolsFramework
{
    //! System Component to wrap active input handler.
    //! EditorInteractionSystemComponent is notified of viewport mouse events from RenderViewport
    //! and forwards them to a concrete implementation of ViewportSelectionRequests.
    class EditorInteractionSystemComponent
        : public AZ::Component
        , private EditorInteractionSystemViewportSelectionRequestBus::Handler
        , private AzFramework::ViewportDebugDisplayEventBus::Handler
        , private EditorEventsBus::Handler
    {
    public:
        AZ_COMPONENT(EditorInteractionSystemComponent, "{146D0317-AF42-45AB-A953-F54198525DD5}")

        static void Reflect(AZ::ReflectContext* context);

        // EditorInteractionSystemViewportSelectionRequestBus
        void SetHandler(const ViewportSelectionRequestsBuilderFn& interactionRequestsBuilder) override;
        void SetDefaultHandler() override;

        // EditorInteractionSystemViewportSelectionRequestBus ...
        bool InternalHandleMouseViewportInteraction(const ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;
        bool InternalHandleMouseManipulatorInteraction(const ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;

        // AzFramework::ViewportDebugDisplayEventBus
        void DisplayViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;
        void DisplayViewport2d(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

    private:
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // EditorEventsBus
        void NotifyCentralWidgetInitialized() override;

        AZStd::unique_ptr<EditorVisibleEntityDataCache> m_entityDataCache = nullptr; //!< Visible EntityData cache to be used by concrete
                                                                                     //!< instantiations of ViewportSelectionRequests.

        AZStd::unique_ptr<InternalViewportSelectionRequests> m_interactionRequests; //!< Hold a concrete implementation of
                                                                                    //!< ViewportSelectionRequests to handle viewport
                                                                                    //!< input and drawing for the Editor.
    };
} // namespace AzToolsFramework
