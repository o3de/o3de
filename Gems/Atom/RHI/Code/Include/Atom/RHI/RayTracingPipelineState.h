/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

            AZ::Name m_rayGenerationShaderName;
            AZ::Name m_missShaderName;
            AZ::Name m_closestHitShaderName;
            AZ::Name m_anyHitShaderName;
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

        //! RayTracingPipelineStateDescriptor
        //!
        //! The Build() operation in the descriptor allows the pipeline state to be initialized
        //! using the following pattern:
        //!
        //! RHI::RayTracingPipelineStateDescriptor descriptor;
        //! descriptor.Build()
        //!     ->ShaderLibrary(shaderDescriptor)
        //!         ->RayGenerationShaderName(AZ::Name("RayGenerationShader"))
        //!     ->ShaderLibrary(missShaderDescriptor)
        //!         ->MissShaderName(AZ::Name("MissShader"))
        //!     ->ShaderLibrary(closestHitShader1Descriptor)
        //!         ->ClosestHitShaderName(AZ::Name("ClosestHitShader1"))
        //!     ->ShaderLibrary(closestHitShader2Descriptor)
        //!         ->ClosestHitShaderName(AZ::Name("ClosestHitShader2"))
        //!     ->HitGroup(AZ::Name("HitGroup1"))
        //!         ->ClosestHitShaderName(AZ::Name("ClosestHitShader1"))
        //!     ->HitGroup(AZ::Name("HitGroup2"))
        //!         ->ClosestHitShaderName(AZ::Name("ClosestHitShader2"))
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

            const RHI::PipelineState* GetPipelineState() const { return m_pipelineState; }

            const RayTracingShaderLibraryVector& GetShaderLibraries() const { return m_shaderLibraries; }
            RayTracingShaderLibraryVector& GetShaderLibraries() { return m_shaderLibraries; }

            const RayTracingHitGroupVector& GetHitGroups() const { return m_hitGroups; }
            RayTracingHitGroupVector& GetHitGroups() { return m_hitGroups; }

            // build operations
            RayTracingPipelineStateDescriptor* Build();
            RayTracingPipelineStateDescriptor* MaxPayloadSize(uint32_t maxPayloadSize);
            RayTracingPipelineStateDescriptor* MaxAttributeSize(uint32_t maxAttributeSize);
            RayTracingPipelineStateDescriptor* MaxRecursionDepth(uint32_t maxRecursionDepth);
            RayTracingPipelineStateDescriptor* PipelineState(const RHI::PipelineState* pipelineState);
            RayTracingPipelineStateDescriptor* ShaderLibrary(RHI::PipelineStateDescriptorForRayTracing& descriptor);

            RayTracingPipelineStateDescriptor* RayGenerationShaderName(const AZ::Name& name);
            RayTracingPipelineStateDescriptor* MissShaderName(const AZ::Name& name);
            RayTracingPipelineStateDescriptor* ClosestHitShaderName(const AZ::Name& closestHitShaderName);
            RayTracingPipelineStateDescriptor* AnyHitShaderName(const AZ::Name& anyHitShaderName);

            RayTracingPipelineStateDescriptor* HitGroup(const AZ::Name& name);

        private:

            void ClearBuildContext();
            void ClearParamBuildContext();
            bool IsTopLevelBuildContext();

            // build contexts
            RayTracingShaderLibrary* m_shaderLibraryBuildContext = nullptr;
            RayTracingHitGroup* m_hitGroupBuildContext = nullptr;

            // pipeline state elements
            RayTracingConfiguration m_configuration;
            const RHI::PipelineState* m_pipelineState = nullptr;
            RayTracingShaderLibraryVector m_shaderLibraries;
            RayTracingHitGroupVector m_hitGroups;
        };

        //! Defines the shaders, hit groups, and other parameters required for ray tracing operations.
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
