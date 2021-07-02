/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

namespace PhysX
{
    class EditorJointTypeDrawer;

    using EditorJointType = AZ::Uuid;
    using EditorSubComponentModeNameCrc = AZ::Crc32;
    using EditorJointTypeDrawerId = AZStd::pair<EditorJointType, EditorSubComponentModeNameCrc>;

    /// The sub-component mode of a component type uses this bus (by invoking GetEditorJointTypeDrawer) to retrieve a drawer.
    /// If nothing is returned, it creates an instance of the drawer that will be shared by other instances of the same component type.
    class EditorJointTypeDrawerRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = EditorJointTypeDrawerId;

        virtual AZStd::shared_ptr<EditorJointTypeDrawer> GetEditorJointTypeDrawer() = 0;
    };
    using EditorJointTypeDrawerBus = AZ::EBus<EditorJointTypeDrawerRequests>;
} // namespace PhysX
