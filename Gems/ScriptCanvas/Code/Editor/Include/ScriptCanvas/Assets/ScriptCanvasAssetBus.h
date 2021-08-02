/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace ScriptCanvas
{

    class ScriptCanvasAssetBusRequests : public AZ::EBusTraits
    {
    public:

        using BusIdType = AZ::Data::AssetId;
        using MutexType = AZStd::recursive_mutex;

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        virtual void SetAsNewAsset() = 0;

    };

    using ScriptCanvasAssetBusRequestBus = AZ::EBus<ScriptCanvasAssetBusRequests>;


}
