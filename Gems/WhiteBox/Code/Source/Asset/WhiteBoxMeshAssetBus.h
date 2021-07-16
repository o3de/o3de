/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>

namespace WhiteBox
{
    //! Notification bus for white box mesh asset modifications.
    class WhiteBoxMeshAssetNotifications : public AZ::EBusTraits
    {
    public:
        // EBusTraits overrides
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::Data::AssetId;

        //! Raised when the asset has been modified.
        //! This is so other white box components referencing the same asset will know to update their render mesh.
        virtual void OnWhiteBoxMeshAssetModified([[maybe_unused]] AZ::Data::Asset<AZ::Data::AssetData> asset) {}
    };

    using WhiteBoxMeshAssetNotificationBus = AZ::EBus<WhiteBoxMeshAssetNotifications>;
} // namespace WhiteBox
