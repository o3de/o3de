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

#include <Atom/RPI.Reflect/Image/StreamingImageControllerAsset.h>

namespace AZ
{
    namespace RPI
    {
        class DefaultStreamingImageControllerAsset
            : public StreamingImageControllerAsset
        {
        public:
            AZ_RTTI(DefaultStreamingImageControllerAsset, "{7CCA94AC-3CFC-4754-BD5E-99E42A752A7D}", StreamingImageControllerAsset);
            AZ_CLASS_ALLOCATOR(DefaultStreamingImageControllerAsset, SystemAllocator, 0);

            static const Data::AssetId BuiltInAssetId;

            static void Reflect(AZ::ReflectContext* context);

            DefaultStreamingImageControllerAsset();
        };
    }
}
