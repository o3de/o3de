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

set(WWISE_ANDROID_LIB_NAMES
    zip
)

set(WWISE_LIB_PATH ${BASE_PATH}/SDK/Android_arm64-v8a/$<IF:$<CONFIG:Debug>,Debug,$<IF:$<CONFIG:Profile>,Profile,Release>>/lib)

set(WWISE_LIBS)

foreach(nonReleaseLibName IN LISTS WWISE_NON_RELEASE_LIB_NAMES)
    list(APPEND WWISE_LIBS $<$<NOT:$<CONFIG:Release>>:${WWISE_LIB_PATH}/lib${nonReleaseLibName}.a>)
endforeach()

foreach(libName IN LISTS WWISE_COMMON_LIB_NAMES WWISE_CODEC_LIB_NAMES WWISE_ANDROID_LIB_NAMES WWISE_ADDITIONAL_LIB_NAMES)
    list(APPEND WWISE_LIBS ${WWISE_LIB_PATH}/lib${libName}.a)
endforeach()
