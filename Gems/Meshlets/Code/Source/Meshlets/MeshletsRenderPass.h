/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI.Reflect/Size.h>

#include <Atom/RPI.Public/Pass/RasterPass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>

#include <Atom/Feature/TransformService/TransformServiceFeatureProcessor.h>

#include <MeshletsRenderObject.h>

namespace AZ
{
    namespace RHI
    {
        struct DrawItem;
        class DrawPacket;
    }

    namespace Meshlets
    {
        class MeshletsRenderObject;
        class MeshletsFeatureProcessor;

        class MeshletsRenderPass
            : public RPI::RasterPass
            , private RPI::ShaderReloadNotificationBus::Handler
        {
            AZ_RPI_PASS(MeshletsRenderPass);

        public:
            AZ_RTTI(MeshletsRenderPass, "{753E455B-8E36-4DC3-B315-789F0EF0483C}", RasterPass);
            AZ_CLASS_ALLOCATOR(MeshletsRenderPass, SystemAllocator);

            static RPI::Ptr<MeshletsRenderPass> Create(const RPI::PassDescriptor& descriptor);

            // Adds the lod array of render data
            bool FillDrawRequestData(RHI::DeviceDrawPacketBuilder::DeviceDrawRequest& drawRequest);
            bool AddDrawPackets(AZStd::list<const RHI::DeviceDrawPacket*> drawPackets);

            Data::Instance<RPI::Shader> GetShader();

            void SetFeatureProcessor(MeshletsFeatureProcessor* featureProcessor)
            {
                m_featureProcessor = featureProcessor;
            }

        protected:
            explicit MeshletsRenderPass(const RPI::PassDescriptor& descriptor);

            // ShaderReloadNotificationBus::Handler overrides...
            void OnShaderReinitialized(const RPI::Shader& shader) override;
            void OnShaderAssetReinitialized(const Data::Asset<RPI::ShaderAsset>& shaderAsset) override;
            void OnShaderVariantReinitialized(const AZ::RPI::ShaderVariant& shaderVariant) override;

            void SetShaderPath(const char* shaderPath) { m_shaderPath = shaderPath; }
            bool LoadShader();
            bool InitializePipelineState();
            bool AcquireFeatureProcessor();
            void BuildShaderAndRenderData();

            // Pass behavior overrides
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

            // Scope producer functions...
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;

        protected:
            MeshletsFeatureProcessor* m_featureProcessor = nullptr;

            // The  shader that will be used by the pass
            Data::Instance<RPI::Shader> m_shader = nullptr;

            // Override the following in the inherited class
            AZStd::string m_shaderPath = "dummyShaderPath";

            // To help create the pipeline state 
            RPI::PassDescriptor m_passDescriptor;

            const RHI::PipelineState* m_pipelineState = nullptr;
            RPI::ViewPtr m_currentView = nullptr;
        };

    } // namespace Meshlets
} // namespace AZ
