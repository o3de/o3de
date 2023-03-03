/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>
#include <Atom/RHI.Reflect/Handle.h>
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

    namespace DX12
    {
        struct RootParameter {
            AZ_TYPE_INFO(RootParameter, "{11422c67-2f4e-4216-88aa-d3722a79387e}");
        };

        using RootParameterIndex = RHI::Handle<uint16_t, RootParameter>;

        /**
         * Describes the root parameter binding for each component of a DX12 shader resource group.
         */
        class RootParameterBinding
        {
        public:
            AZ_TYPE_INFO(RootParameterBinding, "{1E396986-F6B5-4E46-8FAE-2DBA4B697883}");

            /// The root index of the SRG root constant buffer (if it exists).
            RootParameterIndex m_constantBuffer;

            /// The root index of the SRG resource descriptor table (if it exists).
            RootParameterIndex m_resourceTable;

            /// The root indices of the SRG unbounded array resource descriptor tables (if any).
            /// TODO: This restriction should be lifted
            static const uint32_t MaxUnboundedArrays = 2;
            RootParameterIndex m_unboundedArrayResourceTables[MaxUnboundedArrays];

            /// If unbounded arrays are present, the bindless parameter index refers to the root argument designated for the bindless table
            RootParameterIndex m_bindlessTable;

            /// The root index of the SRG sampler descriptor table (if it exists).
            RootParameterIndex m_samplerTable;

            static void Reflect(AZ::ReflectContext* context);
        };

        /**
         * Describes root constant binding information.
         */
        struct RootConstantBinding
        {
            AZ_TYPE_INFO(RootConstantBinding, "{31F53B97-FEB4-4714-98B1-7706FFA8A246}");

            static void Reflect(AZ::ReflectContext* context);

            RootConstantBinding() = default;
            RootConstantBinding(
                uint32_t constantCount,
                uint32_t constantRegister,
                uint32_t constantRegisterSpace
            );

            HashValue64 GetHash(HashValue64 seed = HashValue64{ 0 }) const;

            uint32_t m_constantCount = 0;
            uint32_t m_constantRegister = 0;
            uint32_t m_constantRegisterSpace = 0;
        };

        /**
         * Describes the shader stage mask for the
         * descriptor table used by the SRG.
         */
        struct ShaderResourceGroupVisibility
        {
            AZ_TYPE_INFO(ShaderResourceGroupVisibility, "{58B0A184-E7BA-408D-BC6C-8ACEA8CD8E8F}");

            static void Reflect(AZ::ReflectContext* context);

            ShaderResourceGroupVisibility() = default;

            HashValue64 GetHash(HashValue64 seed = HashValue64{ 0 }) const;

            RHI::ShaderStageMask m_descriptorTableShaderStageMask = RHI::ShaderStageMask::None;
        };

        class PipelineLayoutDescriptor final
            : public RHI::PipelineLayoutDescriptor
        {
            using Base = RHI::PipelineLayoutDescriptor;
        public:
            AZ_RTTI(PipelineLayoutDescriptor, "{A10B0F03-F43D-4462-9306-66195B4EFC46}", Base);
            AZ_CLASS_ALLOCATOR(PipelineLayoutDescriptor, AZ::SystemAllocator);
            static void Reflect(AZ::ReflectContext* context);

            static RHI::Ptr<PipelineLayoutDescriptor> Create();

            void SetRootConstantBinding(const RootConstantBinding& rootConstantBinding);

            const RootConstantBinding& GetRootConstantBinding() const;

            void AddShaderResourceGroupVisibility(const ShaderResourceGroupVisibility& shaderResourceGroupVisibility);

            const ShaderResourceGroupVisibility& GetShaderResourceGroupVisibility(uint32_t index) const;

        private:
            PipelineLayoutDescriptor() = default;

            //////////////////////////////////////////////////////////////////////////
            /// PipelineLayoutDescriptor
            HashValue64 GetHashInternal(HashValue64 seed) const override;
            //////////////////////////////////////////////////////////////////////////

            template <typename, typename>
            friend struct AnyTypeInfoConcept;
            template <typename, bool, bool>
            friend struct Serialize::InstanceFactory;

            RootConstantBinding m_rootConstantBinding;
            AZStd::fixed_vector<ShaderResourceGroupVisibility, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_shaderResourceGroupVisibilities;
        };
    }
}
