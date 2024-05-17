/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/ShaderResourceGroup.h>
#include <AzCore/std/parallel/shared_mutex.h>

namespace AZ
{
    namespace Vulkan
    {
        class MergedShaderResourceGroupPool;

        //! Represents a group of ShaderResourceGroups that were merged together due to device limitations.
        //! At submit time (the moment that we know all SRGs being used), a MergedShaderResourceGroup is created 
        //! and the compiled data from the ShaderResourceGroups is copied (compiled) to the MergedShaderResourceGroup. 
        //! This process is automatic and it happens transparent to the user. MergedShaderResourceGroup are cached in
        //! the MergedShaderResourceGroupPool so they can be reused as much as possible.
        class MergedShaderResourceGroup final
            : public ShaderResourceGroup
        {
            using Base = ShaderResourceGroup;
            friend class MergedShaderResourceGroupPool;

        public:
            AZ_CLASS_ALLOCATOR(MergedShaderResourceGroup, AZ::SystemAllocator);
            AZ_RTTI(MergedShaderResourceGroup, "{5A98B01F-E5AC-4648-B307-08371FD8AA56}", Base);

            ~MergedShaderResourceGroup() = default;

            //! Returns if the MergedShaderResourceGroup needs to be compiled (copy compile data from original SRGs) before it can be used.
            bool NeedsCompile() const;

            //! Compile the MergedShaderResourceGroup synchronously using the compiled data of the ShaderResourceGroups instances.
            void Compile();

            //! Generates the name of a merged Shader Input.
            static AZ::Name GenerateMergedShaderInputName(const AZ::Name& shaderInputName, uint32_t bindinSlot);

            using ShaderResourceGroupArray = AZStd::array<const ShaderResourceGroup*, RHI::Limits::Pipeline::ShaderResourceGroupCountMax>;

            //! Suffix name used for a constant buffer that will contain the constant data of a merged SRG.
            static const char* ConstantDataBufferName;

        private:
            MergedShaderResourceGroup() = default;

            // Helper struct for easy initialization of the frame iteration.
            struct FrameIteration
            {
                uint64_t m_frameIteration = std::numeric_limits<uint64_t>::max();
            };

            // Utility function that merges multiple ShaderResoruceGroup data into one.
            RHI::DeviceShaderResourceGroupData MergeShaderResourceData(const ShaderResourceGroupArray& srgList) const;
            // List of the ShaderResourceGroup instances that are being merged.
            ShaderResourceGroupArray m_mergedShaderResourceGroupList = {};
            // Keeps track of the frame iteration for each ShaderResourceGroup when the last compile happens. This is used
            // to calculate if we need to compile again.
            AZStd::array<FrameIteration, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_lastCompileFrameIteration;

            mutable AZStd::shared_mutex m_compileMutex;
        };
    }
}

namespace AZStd
{
    template<>
    struct hash<AZ::Vulkan::MergedShaderResourceGroup::ShaderResourceGroupArray>
    {
        using argument_type = AZ::Vulkan::MergedShaderResourceGroup::ShaderResourceGroupArray;
        using result_type = AZStd::size_t;
        inline result_type operator()(const argument_type& value) const { return AZStd::hash_range(value.begin(), value.end()); }
    };
} // namespace AZStd
