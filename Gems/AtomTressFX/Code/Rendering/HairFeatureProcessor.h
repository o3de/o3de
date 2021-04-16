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

#include <AzCore/base.h>
#include <AzCore/std/containers/list.h>

#include <AtomCore/Instance/Instance.h>

#include <Atom/RPI.Public/FeatureProcessor.h>
//#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>

// Hair specific
//#include <TressFX/AMD_TressFX.h>
//#include <TressFX/AMD_Types.h>
#include <TressFX/TressFXConstantBuffers.h>

#include <Passes/HairSkinningComputePass.h>
#include <Passes/HairPPLLRasterPass.h>
#include <Passes/HairPPLLResolvePass.h>

#include <Rendering/HairRenderObject.h>
#include <Rendering/SharedBuffer.h>
#include <Rendering/HairCommon.h>

namespace AZ
{
    namespace RHI
    {
        struct ImageDescriptor;
    }

    namespace Render
    {
        namespace Hair
        {
            static const size_t PPLL_NODE_SIZE = 16;
            // Adi: replace the following with the actual screen dimensions.
            static const size_t AVE_FRAGS_PER_PIXEL = 12;
            static const size_t SCREEN_WIDTH = 1920;
            static const size_t SCREEN_HEIGHT = 1080;
            static const size_t RESERVED_PIXELS_FOR_OIT = SCREEN_WIDTH * SCREEN_HEIGHT * AVE_FRAGS_PER_PIXEL;

            class HairSkinningPass;
            class HairRenderObject;

            class HairFeatureProcessor final
                : public RPI::FeatureProcessor
            {
            public:
                AZ_RTTI(AZ::Render::HairFeatureProcessor, "{5F9DDA81-B43F-4E30-9E56-C7C3DC517A4C}", RPI::FeatureProcessor);
                AZ_FEATURE_PROCESSOR(HairFeatureProcessor);

                static void Reflect(AZ::ReflectContext* context);

                HairFeatureProcessor();
                virtual ~HairFeatureProcessor() = default;

                void UpdateHairSkinning();

                bool Init();

                // FeatureProcessor overrides ...
                void Activate() override;
                void Deactivate() override;
                void Simulate(const FeatureProcessor::SimulatePacket& packet) override;
                void Render(const FeatureProcessor::RenderPacket& packet) override;

                void AddHairRenderObject(Data::Instance<HairRenderObject> renderObject);
                bool RemoveHairRenderObject(Data::Instance<HairRenderObject> renderObject);

                // RPI::SceneNotificationBus overrides ...
                void OnRenderPipelineAdded(RPI::RenderPipelinePtr pipeline) override;
                void OnRenderPipelineRemoved(RPI::RenderPipeline* pipeline) override;
                void OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline) override;

                void OnBeginPrepareRender() override;
                void OnEndPrepareRender() override;

                Data::Instance<HairSkinningComputePass> GetHairSkinningComputegPass() { return m_hairSkinningComputePass; }
                Data::Instance<HairPPLLRasterPass> GetHairPPLLRasterPass() { return m_hairPPLLRasterPass; }
                Data::Instance<HairPPLLResolvePass> GetHairPPLLResolvePass() { return m_hairPPLLResolvePass; }

                //! Update the hair objects materials array.
                void CreateHairMaterialsArray(std::vector<const AMD::TressFXRenderParams*>& renderSettings);
                bool GetHairSkinningComputeShader();

                bool CreateAndBindPerPassSrg();

                Data::Instance<RPI::ShaderResourceGroup> GetPerPassSrg() { return m_perPassSrg; }
                Data::Instance<RPI::AttachmentImage> GetPerPixelHeadImage() { return m_linkedListPerPixelHead; }
                Data::Instance<RPI::Buffer> GetSharedBuffer() { return m_sharedDynamicBuffer->GetBuffer(); }
                Data::Instance<RPI::Buffer> GetPerPixelListBuffer() { return m_linkedListNodesBuffer; }
                Data::Instance<RPI::Buffer> GetPerPixelCounterBuffer() { return m_linkedListCounterBuffer;  }

                bool CreateAndSetPerPixelHeadImage(RHI::ImageDescriptor& imageDesc);

            private:
                AZ_DISABLE_COPY_MOVE(HairFeatureProcessor);

                bool InitSkinningPass();
                bool InitPPLLFillPass();
                bool InitPPLLResolvePass();

                static const char* s_featureProcessorName;

                //! The Hair Objects in the scene (one per hair component)
                AZStd::list<Data::Instance<HairRenderObject>> m_hairRenderObjects;

                //! Hair Passes - contain the shaders and shaders data within hence no need to hold it here
                //! The passes also contain the dispathItem for the compute, hence no need to hold this?
                Data::Instance<HairSkinningComputePass> m_hairSkinningComputePass = nullptr;
                Data::Instance<HairPPLLRasterPass> m_hairPPLLRasterPass = nullptr;
                Data::Instance<HairPPLLResolvePass> m_hairPPLLResolvePass = nullptr;

                Data::Instance<RPI::Shader> m_hairSkinningComputerShader = nullptr;

                //--------------------------------------------------------------
                //                  Per Pass Srg and its data
                //--------------------------------------------------------------
                //! Per pass srg shared by all passes and contains shared buffers such
                //!  as the hair vertices, PPLL data and hair objects material array.
                Data::Instance<RPI::ShaderResourceGroup> m_perPassSrg = nullptr;

                //! The shared buffer used by all dynamic buffer views for the hair skinning / simulation
                AZStd::unique_ptr<SharedBuffer> m_sharedDynamicBuffer;  // used for the hair data changed between passes.

                //! The constant buffer structure containing an array of all hair objects materials
                //!  to be used by the full screen resolve pass.
                HairUniformBuffer<AMD::TressFXShadeParams> m_hairObjectsMaterialsCB;

                //! Image pool used to create the linked list image.
 //               Data::Instance<RHI::ImagePool> m_imagePool;
                Data::Instance<RPI::AttachmentImagePool> m_pool = nullptr;
                //! Image containing the head pointers to each pixel's linked list
                Data::Instance<RPI::AttachmentImage> m_linkedListPerPixelHead = nullptr;
//                Data::Instance<RHI::Image> m_linkedListPerPixelHead;
// 
                //! PPLL single buffer containing all the PPLL elements
                Data::Instance<RPI::Buffer> m_linkedListNodesBuffer = nullptr;
                //! shared counter representing the next free index in the buffer
                //! 
                Data::Instance<RPI::Buffer> m_linkedListCounterBuffer = nullptr;
                //--------------------------------------------------------------

                bool m_passSrgCreated = false;
                bool m_initialized = false;
            };
        } // namespace Hair
    } // namespace Render
} // namespace AZ
