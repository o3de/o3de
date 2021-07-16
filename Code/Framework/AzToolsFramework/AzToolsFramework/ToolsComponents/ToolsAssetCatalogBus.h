/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzToolsFramework
{
    //! This EBUS is used to tell the assetprocessor about an asset type that should be retrieved from the source instead of the cache
    class ToolsAssetSystemRequests :
        public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides - Application is a singleton
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using MutexType = AZStd::recursive_mutex;
        //////////////////////////////////////////////////////////////////////////

        virtual ~ToolsAssetSystemRequests() = default;

        //! Register an asset type that should return a stream to the source instead of the product
        virtual void RegisterSourceAssetType(const AZ::Data::AssetType& assetType, const char* assetFileFilter) = 0;
        //! Unregister an asset type as a source, reverting to returning the product stream instead
        virtual void UnregisterSourceAssetType(const AZ::Data::AssetType& assetType) = 0;
    };

    using ToolsAssetSystemBus = AZ::EBus<ToolsAssetSystemRequests>;
}
