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

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/EBus/EBus.h>

namespace SliceFavorites
{
    class SliceFavoritesRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual size_t GetNumFavorites() = 0;

        /**
        * Enumerates all favorite slices and invokes the specified callback for each one, passing the assetID of the slice
        * @param callback A reference to the callback that is invoked for each favorite.
        */
        virtual void EnumerateFavorites(const AZStd::function<void(const AZ::Data::AssetId& assetId)>& callback) = 0;
    };
    using SliceFavoritesRequestBus = AZ::EBus<SliceFavoritesRequests>;
} // namespace SliceFavorites
