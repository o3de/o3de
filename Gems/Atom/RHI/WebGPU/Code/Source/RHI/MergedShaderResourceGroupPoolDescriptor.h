/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ShaderResourceGroupPoolDescriptor.h>
#include <RHI/BindGroupLayout.h>

namespace AZ::WebGPU
{
    class MergedShaderResourceGroupPoolDescriptor
        : public RHI::ShaderResourceGroupPoolDescriptor
    {
    public:
        virtual ~MergedShaderResourceGroupPoolDescriptor() = default;
        AZ_CLASS_ALLOCATOR(MergedShaderResourceGroupPoolDescriptor, SystemAllocator)
        AZ_RTTI(MergedShaderResourceGroupPoolDescriptor, "{5D0E0DBB-FC94-45E8-A014-9A5BD1F6C97F}");

        MergedShaderResourceGroupPoolDescriptor() = default;

        RHI::Ptr<BindGroupLayout> m_bindGroupLayout;
    };
}
