/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzToolsFramework
{
    class EditorVisibleEntityDataCache;
    class ViewportEditorModeTrackerInterface;

    //! Bus to handle all mouse events originating from the viewport.
    //! Coordinated by the EditorInteractionSystemComponent
    class EditorInteractionSystemViewportSelectionRequests : public AZ::EBusTraits
    {
    public:
        using BusIdType = AzFramework::EntityContextId;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

    protected:
        ~EditorInteractionSystemViewportSelectionRequests() = default;
    };

    //! Alias for factory function to create a new type implementing the ViewportSelectionRequests interface.
    using ViewportSelectionRequestsBuilderFn = AZStd::function<AZStd::unique_ptr<ViewportInteraction::InternalViewportSelectionRequests>(
        const EditorVisibleEntityDataCache*, ViewportEditorModeTrackerInterface*)>;

    //! Interface for system component implementing the ViewportSelectionRequests interface.
    //! This interface also includes a setter to set a custom handler also implementing
    //! the ViewportSelectionRequests interface to customize editor behavior.
    class EditorInteractionSystemViewportSelection : public ViewportInteraction::InternalViewportSelectionRequests
    {
    public:
        //! \ref SetHandler takes a factory function to create a new type implementing
        //! the ViewportSelectionRequests interface.
        //! It provides a handler implementing ViewportSelectionRequests to handle all
        //! viewport mouse input and drawing.
        virtual void SetHandler(const ViewportSelectionRequestsBuilderFn& interactionRequestsBuilder) = 0;
        //! \ref SetDefaultHandler is a utility function to set the
        //! default editor handler (currently \ref EditorDefaultSelection).
        //! This is useful to call after setting another mode and then wishing
        //! to return to normal operation of the editor.
        virtual void SetDefaultHandler() = 0;
    };

    //! Type to inherit to implement EditorInteractionSystemViewportSelection.
    //! @note Called by viewport events (RenderViewport) and then handled by concrete
    //! implementation of InternalViewportSelectionRequests.
    using EditorInteractionSystemViewportSelectionRequestBus =
        AZ::EBus<EditorInteractionSystemViewportSelection, EditorInteractionSystemViewportSelectionRequests>;

} // namespace AzToolsFramework
