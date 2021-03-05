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

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <Atom/RHI/PipelineState.h>
#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI/DeviceObject.h>

namespace AZ
{
    namespace RHI
    {
        //! Contains ray tracing shaders used by the pipeline
        struct RayTracingShaderLibrary
        {
            RHI::PipelineStateDescriptorForRayTracing m_descriptor;
        };
        using RayTracingShaderLibraryVector = AZStd::vector<RayTracingShaderLibrary>;

        //! Defines a hit group which consists of a ClosestHit and/or an AnyHit shader
        struct RayTracingHitGroup
        {
            AZ::Name m_hitGroupName;
            AZ::Name m_closestHitShaderName;
            AZ::Name m_anyHitShaderName;
        };
        using RayTracingHitGroupVector = AZStd::vector<RayTracingHitGroup>;

        //! Defines ray tracing pipeline settings
        struct RayTracingConfiguration
        {
            static const uint32_t MaxPayloadSizeDefault = 16;
            uint32_t m_maxPayloadSize = MaxPayloadSizeDefault;

            static const uint32_t MaxAttributeSizeDefault = 8;
            uint32_t m_maxAttributeSize = MaxAttributeSizeDefault;

            static const uint32_t MaxRecursionDepthDefault = 1;
            uint32_t m_maxRecursionDepth = MaxRecursionDepthDefault;
        };

        //! Parameter entries for a Root Signature
        // [GFX TODO][ATOM-5570] Use the shader PipelineLayout to set DXR root signatures

        //! Cbv Parameter
        struct RayTracingRootSignatureCbvParam
        {
            uint32_t m_rootParameterIndex = 0;
            uint32_t m_shaderRegister = 0;
            uint32_t m_registerSpace = 0;

            bool m_addedToRootSignature = false;
        };

        //! DescriptorTable Parameter
        struct RayTracingRootSignatureDescriptorTableParam
        {
            struct Range
            {
                uint32_t m_shaderRegister = 0;
                uint32_t m_registerSpace = 0;
                enum Type { Constant, Read, ReadWrite };
                Type m_type;
            };
            using RangeVector = AZStd::vector<Range>;

            uint32_t m_rootParameterIndex = 0;
            RangeVector m_ranges;

            bool m_addedToRootSignature = false;
        };

        //! Static Sampler entry for a Root Signature
        struct RayTracingRootSignatureStaticSampler
        {
            uint32_t m_rootParameterIndex = 0;
            uint32_t m_shaderRegister = 0;
            uint32_t m_registerSpace = 0;

            RHI::FilterMode m_filterMode;
            RHI::AddressMode m_addressMode;
        };
        using RayTracingRootSignatureStaticSamplerVector = AZStd::vector<RayTracingRootSignatureStaticSampler>;

        //! Global Root Signature
        struct RayTracingRootSignature
        {
            RayTracingRootSignatureCbvParam m_cbvParam;
            RayTracingRootSignatureDescriptorTableParam m_descriptorTableParam;
            RayTracingRootSignatureStaticSamplerVector m_staticSamplers;
        };

        //! Local Root Signature
        struct RayTracingLocalRootSignature : public RayTracingRootSignature
        {
            AZStd::vector<AZ::Name> m_shaderAssociations; // the ray tracing shaders that use this root signature
        };
        using RayTracingLocalRootSignatureVector = AZStd::vector<RayTracingLocalRootSignature>;

        //! RayTracingPipelineStateDescriptor
        //!
        //! The Build() operation in the descriptor allows the pipeline state to be initialized
        //! using the following pattern:
        //!
        //! RHI::RayTracingPipelineStateDescriptor descriptor;
        //! descriptor.Build()
        //!     ->ShaderLibrary(shaderDescriptor)
        //!     ->HitGroup(AZ::Name("HitGroup1"))
        //!         ->ClosestHitShaderName(AZ::Name("ClosestHitShader1"))
        //!     ->HitGroup(AZ::Name("HitGroup2"))
        //!         ->ClosestHitShaderName(AZ::Name("ClosestHitShader2"))
        //!     ->GlobalRootSignature()
        //!         ->DescriptorTableParameter()
        //!             ->RootParameterIndex(0)
        //!             ->Range()
        //!                 ->ShaderRegister(0)
        //!                 ->RegisterSpace(0)
        //!                 ->RangeType(RHI::RayTracingRootSignatureDescriptorTableParam::Range::Type::Read)
        //!     ->LocalRootSignature()
        //!         ->ShaderAssociation(AZ::Name("HitGroup1"))
        //!         ->CbvParameter()
        //!             ->ShaderRegister(0)
        //!             ->RegisterSpace(1)
        //!     ->LocalRootSignature()
        //!         ->ShaderAssociation(AZ::Name("HitGroup2"))
        //!         ->DescriptorTableParameter()
        //!             ->Range()
        //!                 ->ShaderRegister(0)
        //!                 ->RegisterSpace(2)
        //!                 ->RangeType(RHI::RayTracingRootSignatureDescriptorTableParam::Range::Type::Read)
        //!     ;
        //!
        class RayTracingPipelineStateDescriptor final
        {
        public:
            RayTracingPipelineStateDescriptor() = default;
            ~RayTracingPipelineStateDescriptor() = default;

            // accessors
            const RayTracingConfiguration& GetConfiguration() const { return m_configuration; }
            RayTracingConfiguration& GetConfiguration() { return m_configuration; }

            const RayTracingShaderLibraryVector& GetShaderLibraries() const { return m_shaderLibraries; }
            RayTracingShaderLibraryVector& GetShaderLibraries() { return m_shaderLibraries; }

            const RayTracingHitGroupVector& GetHitGroups() const { return m_hitGroups; }
            RayTracingHitGroupVector& GetHitGroups() { return m_hitGroups; }

            const RayTracingRootSignature& GetGlobalRootSignature() const { return m_globalRootSignature; }
            RayTracingRootSignature& GetGlobalRootSignature() { return m_globalRootSignature; }

            const RayTracingLocalRootSignatureVector& GetLocalRootSignatures() const { return m_localRootSignatures; }
            RayTracingLocalRootSignatureVector& GetLocalRootSignatures() { return m_localRootSignatures; }

            // build operations
            RayTracingPipelineStateDescriptor* Build();
            RayTracingPipelineStateDescriptor* MaxPayloadSize(uint32_t maxPayloadSize);
            RayTracingPipelineStateDescriptor* MaxAttributeSize(uint32_t maxAttributeSize);
            RayTracingPipelineStateDescriptor* MaxRecursionDepth(uint32_t maxRecursionDepth);
            RayTracingPipelineStateDescriptor* ShaderLibrary(RHI::PipelineStateDescriptorForRayTracing& descriptor);

            RayTracingPipelineStateDescriptor* HitGroup(const AZ::Name& name);
            RayTracingPipelineStateDescriptor* ClosestHitShaderName(const AZ::Name& closestHitShaderName);
            RayTracingPipelineStateDescriptor* AnyHitShaderName(const AZ::Name& anyHitShaderName);

            RayTracingPipelineStateDescriptor* GlobalRootSignature();
            RayTracingPipelineStateDescriptor* LocalRootSignature();
            RayTracingPipelineStateDescriptor* ConstantParameter();
            RayTracingPipelineStateDescriptor* NumConstants(uint32_t numConstants);
            RayTracingPipelineStateDescriptor* CbvParameter();
            RayTracingPipelineStateDescriptor* DescriptorTableParameter();
            RayTracingPipelineStateDescriptor* Range();
            RayTracingPipelineStateDescriptor* RangeType(RayTracingRootSignatureDescriptorTableParam::Range::Type rangeType);
            RayTracingPipelineStateDescriptor* RootParameterIndex(uint32_t rootParameterIndex);
            RayTracingPipelineStateDescriptor* ShaderRegister(uint32_t shaderRegister);
            RayTracingPipelineStateDescriptor* RegisterSpace(uint32_t registerSpace);
            RayTracingPipelineStateDescriptor* ShaderAssociation(const AZ::Name& shaderName);

            RayTracingPipelineStateDescriptor* StaticSampler();
            RayTracingPipelineStateDescriptor* FilterMode(RHI::FilterMode filterMode);
            RayTracingPipelineStateDescriptor* AddressMode(RHI::AddressMode addressMode);

        private:

            void ClearBuildContext();
            void ClearParamBuildContext();

            // build contexts
            RayTracingHitGroup* m_hitGroupBuildContext = nullptr;
            RayTracingRootSignature* m_rootSignatureBuildContext = nullptr;
            RayTracingLocalRootSignature* m_localRootSignatureBuildContext = nullptr;
            RayTracingRootSignatureCbvParam* m_rootSignatureCbvParamBuildContext = nullptr;
            RayTracingRootSignatureDescriptorTableParam* m_rootSignatureDescriptorTableParamBuildContext = nullptr;
            RayTracingRootSignatureDescriptorTableParam::Range* m_rootSignatureDescriptorTableParamRangeBuildContext = nullptr;
            RayTracingRootSignatureStaticSampler* m_rootSignatureStaticSamplerBuildContext = nullptr;

            // pipeline state elements
            RayTracingConfiguration m_configuration;
            RayTracingShaderLibraryVector m_shaderLibraries;
            RayTracingHitGroupVector m_hitGroups;
            RayTracingRootSignature m_globalRootSignature;
            RayTracingLocalRootSignatureVector m_localRootSignatures;
        };

        //! Defines the shaders, root signatures, hit groups, and other parameters required for
        //! ray tracing operations.
        class RayTracingPipelineState
            : public DeviceObject
        {
        public:
            RayTracingPipelineState() = default;
            virtual ~RayTracingPipelineState() = default;

            static RHI::Ptr<RHI::RayTracingPipelineState> CreateRHIRayTracingPipelineState();

            const RayTracingPipelineStateDescriptor& GetDescriptor() const { return m_descriptor; }
            ResultCode Init(Device& device, const RayTracingPipelineStateDescriptor* descriptor);

        private:

            // explicit shutdown is not allowed for this type
            void Shutdown() override final;

            //////////////////////////////////////////////////////////////////////////
            // Platform API
            virtual RHI::ResultCode InitInternal(RHI::Device& deviceBase, const RHI::RayTracingPipelineStateDescriptor* descriptor) = 0;
            virtual void ShutdownInternal() = 0;
            //////////////////////////////////////////////////////////////////////////

            RayTracingPipelineStateDescriptor m_descriptor;
        };
    }
}
