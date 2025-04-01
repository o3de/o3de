/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ResourcePoolDescriptor.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::RHI
{
    class StreamingImagePoolDescriptor
        : public ResourcePoolDescriptor
    {
    public:
        AZ_RTTI(StreamingImagePoolDescriptor, "{67F5A1DB-37B9-4959-A04F-4A7C939DD853}", ResourcePoolDescriptor);
        AZ_CLASS_ALLOCATOR(StreamingImagePoolDescriptor, SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        StreamingImagePoolDescriptor() = default;
        virtual ~StreamingImagePoolDescriptor() = default;

        // Currently empty.
    };
}
