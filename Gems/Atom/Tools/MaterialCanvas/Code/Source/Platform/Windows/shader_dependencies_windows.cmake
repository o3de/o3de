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
# Windows only, and deliberately not abstracted: the whole DX12 gem sits behind PAL_TRAIT_ATOM_RHI_DX12_SUPPORTED and does
# not exist to link against elsewhere. The code behind this is guarded with AZ_PLATFORM_WINDOWS to match, and the in-memory
# path reports itself unavailable on other platforms, where the Asset Processor route still works.
set(LY_BUILD_DEPENDENCIES
    PRIVATE
        Gem::Atom_RHI_DX12.Builders.Static
)
