/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Component.h>

class UiSystemInterface
    : public AZ::EBusTraits
{
public:
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

    // Public functions

    //! Register a component type with the UI system.
    //! The order in which component types are registered is the order that they show up in
    //! the add component and in the properties pane.
    //! This will go away once there is a system that orders things the way we want based
    //! on the existing component registration.
    virtual void RegisterComponentTypeForMenuOrdering([[maybe_unused]] const AZ::Uuid& typeUuid) {}

    //! Get the UI component types registered with the UI system
    //! This is a short-term solution until there is a way to get the registered components
    //! from the framework in an order that we want for the menus and the properties pane
    virtual const AZStd::vector<AZ::Uuid>* GetComponentTypesForMenuOrdering() = 0;

    //! We use this for metrics to find out which components are part of the LyShine Gem
    virtual const AZStd::list<AZ::ComponentDescriptor*>* GetLyShineComponentDescriptors() = 0;
};

using UiSystemBus = AZ::EBus<UiSystemInterface>;

