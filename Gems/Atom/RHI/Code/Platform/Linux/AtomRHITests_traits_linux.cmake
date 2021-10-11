#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(ATOM_RHI_TRAIT_BUILD_SUPPORTS_TEST FALSE)
set(ATOM_RHI_TRAIT_BUILD_SUPPORTS_EDIT TRUE)
if(LY_MONOLITHIC_GAME)
    set(PAL_TRAIT_BUILD_RENDERDOC_SUPPORTED FALSE)
else()
    set(PAL_TRAIT_BUILD_RENDERDOC_SUPPORTED TRUE)
endif()
set(LY_RENDERDOC_PATH "/usr/local" CACHE PATH "Path to RenderDoc.")
