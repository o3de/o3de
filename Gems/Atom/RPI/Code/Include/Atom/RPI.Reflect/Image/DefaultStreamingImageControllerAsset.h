/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
