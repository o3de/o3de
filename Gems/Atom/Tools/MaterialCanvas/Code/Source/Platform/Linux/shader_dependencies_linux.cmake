#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# No in-memory shader compilation off Windows: it needs a ShaderPlatformInterface to drive, and the only one available here
# is DX12's, which is behind PAL_TRAIT_ATOM_RHI_DX12_SUPPORTED. The preview still builds through the Asset Processor.
set(LY_BUILD_DEPENDENCIES
)
