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

if (LY_MONOLITHIC_GAME)
    # Import Libs
    set(AWSGAMELIFTSERVERSDK_LIB_PATH ${BASE_PATH}/lib/windows/intel64/vs2017/$<IF:$<CONFIG:Debug>,Debug,Release>)
else()
    # Static Libs
    set(AWSGAMELIFTSERVERSDK_LIB_PATH ${BASE_PATH}/bin/windows/intel64/vs2017/$<IF:$<CONFIG:Debug>,Debug,Release>)
endif()

set(AWSGAMELIFTSERVERSDK_LIBS
    ${AWSGAMELIFTSERVERSDK_LIB_PATH}/sioclient.lib
    ${AWSGAMELIFTSERVERSDK_LIB_PATH}/libboost_date_time.lib
    ${AWSGAMELIFTSERVERSDK_LIB_PATH}/libboost_random.lib
    ${AWSGAMELIFTSERVERSDK_LIB_PATH}/libboost_system.lib
    ${AWSGAMELIFTSERVERSDK_LIB_PATH}/libprotobuf$<$<CONFIG:Debug>:d>.lib
)

set(AWSGAMELIFTSERVERSDK_COMPILE_DEFINITIONS
    USE_IMPORT_EXPORT
    AWS_CUSTOM_MEMORY_MANAGEMENT
    PLATFORM_SUPPORTS_AWS_NATIVE_SDK
    GAMELIFT_USE_STD
)

if (NOT LY_MONOLITHIC_GAME)

    # Add 'USE_IMPORT_EXPORT' for external linkage
    LIST(APPEND AWSGAMELIFTSERVERSDK_COMPILE_DEFINITIONS USE_IMPORT_EXPORT)

    # Import Lib
    LIST(APPEND AWSGAMELIFTSERVERSDK_LIBS ${AWSGAMELIFTSERVERSDK_LIB_PATH}/aws-cpp-sdk-gamelift-server.lib)

    # Shared libs
    set(AWSGAMELIFTSERVERSDK_SHARED_LIB_PATH ${BASE_PATH}/bin/windows/intel64/vs2017/$<IF:$<CONFIG:Debug>,Debug,Release>)
    set(AWSGAMELIFTSERVERSDK_RUNTIME_DEPENDENCIES ${AWSGAMELIFTSERVERSDK_SHARED_LIB_PATH}/aws-cpp-sdk-gamelift-server.dll)

endif()