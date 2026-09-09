#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# The DX12 ShaderPlatformInterface, so the in-memory preview shader path can run DXC and build a ShaderStageFunction in
# process rather than waiting for the Asset Processor to do it. It is the same class the Shader Asset Builder drives, and
# its constructor is public, so nothing is reimplemented here.
#
# Windows only: the whole DX12 gem sits behind PAL_TRAIT_ATOM_RHI_DX12_SUPPORTED and does not exist to link against
# elsewhere. The code behind this is guarded with AZ_TRAIT_MATERIALCANVAS_IN_MEMORY_SHADER_COMPILATION_SUPPORTED, which
# MaterialCanvas_Traits_Windows.h sets to 1 to match, and the in-memory path reports itself unavailable on the platforms
# where that trait is 0, and where the Asset Processor route still works.
set(LY_BUILD_DEPENDENCIES
    PRIVATE
        Gem::Atom_RHI_DX12.Builders.Static
)
