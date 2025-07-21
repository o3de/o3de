/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <LyShine/UiBase.h>

    // The event bus that interfaces with the UiHierarchyInteractivityToggle Component.
    // Used to toggle interactivity to the current entity and descendants.
    class UiHierarchyInteractivityToggleInterface
        : public AZ::ComponentBus
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //Interactive
        //! The root event call used to manipulate the Interactive state.
        virtual bool SetInteractivity(bool enabled) = 0;
        virtual bool SetParentInteractivity(bool parentEnabled) = 0;
        //! Getter to see the current interactive state.
        virtual bool GetInteractiveState() = 0;
    };   

    typedef AZ::EBus<UiHierarchyInteractivityToggleInterface> UiHierarchyInteractivityToggleBus;
