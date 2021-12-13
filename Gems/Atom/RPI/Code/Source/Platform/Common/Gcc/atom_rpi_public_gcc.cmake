#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_source_properties(
    SOURCES External/MaskedOcclusionCulling/MaskedOcclusionCullingAVX2.cpp
    PROPERTY COMPILE_OPTIONS
    VALUES -mavx2 -mfma -msse4.1
)

ly_add_source_properties(
    SOURCES External/MaskedOcclusionCulling/MaskedOcclusionCulling.cpp
    PROPERTY COMPILE_OPTIONS
    VALUES -mno-avx
)
