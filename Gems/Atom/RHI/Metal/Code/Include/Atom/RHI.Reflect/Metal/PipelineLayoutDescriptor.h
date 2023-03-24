/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ::Serialize
{
    template<class T, bool U, bool A>
    struct InstanceFactory;
}
namespace AZ
{
    template<typename ValueType, typename>
    struct AnyTypeInfoConcept;
}

namespace AZ
{
    class ReflectContext;

    namespace Metal
    {
        using SlotToIndexTable = AZStd::array<uint8_t, RHI::Limits::Pipeline::ShaderResourceGroupCountMax>;
        using IndexToSlotTable = AZStd::fixed_vector<uint8_t, RHI::Limits::Pipeline::ShaderResourceGroupCountMax>;


        //! This class describes the usage mask for an Shader Resource
        //! Group that is part of a Pipeline.
        //! Contains a mask that describes in which shader stage a resource
        //! is being used.
        class ShaderResourceGroupVisibility
        {
        public:
            AZ_TYPE_INFO(ShaderResourceGroupVisibility, "{7E565D57-6388-45F5-A8AC-AF160D30ABBD}");

            static void Reflect(AZ::ReflectContext* context);

            ShaderResourceGroupVisibility() = default;
            HashValue64 GetHash(HashValue64 seed = HashValue64{ 0 }) const;

            /// Shader usage mask for each resource.
            AZStd::unordered_map<AZ::Name, RHI::ShaderStageMask> m_resourcesStageMask;
            /// Shader usage mask for the constant data. All constants share the same usage mask.
            RHI::ShaderStageMask m_constantDataStageMask = RHI::ShaderStageMask::None;
        };

        //! Describes root constant binding information.
        struct RootConstantBinding
        {
            AZ_TYPE_INFO(RootConstantBinding, "{28679FD9-6056-4803-8351-A112A2FB00A3}");

            static void Reflect(AZ::ReflectContext* context);

            RootConstantBinding() = default;
            RootConstantBinding(
                uint32_t constantRegister,
                uint32_t constantRegisterSpace
            );

            HashValue64 GetHash(HashValue64 seed = HashValue64{ 0 }) const;

            uint32_t m_constantRegister = 0;
            uint32_t m_constantRegisterSpace = 0;
        };

        class PipelineLayoutDescriptor final
            : public RHI::PipelineLayoutDescriptor
        {
            using Base = RHI::PipelineLayoutDescriptor;
        public:
            AZ_RTTI(PipelineLayoutDescriptor, "{BC89E796-AB67-40EA-BE56-9F4B5975E0C8}", Base);
            AZ_CLASS_ALLOCATOR(PipelineLayoutDescriptor, AZ::SystemAllocator);
            static void Reflect(AZ::ReflectContext* context);

            static RHI::Ptr<PipelineLayoutDescriptor> Create();

            void SetBindingTables(const SlotToIndexTable& slotToIndexTable, const IndexToSlotTable& indexToSlotTable);
            const SlotToIndexTable& GetSlotToIndexTable() const;
            const IndexToSlotTable& GetIndexToSlotTable() const;

            void AddShaderResourceGroupVisibility(const ShaderResourceGroupVisibility& visibilityInfo);
            const ShaderResourceGroupVisibility& GetShaderResourceGroupVisibility(uint32_t index) const;

            void SetRootConstantBinding(const RootConstantBinding& rootConstantBinding);
            const RootConstantBinding& GetRootConstantBinding() const;
        private:

            PipelineLayoutDescriptor() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::PipelineLayoutDescriptor overrides
            void ResetInternal() override;
            HashValue64 GetHashInternal(HashValue64 seed) const override;
            //////////////////////////////////////////////////////////////////////////

            template <typename, typename>
            friend struct AnyTypeInfoConcept;
            template <typename, bool, bool>
            friend struct Serialize::InstanceFactory;

            // Binding slot to SRG index tables ...
            SlotToIndexTable m_slotToIndexTable;
            IndexToSlotTable m_indexToSlotTable;

            /// Visibility info for each Shader Resource Group.
            AZStd::fixed_vector<ShaderResourceGroupVisibility, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_shaderResourceGroupVisibilities;

            RootConstantBinding m_rootConstantBinding;
        };
    }
}
