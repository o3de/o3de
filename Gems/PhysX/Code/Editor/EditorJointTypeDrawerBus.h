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
