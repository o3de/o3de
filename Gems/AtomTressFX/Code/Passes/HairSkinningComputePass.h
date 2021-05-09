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

#include <Atom/RHI.Reflect/Size.h>

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace RHI
    {
        struct DispatchItem;
    }

    namespace Render
    {
        namespace Hair
        {
            class HairDispatchItem;
            class HairFeatureProcessor;
            class HairRenderObject;
            enum class DispatchLevel;

            // This pass is a temporary test pass that using a compute shader generates the bone
            // skinning per vertex required for the hair to work without needing to have the complete
            // physics multi-pass.
            //! The Skinning Compute pass is using the following Srgs indirectly via the dispatchItem:
            //!  - PerPassSrg: shared by all hair passes for the shared dynamic buffer and the PPLL buffers
            //!  - HairGenerationSrg: dictates how to construct the hair vertices and skinning
            //!  - HairSimSrg: defines vertices and tangent data shared between all passes 
            // Reference Passes: EyeAdaptationPass and SkinnedMeshComputePass
            class HairSkinningComputePass final
                : public RPI::ComputePass
            {
                AZ_RPI_PASS(HairSkinningComputePass);

            public:
                AZ_RTTI(HairSkinningComputePass, "{DC8D323E-41FF-4FED-89C6-A254FD6809FC}", RPI::ComputePass);
                AZ_CLASS_ALLOCATOR(HairSkinningComputePass, SystemAllocator, 0);
                ~HairSkinningComputePass() = default;

                // Creates a HairSkinningComputePass
                static RPI::Ptr<HairSkinningComputePass> Create(const RPI::PassDescriptor& descriptor);

                bool BuildDispatchItem(HairRenderObject* hairObject, DispatchLevel dispatchLevel );

                //! Thread-safe function for adding a dispatch item to the current frame.
                void AddDispatchItem(HairRenderObject* hairObject);

                // Pass behavior overrides
                void CompileResources(const RHI::FrameGraphCompileContext& context) override;
                void OnBuildAttachmentsFinishedInternal() override;

                virtual bool IsEnabled() const override;
                //! returns the shader held by the ComputePass
                //! [To Do] : expose this in the ComputePass
                Data::Instance<RPI::Shader> GetShader();

                void SetFeatureProcessor(HairFeatureProcessor* featureProcessor)
                {
                    m_featureProcessor = featureProcessor;
                }

            protected:
                HairSkinningComputePass(const RPI::PassDescriptor& descriptor);

                void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

                // Attach here all the pass buffers
                void BuildAttachmentsInternal() override;

                void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;

                // ShaderReloadNotificationBus::Handler overrides...
                void OnShaderReinitialized(const AZ::RPI::Shader& shader) override;
                void OnShaderAssetReinitialized(const Data::Asset<AZ::RPI::ShaderAsset>& shaderAsset) override;
                void OnShaderVariantReinitialized(const AZ::RPI::Shader& shader, const AZ::RPI::ShaderVariantId& shaderVariantId, AZ::RPI::ShaderVariantStableId shaderVariantStableId) override;

            private:
                HairFeatureProcessor* m_featureProcessor = nullptr;
                RHI::Size  m_attachmentSize = RHI::Size(1,1,0);    // for buffer set.

                AZStd::mutex m_mutex;

                //! list of dispatch items, each represents a single hair object that
                //!  will be used by the skinning compute shader.
                AZStd::unordered_set<const RHI::DispatchItem*> m_dispatchItems;
            };

        }   // namespace Hair
    }   // namespace Render
}   // namespace AZ

