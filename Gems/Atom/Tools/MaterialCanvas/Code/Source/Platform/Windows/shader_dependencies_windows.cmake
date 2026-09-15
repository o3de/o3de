#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# DX12 ShaderPlatformInterface for the in-memory preview shader path; Windows only, matching the MaterialCanvas traits.
set(LY_BUILD_DEPENDENCIES
    PRIVATE
        Gem::Atom_RHI_DX12.Builders.Static
)
