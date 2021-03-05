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
