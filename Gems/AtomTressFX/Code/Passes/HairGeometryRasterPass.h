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

namespace AZ
{
    namespace RHI
    {
        struct DrawItem;
    }

    namespace Render
    {
        namespace Hair
        {
            class HairRenderObject;
            class HairFeatureProcessor;

            //! A HairGeometryRasterPass is used for the render of the hair geometries.  This is the base
            //!  class that can be inherited - for example by the HiarPPLLRasterPass and have only the specific
            //!  class data handling added on top.
            class HairGeometryRasterPass
                : public RPI::RasterPass
                , private RPI::ShaderReloadNotificationBus::Handler
            {
                AZ_RPI_PASS(HairGeometryRasterPass);

            public:
                AZ_RTTI(HairGeometryRasterPass, "{0F07360A-A286-4060-8C62-137AFFA50561}", RasterPass);
                AZ_CLASS_ALLOCATOR(HairGeometryRasterPass, SystemAllocator, 0);

                //! Creates a HairGeometryRasterPass
                static RPI::Ptr<HairGeometryRasterPass> Create(const RPI::PassDescriptor& descriptor);

                bool AddDrawPackets(AZStd::list<Data::Instance<HairRenderObject>>& hairObjects);

                //! The following will be called when an object was added or shader has been compiled
                void SchedulePacketBuild(HairRenderObject* hairObject);

                Data::Instance<RPI::Shader> GetShader();

                void SetFeatureProcessor(HairFeatureProcessor* featureProcessor)
                {
                    m_featureProcessor = featureProcessor;
                }

                virtual bool IsEnabled() const override;

            protected:
                explicit HairGeometryRasterPass(const RPI::PassDescriptor& descriptor);

                // ShaderReloadNotificationBus::Handler overrides...
                void OnShaderReinitialized(const RPI::Shader& shader) override;
                void OnShaderAssetReinitialized(const Data::Asset<RPI::ShaderAsset>& shaderAsset) override;
                void OnShaderVariantReinitialized(const AZ::RPI::ShaderVariant& shaderVariant) override;

                void SetShaderPath(const char* shaderPath) { m_shaderPath = shaderPath; }
                bool LoadShaderAndPipelineState();
                bool AcquireFeatureProcessor();
                void BuildShaderAndRenderData();
                bool BuildDrawPacket(HairRenderObject* hairObject);

                // Pass behavior overrides
                void InitializeInternal() override;
                void FrameBeginInternal(FramePrepareParams params) override;

                // Scope producer functions...
                void CompileResources(const RHI::FrameGraphCompileContext& context) override;

            protected:
                HairFeatureProcessor* m_featureProcessor = nullptr;

                // The  shader that will be used by the pass
                Data::Instance<RPI::Shader> m_shader = nullptr;

                // Override the following in the inherited class
                AZStd::string m_shaderPath = "dummyShaderPath";

                // To help create the pipeline state 
                RPI::PassDescriptor m_passDescriptor;

                const RHI::PipelineState* m_pipelineState = nullptr;
                RPI::ViewPtr m_currentView = nullptr;

                AZStd::mutex m_mutex;

                //! List of new render objects introduced this frame so that their in order to identify 
                //! that their PerObject (dynamic) Srg needs binding to the resources.
                //! Done once per every new object introduced / requires update.
                AZStd::unordered_set<HairRenderObject*> m_newRenderObjects;

                bool m_initialized = false;
            };

        } // namespace Hair
    } // namespace Render
}   // namespace AZ
