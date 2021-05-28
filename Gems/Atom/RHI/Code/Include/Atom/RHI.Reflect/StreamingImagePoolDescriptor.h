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

#include <Atom/RHI.Reflect/ResourcePoolDescriptor.h>

namespace AZ
{
    class ReflectContext;

    namespace RHI
    {
        class StreamingImagePoolDescriptor
            : public ResourcePoolDescriptor
        {
        public:
            AZ_RTTI(StreamingImagePoolDescriptor, "{67F5A1DB-37B9-4959-A04F-4A7C939DD853}", ResourcePoolDescriptor);
            AZ_CLASS_ALLOCATOR(StreamingImagePoolDescriptor, SystemAllocator, 0);

            static void Reflect(AZ::ReflectContext* context);

            StreamingImagePoolDescriptor() = default;
            virtual ~StreamingImagePoolDescriptor() = default;

            // Currently empty.
        };
    }
}
