/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/Event.h>
#include <AzFramework/Viewport/ViewportId.h>
#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>

namespace AzToolsFramework
{
    //! Enumeration of each viewport editor state.
    enum class ViewportEditorMode : AZ::u8
    {
        Default,
        Component,
        Focus,
        Pick
    };

    //! Viewport identifier and other relevant viewport data.
    struct ViewportEditorModeInfo
    {
        using IdType = AzFramework::ViewportId;
        IdType m_id = ViewportUi::DefaultViewportId; //!< The unique identifier for a given viewport.
    };

    //! Interface for the editor mode state of a given viewport.
    class ViewportEditorModeStateInterface
    {
    public:
        virtual ~ViewportEditorModeStateInterface() = default;

        //! Returns true if the specified editor mode is active, otherwise false.
        virtual bool IsModeActive(ViewportEditorMode mode) const = 0;
    };

    //! Provides a bus to notify when the different editor modes are entered/exit.
    class ViewportEditorModeNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ViewportEditorModeInfo::IdType;
        //////////////////////////////////////////////////////////////////////////

        //! Notifies subscribers of the a given viewport to the entering of the specified editor mode.
        virtual void OnEditorModeEnter([[maybe_unused]] const ViewportEditorModeStateInterface& editorModeState, [[maybe_unused]] ViewportEditorMode mode)
        {
        }

        //! Notifies subscribers of the a given viewport to the exiting of the specified editor mode.
        virtual void OnEditorModeExit([[maybe_unused]] const ViewportEditorModeStateInterface& editorModeState, [[maybe_unused]] ViewportEditorMode mode)
        {
        }
    };
    using ViewportEditorModeNotificationsBus = AZ::EBus<ViewportEditorModeNotifications>;
}
