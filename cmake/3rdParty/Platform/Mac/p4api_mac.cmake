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

set(p4api_lib_folder ${BASE_PATH}/macos)

set(P4API_LIBS 
    ${p4api_lib_folder}/libclient.a
    ${p4api_lib_folder}/librpc.a
    ${p4api_lib_folder}/libsupp.a
    ${p4api_lib_folder}/libp4script.a
    ${p4api_lib_folder}/libp4script_c.a
    ${p4api_lib_folder}/libp4script_curl.a
    ${p4api_lib_folder}/libp4script_sqlite.a
)