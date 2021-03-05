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

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DrawItem.h>
#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace Render
    {
        //! Compute shader that performs light culling
        class LightCullingPass final
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(LightCullingPass);

        public:
            AZ_RTTI(AZ::Render::LightCullingPass, "{F99EB06A-052E-4FAA-B2C4-4247BAD9BDC7}", RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(LightCullingPass, SystemAllocator, 0);
            virtual ~LightCullingPass() = default;

            /// Creates a LightCullingPass
            static RPI::Ptr<LightCullingPass> Create(const RPI::PassDescriptor& descriptor);

            static Name GetLightCullingPassTemplateName()
            {
                return Name("LightCullingTemplate");
            }

        private:

            LightCullingPass(const RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            virtual void ResetInternal()override;
            virtual void BuildAttachmentsInternal() override;

            // Scope producer functions...
            virtual void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph, const RPI::PassScopeProducer& producer) override;
            virtual void CompileResources(const RHI::FrameGraphCompileContext& context, const RPI::PassScopeProducer& producer) override;
            virtual void BuildCommandList(const RHI::FrameGraphExecuteContext& context, const RPI::PassScopeProducer& producer) override;

            void Init();

            void CacheSrgIndices();
            void CacheLightCounts();
            void CacheConstantData();
            void CacheLightBuffer();

            void SetLightBuffersToSRG();
            void SetLightsCountToSRG();
            void SetConstantdataToSRG();
            void SetLightListToSRG();

            AZ::RHI::Size GetDepthBufferResolution();
            float CreateTraceValues(const AZ::Vector2& unprojection);
            void GetLightDataFromFeatureProcessor();

            uint32_t FindInputBinding(const AZ::Name& name);

            // Used for conversion from z-buffer values to view space depth
            AZStd::array<float, 2> ComputeGridPixelSize();
            void CreateLightList();
            void AttachLightList();

            AZ::RHI::Size GetTileDataBufferResolution();
            struct LightTypeData
            {
                AZ::Name m_lightCountNameInShader;
                AZ::Name lightBufferNameInShader;

                Data::Instance<RPI::Buffer>         m_lightBuffer;
                AZ::RHI::ShaderInputBufferIndex     m_lightBufferIndex;
                AZ::RHI::ShaderInputConstantIndex   m_lightCountIndex;
                int m_lightCount = 0;
            };

            enum LightTypes
            {
                eLightTypes_Point,
                eLightTypes_Spot,
                eLightTypes_Disk,
                eLightTypes_Capsule,
                eLightTypes_Quad,
                eLightTypes_Decal,
                eLightTypes_Count
            };

            AZStd::array<LightTypeData, eLightTypes_Count> m_lightdata;

            AZ::RHI::ShaderInputConstantIndex   m_constantDataIndex;

            bool m_initialized = false;
            Data::Instance<RPI::Buffer> m_lightList;

            uint32_t m_tileDataIndex = -1;
        };
    }   // namespace Render
}   // namespace AZ
