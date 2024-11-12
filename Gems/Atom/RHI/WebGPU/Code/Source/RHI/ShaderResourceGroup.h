/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceShaderResourceGroup.h>
#include <AzCore/std/containers/vector.h>

namespace AZ::WebGPU
{
    class BindGroup;

    class ShaderResourceGroup
        : public RHI::DeviceShaderResourceGroup
    {
        using Base = RHI::DeviceShaderResourceGroup;
        friend class ShaderResourceGroupPool;

    public:
        AZ_CLASS_ALLOCATOR(ShaderResourceGroup, AZ::SystemAllocator);

        static RHI::Ptr<ShaderResourceGroup> Create();

        void UpdateCompiledDataIndex(uint64_t frameIteration);
        const BindGroup& GetCompiledData() const;
        uint32_t GetCompileDataIndex() const;
        uint64_t GetLastCompileFrameIteration() const;

    protected:
        ShaderResourceGroup() = default;

    private:
        uint32_t m_compiledDataIndex = 0;
        uint64_t m_lastCompileFrameIteration = 0;
        AZStd::vector<RHI::Ptr<BindGroup>> m_compiledData;
    };
}
