/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/Event.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>

namespace AzToolsFramework
{
    //! Enumeration of each viewport editor mode.
    enum class ViewportEditorMode : AZ::u8
    {
        Default,
        Component,
        Focus,
        Pick
    };

    //! Viewport editor mode tracker identifier and other relevant data.
    struct ViewportEditorModeTrackerInfo
    {
        using IdType = AzFramework::EntityContextId;
        IdType m_id = AzFramework::EntityContextId::CreateNull(); //!< The unique identifier for a given viewport editor mode tracker.
    };

    //! Interface for the editor modes of a given viewport.
    class ViewportEditorModesInterface
    {
    public:
        AZ_RTTI(ViewportEditorModesInterface, "{2421496C-4A46-41C9-8AEF-AE2B6E43E6CF}");

        virtual ~ViewportEditorModesInterface() = default;

        //! Returns true if the specified editor mode is active, otherwise false.
        virtual bool IsModeActive(ViewportEditorMode mode) const = 0;
    };

    //! Provides a bus to notify when the different editor modes are entered/exit.
    //! @note The editor modes are not discrete states but rather each progression of mode retain the active the parent
    //! mode that the new mode progressed from.
    class ViewportEditorModeNotifications : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ViewportEditorModeTrackerInfo::IdType;
        //////////////////////////////////////////////////////////////////////////

        AZ_RTTI(ViewportEditorModeNotifications, "{9469DE39-6C21-423C-94FA-EF3A9616B14F}", AZ::EBusTraits);
        static void Reflect(AZ::ReflectContext* context);

        //! Notifies subscribers of the a given viewport to the activation of the specified editor mode.
        virtual void OnEditorModeActivated(
            [[maybe_unused]] const ViewportEditorModesInterface& editorModeState, [[maybe_unused]] ViewportEditorMode mode)
        {
        }

        //! Notifies subscribers of the a given viewport to the deactivation of the specified editor mode.
        virtual void OnEditorModeDeactivated(
            [[maybe_unused]] const ViewportEditorModesInterface& editorModeState, [[maybe_unused]] ViewportEditorMode mode)
        {
        }
    };

    using ViewportEditorModeNotificationsBus = AZ::EBus<ViewportEditorModeNotifications>;
} // namespace AzToolsFramework
