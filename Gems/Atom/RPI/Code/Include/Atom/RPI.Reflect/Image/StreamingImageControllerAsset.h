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

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Asset/AssetCommon.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        class StreamingImageController;

        //! The streaming image controller descriptor is a configuration data block used to instantiate a
        //! concrete streaming image controller instance. Users should derive from this class, store
        //! configuration data on it for your specific back-end, and implement the factory method to create
        //! an instance at runtime.
        class StreamingImageControllerAsset
            : public Data::AssetData
        {
        public:
            AZ_RTTI(StreamingImageControllerAsset, "{0797F48F-B097-4209-8D6F-DA40DC0FAB42}", Data::AssetData);
            AZ_CLASS_ALLOCATOR(StreamingImageControllerAsset, SystemAllocator, 0);

            static void Reflect(AZ::ReflectContext* context);

            virtual ~StreamingImageControllerAsset() = default;
        };
    }
}
