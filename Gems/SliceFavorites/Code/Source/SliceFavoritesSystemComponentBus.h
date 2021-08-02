/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/EBus/EBus.h>

namespace SliceFavorites
{
    class FavoriteDataModel;

    class SliceFavoritesSystemComponentRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual FavoriteDataModel* GetSliceFavoriteDataModel() = 0;
    };
    using SliceFavoritesSystemComponentRequestBus = AZ::EBus<SliceFavoritesSystemComponentRequests>;
} // namespace SliceFavorites
