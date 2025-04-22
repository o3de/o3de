/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>

namespace AZ
{
    namespace Render
    {
        namespace StencilRefs
        {
            const uint32_t None = 0x00;

            // UseIBLSpecularPass
            //
            // The MeshFeatureProcessor sets the UseIBLSpecularPass stencil value on any geometry that should receive IBL Specular
            // in the Reflections pass, otherwise IBL specular is rendered in the Forward pass.  The Reflections pass only renders
            // to areas with these stencil bits set.
            //
            // Used in pass range: Forward -> Reflections
            // 
            // Notes:
            // - Two bits are needed here (0x3) so that the ReflectionProbeStencilPass can use "Less" on its stencil test to
            //   properly handle the DecrSat on the FrontFace stencil operation depth-fail.
            // - The ReflectionProbeStencilPass pass may overwrite other bits in the stencil buffer, depending on the amount of
            //   reflection probe volume nesting in the content.
            // - New stencil bits for other purposes should be added to the most signficant bits and masked out of the Reflection passes.
            //   This is necessary to allow the most amount of bits to be used by the ReflectionProbeStencilPass for nested probe volumes.
            // - The Reflection passes currently use 0x3F for the ReadMask and WriteMask to exclude the stencil bits below.
            //   If other stencil bits are added then these masks will need to be updated.
            const uint32_t UseIBLSpecularPass = 0x3;

            // BlockSilhouettes
            // 
            // The MeshFeatureProcessor sets this stencil bit on any geometry that should block silhouettes in the SilhouetteGather pass. 
            //
            // Used in pass range: Forward -> Silhouette
            // This setting needs to match the Stencil ReadMask in SilhouetteGather.shader
            const uint32_t BlockSilhouettes = 0x40;

            // UseDiffuseGIPass
            // 
            // The MeshFeatureProcessor sets this stencil bit on any geometry that should receive Diffuse GI in the DiffuseGlobalIllumination pass.
            //
            // Used in pass range: Forward -> DiffuseGlobalIllumination
            const uint32_t UseDiffuseGIPass = 0x80;
        }

        namespace Culling
        {
            // Component types used in the RPI CullData
            enum ComponentType : uint32_t
            {
                Unknown,
                ReflectionProbe
            };
        }
    }
}
