/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ShaderResourceGroupLayout.h>
#include <Atom/RHI.Reflect/ResourcePoolDescriptor.h>

namespace AZ::RHI
{
    enum class ShaderResourceGroupUsage : uint32_t
    {
        // The resource set is persistent across frames and will be updated rarely.
        Persistent = 0,

        // The resource group is optimized for once-per-frame updates. Its contents become invalid
        // after the current frame has completed.
        Transient
    };

    class ShaderResourceGroupPoolDescriptor
        : public ResourcePoolDescriptor
    {
    public:
        virtual ~ShaderResourceGroupPoolDescriptor() = default;
        AZ_CLASS_ALLOCATOR(ShaderResourceGroupPoolDescriptor, SystemAllocator)
        AZ_RTTI(ShaderResourceGroupPoolDescriptor, "{7074E7D1-B98D-48C7-8622-D6635CAEFBB4}");
        static void Reflect(AZ::ReflectContext* context);

        ShaderResourceGroupPoolDescriptor() = default;

        /// [Not Serialized] The resource group layout shared by all resource groups in the pool.
        RHI::ConstPtr<ShaderResourceGroupLayout> m_layout;

        /// The usage type used to update resource groups.
        ShaderResourceGroupUsage m_usage = ShaderResourceGroupUsage::Persistent;
    };
}
