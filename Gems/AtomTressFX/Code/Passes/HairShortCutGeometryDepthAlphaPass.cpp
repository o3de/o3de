/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Pass/RasterPassData.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <Passes/HairShortCutGeometryDepthAlphaPass.h>
#include <Rendering/HairRenderObject.h>
#include <Rendering/HairFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {

            HairShortCutGeometryDepthAlphaPass::HairShortCutGeometryDepthAlphaPass(const RPI::PassDescriptor& descriptor)
                : HairGeometryRasterPass(descriptor)
            {
                SetShaderPath("Shaders/hairshortcutgeometrydepthalpha.azshader");  
            }

            RPI::Ptr<HairShortCutGeometryDepthAlphaPass> HairShortCutGeometryDepthAlphaPass::Create(const RPI::PassDescriptor& descriptor)
            {
                RPI::Ptr<HairShortCutGeometryDepthAlphaPass> pass = aznew HairShortCutGeometryDepthAlphaPass(descriptor);
                return pass;
            }

            void HairShortCutGeometryDepthAlphaPass::BuildInternal()
            {
                RasterPass::BuildInternal();    // change this to call parent if the method exists

                if (!AcquireFeatureProcessor())
                {
                    return;
                }

                LoadShaderAndPipelineState();
            }

        } // namespace Hair
    }   // namespace Render
}   // namespace AZ
