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
    //! Contains the vulkan structure needed for using specialization constants
    class SpecializationConstantData
    {
    public:
        SpecializationConstantData() = default;

        //! Initialize the contents with the specialization constants information of the pipeline descriptor
        RHI::ResultCode Init(const RHI::PipelineStateDescriptor& descriptor);
        //! Release any data previously used
        void Shutdown();

        //! Returns the vulkan specialization info
        const VkSpecializationInfo* GetVkSpecializationInfo() const;

    private:
        // Vulkan structure for using specialization constants
        VkSpecializationInfo m_specializationInfo{};
        // Vector with the mapping information of the specialization constants (ids and offsets).
        AZStd::vector<VkSpecializationMapEntry> m_specializationMap;
        // Memory buffer with the values of the specialization constants
        AZStd::vector<uint8_t> m_specializationData;
    };
}
