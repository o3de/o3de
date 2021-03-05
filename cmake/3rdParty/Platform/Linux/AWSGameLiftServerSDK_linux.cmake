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

# TODO AWSGameLiftServerSDK Linux shared libs are not compiled.
set(AWSGAMELIFTSERVERSDK_LIB_PATH ${BASE_PATH}/lib/linux/libstdcxx/intel64/clang-6.0.0/$<IF:$<CONFIG:Debug>,Debug,Release>)

set(AWSGAMELIFTSERVERSDK_LIBS ${AWSGAMELIFTSERVERSDK_LIB_PATH}/libaws-cpp-sdk-gamelift-server.a
    ${AWSGAMELIFTSERVERSDK_LIB_PATH}/libsioclient.a
    ${AWSGAMELIFTSERVERSDK_LIB_PATH}/libboost_date_time.a
    ${AWSGAMELIFTSERVERSDK_LIB_PATH}/libboost_random.a
    ${AWSGAMELIFTSERVERSDK_LIB_PATH}/libboost_system.a
    ${AWSGAMELIFTSERVERSDK_LIB_PATH}/libprotobuf.a
)
