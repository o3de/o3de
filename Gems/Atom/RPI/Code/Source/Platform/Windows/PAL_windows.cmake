#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set (PAL_TRAIT_BUILD_ATOM_RPI_ASSETS_SUPPORTED TRUE)
set (PAL_TRAIT_BUILD_ATOM_RPI_MASKED_OCCLUSION_CULLING_SUPPORTED TRUE)

ly_add_source_properties(
    SOURCES External/MaskedOcclusionCulling/MaskedOcclusionCullingAVX2.cpp
    PROPERTY COMPILE_OPTIONS
    VALUES /arch:AVX2 /W3
)
ly_add_source_properties(
    SOURCES 
        External/MaskedOcclusionCulling/MaskedOcclusionCullingAVX512.cpp
        External/MaskedOcclusionCulling/MaskedOcclusionCulling.cpp
    PROPERTY COMPILE_OPTIONS
    VALUES /W3
)