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

    //! @deprecated Use standard Category attributes in component EditContext reflection instead.
    //! The add-component menu now uses the standard ComponentPaletteWidget with Category-based organization.
    //! This function is only used for legacy component ordering in the properties pane.
    virtual void RegisterComponentTypeForMenuOrdering([[maybe_unused]] const AZ::Uuid& typeUuid) {}

    //! @deprecated Use standard Category attributes in component EditContext reflection instead.
    //! The add-component menu now uses the standard ComponentPaletteWidget with Category-based organization.
    //! This function is only used for legacy component ordering in the properties pane.
    virtual const AZStd::vector<AZ::Uuid>* GetComponentTypesForMenuOrdering() = 0;

    //! We use this for metrics to find out which components are part of the LyShine Gem
    virtual const AZStd::list<AZ::ComponentDescriptor*>* GetLyShineComponentDescriptors() = 0;
};

using UiSystemBus = AZ::EBus<UiSystemInterface>;

