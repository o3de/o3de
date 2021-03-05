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

set(JSMN_NAME jsmn)

if(NOT TARGET 3rdParty::${JSMN_NAME})

    set(JSMN_VERSION 78b1dca)
    set(BASE_PATH "${LY_3RDPARTY_PATH}/${JSMN_NAME}/${JSMN_VERSION}")

    add_library(3rdParty::${JSMN_NAME} 
        STATIC IMPORTED
        jsmn.c
    )

    ly_target_include_system_directories(TARGET 3rdParty::${JSMN_NAME}
        PUBLIC ${BASE_PATH}
    )

endif()
