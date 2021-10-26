/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

//#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/DrawPacketBuilder.h>
#include <Atom/RHI/PipelineState.h>

#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/Scene.h>

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Pass/RasterPassData.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <Passes/HairPPLLRasterPass.h>
#include <Rendering/HairRenderObject.h>
#include <Rendering/HairFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {

            HairPPLLRasterPass::HairPPLLRasterPass(const RPI::PassDescriptor& descriptor)
                : HairGeometryRasterPass(descriptor)
            {
                SetShaderPath("Shaders/hairrenderingfillppll.azshader");  
            }

            RPI::Ptr<HairPPLLRasterPass> HairPPLLRasterPass::Create(const RPI::PassDescriptor& descriptor)
            {
                RPI::Ptr<HairPPLLRasterPass> pass = aznew HairPPLLRasterPass(descriptor);
                return pass;
            }

            //! This method is used for attaching the PPLL data buffer which is a transient buffer.
            //! It is done this ways because Atom doesn't support transient structured buffers declaration
            //! via Pass yet.
            //! Once supported, this will be done via data driven code and the method can be removed.
            void HairPPLLRasterPass::BuildInternal()
            {
                RasterPass::BuildInternal();    // change this to call parent if the method exists

                if (!AcquireFeatureProcessor())
                {
                    return;
                }

                if (!LoadShaderAndPipelineState())
                {
                    return;
                }

                // Output
                AttachBufferToSlot(Name{ "PerPixelLinkedList" }, m_featureProcessor->GetPerPixelListBuffer());
            }

        } // namespace Hair
    }   // namespace Render
}   // namespace AZ
