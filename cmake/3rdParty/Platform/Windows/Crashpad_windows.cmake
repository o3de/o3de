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

set(libpath ${BASE_PATH}/bin/windows/vs2019/$<IF:$<CONFIG:debug>,Debug,Release>_x64)

set(CRASHPAD_LIBS 
    ${libpath}/base.lib
    ${libpath}/crashpad_client.lib
    ${libpath}/crashpad_context.lib
    ${libpath}/crashpad_util.lib
    winhttp
    version
    powrprof
)

set(CRASHPAD_INCLUDE_DIRECTORIES
    include/compat/win
)

set(CRASHPAD_HANDLER_LIBS
    ${libpath}/crashpad_tool_support.lib
    ${libpath}/crashpad_compat.lib
    ${libpath}/third_party/getopt.lib
    ${libpath}/crashpad_minidump.lib
    ${libpath}/crashpad_snapshot.lib
    ${libpath}/crashpad_handler.lib
    ${libpath}/third_party/zlib.lib
)
