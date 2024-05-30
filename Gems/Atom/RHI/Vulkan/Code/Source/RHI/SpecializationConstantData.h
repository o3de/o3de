/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/PipelineStateDescriptor.h>
#include <AzCore/std/containers/vector.h>

namespace AZ::Vulkan
{
    class SpecializationConstantData
    {
    public:
        SpecializationConstantData() = default;

        RHI::ResultCode Init(const RHI::PipelineStateDescriptor& descriptor);
        void Shutdown();

        const VkSpecializationInfo* GetVkSpecializationInfo() const;

    private:
        VkSpecializationInfo m_specializationInfo{};
        AZStd::vector<VkSpecializationMapEntry> m_specializationMap;
        AZStd::vector<uint8_t> m_specializationData;
    };
}
