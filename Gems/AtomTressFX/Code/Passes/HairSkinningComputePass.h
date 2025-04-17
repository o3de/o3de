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
#include <Atom/RHI/DispatchItem.h>

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace RHI
    {
        class DispatchItem;
    }

    namespace Render
    {
        namespace Hair
        {
            class HairDispatchItem;
            class HairFeatureProcessor;
            class HairRenderObject;
            enum class DispatchLevel;

            // This pass class serves for all skinning and simulation hair compute passes.
            //! The Skinning Compute passes are all using the following Srgs via the dispatchItem:
            //!  - PerPassSrg: shared by all hair passes for the shared dynamic buffer and the PPLL buffers
            //!  - HairGenerationSrg: dictates how to construct the hair vertices and skinning
            //!  - HairSimSrg: defines vertices and tangent data shared between all passes 
            class HairSkinningComputePass final
                : public RPI::ComputePass
            {
                AZ_RPI_PASS(HairSkinningComputePass);

            public:
                AZ_RTTI(HairSkinningComputePass, "{DC8D323E-41FF-4FED-89C6-A254FD6809FC}", RPI::ComputePass);
                AZ_CLASS_ALLOCATOR(HairSkinningComputePass, SystemAllocator);
                ~HairSkinningComputePass() = default;

                // Creates a HairSkinningComputePass
                static RPI::Ptr<HairSkinningComputePass> Create(const RPI::PassDescriptor& descriptor);

                bool BuildDispatchItem(HairRenderObject* hairObject, DispatchLevel dispatchLevel );

                //! Thread-safe function for adding the frame's dispatch items
                void AddDispatchItems(AZStd::list<Data::Instance<HairRenderObject>>& renderObjects);

                // Pass behavior overrides
                void CompileResources(const RHI::FrameGraphCompileContext& context) override;

                virtual bool IsEnabled() const override;

                //! Returns the shader held by the ComputePass
                Data::Instance<RPI::Shader> GetShader();

                void SetFeatureProcessor(HairFeatureProcessor* featureProcessor)
                {
                    m_featureProcessor = featureProcessor;
                }
                void SetAllowIterations(bool allowIterations)
                {
                    m_allowSimIterations = allowIterations;
                }

            protected:
                HairSkinningComputePass(const RPI::PassDescriptor& descriptor);

                void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
                void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

                // Attach here all the pass buffers
                void InitializeInternal() override;
                void BuildInternal() override;
                void FrameBeginInternal(FramePrepareParams params) override;

                // ShaderReloadNotificationBus::Handler overrides...
                void OnShaderReinitialized(const AZ::RPI::Shader& shader) override;
                void OnShaderAssetReinitialized(const Data::Asset<AZ::RPI::ShaderAsset>& shaderAsset) override;
                void OnShaderVariantReinitialized(const AZ::RPI::ShaderVariant& shaderVariant) override;

                bool AcquireFeatureProcessor();
                void BuildShaderAndRenderData();

            private:
                HairFeatureProcessor* m_featureProcessor = nullptr;

                bool m_allowSimIterations = false;
                bool m_initialized = false;
                bool m_buildShaderAndData = false;  // If shader is updated, mark it for build

                AZStd::mutex m_mutex;

                //! list of dispatch items, each represents a single hair object that
                //!  will be used by the skinning compute shader.
                AZStd::unordered_set<const RHI::DispatchItem*> m_dispatchItems;

                //! List of new render objects that their Per Object (dynamic) Srg should be bound
                //!  to the resources.  Done once per pass per object only.
                AZStd::unordered_set<HairRenderObject*> m_newRenderObjects;
            };

        }   // namespace Hair
    }   // namespace Render
}   // namespace AZ

