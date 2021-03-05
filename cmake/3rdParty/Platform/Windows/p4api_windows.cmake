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

set(p4api_lib_folder $<IF:$<CONFIG:debug>,${BASE_PATH}/vs14/debug,${BASE_PATH}/vs14/release>)

set(P4API_LIBS 
    ${p4api_lib_folder}/libclient.lib
    ${p4api_lib_folder}/librpc.lib
    ${p4api_lib_folder}/libsupp.lib
    ${p4api_lib_folder}/libp4script.lib
    ${p4api_lib_folder}/libp4script_c.lib
    ${p4api_lib_folder}/libp4script_curl.lib
    ${p4api_lib_folder}/libp4script_sqlite.lib
)