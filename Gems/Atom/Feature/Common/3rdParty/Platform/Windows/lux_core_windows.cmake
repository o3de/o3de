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

set(LUX_CORE_LIBS ${BASE_PATH}/win64/lib/luxcore.lib)

set(LUX_CORE_RUNTIME_DEPENDENCIES 
    ${BASE_PATH}/win64/dll/embree3.dll
    ${BASE_PATH}/win64/dll/luxcore.dll
    ${BASE_PATH}/win64/dll/OpenImageDenoise.dll
    #${BASE_PATH}/win64/dll/OpenImageIO.dll ### ATOM-5988 This dll is 1.8 and currently conflicts with Windows version of full SDK of OpenImageIO 2.1
    ${BASE_PATH}/win64/dll/tbb.dll
    ${BASE_PATH}/win64/dll/tbbmalloc.dll
)
