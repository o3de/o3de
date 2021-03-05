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

set(OPENSSL_LIBS
    ${BASE_PATH}/bin/linux-x86_64-clang-$<IF:$<CONFIG:Debug>,debug,release>/libcrypto.so
    ${BASE_PATH}/bin/linux-x86_64-clang-$<IF:$<CONFIG:Debug>,debug,release>/libssl.so
)

set(OPENSSL_RUNTIME_DEPENDENCIES
    ${BASE_PATH}/bin/linux-x86_64-clang-$<IF:$<CONFIG:Debug>,debug,release>/libcrypto.so
    ${BASE_PATH}/bin/linux-x86_64-clang-$<IF:$<CONFIG:Debug>,debug,release>/libssl.so
    ${BASE_PATH}/bin/linux-x86_64-clang-$<IF:$<CONFIG:Debug>,debug,release>/libcrypto.so.1.1
    ${BASE_PATH}/bin/linux-x86_64-clang-$<IF:$<CONFIG:Debug>,debug,release>/libssl.so.1.1
)
