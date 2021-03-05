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

set( NVCLOTH_COMPILEDEFINITIONS NV_CLOTH_IMPORT= PX_CALL_CONV= )

ly_add_external_target(
    NAME nvcloth
    VERSION 1.1.6-az.4
    3RDPARTY_DIRECTORY "NvCloth"
    INCLUDE_DIRECTORIES
        NvCloth/include
        NvCloth/extensions/include
        PxShared/include
    COMPILE_DEFINITIONS ${NVCLOTH_COMPILEDEFINITIONS}
)
