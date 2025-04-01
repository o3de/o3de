/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DeviceDrawItem.h>
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
            AZ_CLASS_ALLOCATOR(LightCullingPass, SystemAllocator);
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
            void ResetInternal()override;
            void BuildInternal() override;

            // Scope producer functions...
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            void SetLightBuffersToSRG();
            void SetLightsCountToSRG();
            void SetConstantdataToSRG();

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
                Data::Instance<RPI::Buffer>     m_lightBuffer;
                AZ::RHI::ShaderInputNameIndex   m_lightBufferIndex;
                AZ::RHI::ShaderInputNameIndex   m_lightCountIndex;
                int m_lightCount = 0;
            };

            enum LightTypes
            {
                eLightTypes_SimplePoint,
                eLightTypes_SimpleSpot,
                eLightTypes_Point,
                eLightTypes_Disk,
                eLightTypes_Capsule,
                eLightTypes_Quad,
                eLightTypes_Decal,
                eLightTypes_Count
            };

            AZStd::array<LightTypeData, eLightTypes_Count> m_lightdata;

            AZ::RHI::ShaderInputNameIndex m_constantDataIndex = "m_constantData";

            Data::Instance<RPI::Buffer> m_lightList;

            uint32_t m_tileDataIndex = std::numeric_limits<uint32_t>::max();
        };
    }   // namespace Render
}   // namespace AZ
