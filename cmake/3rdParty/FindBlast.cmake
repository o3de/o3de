#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

ly_add_external_target(
    NAME Blast
    VERSION 1.1.6-az.1
    INCLUDE_DIRECTORIES
        sdk/common
        sdk/extensions/assetutils/include
        sdk/extensions/authoring/include
        sdk/extensions/exporter/include
        sdk/extensions/physx/include
        sdk/extensions/serialization/include
        sdk/extensions/shaders/include
        sdk/extensions/stress/include
        sdk/globals/include
        sdk/lowlevel/include
        sdk/toolkit/include
    COMPILE_DEFINITIONS
        $<$<BOOL:${LY_MONOLITHIC_GAME}>:BLAST_STATIC_LIB>
)
