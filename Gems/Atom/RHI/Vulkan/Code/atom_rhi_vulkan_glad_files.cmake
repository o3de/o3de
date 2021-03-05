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

set(FILES
    Source/RHI.Loader/FunctionLoader.cpp
    Include/Atom/RHI.Loader/FunctionLoader.h
    Source/RHI.Loader/Glad/GladFunctionLoader.cpp
    Source/RHI.Loader/Glad/GladFunctionLoader.h
)

set(SKIP_UNITY_BUILD_INCLUSION_FILES
    # The following file defines GLAD_VULKAN_IMPLEMENTATION before including vulkan.h changing
    # the behavior inside vulkan.h. FunctionLoader.cpp also includes vulkan.h so this file cannot
    # be added to unity, other files could end up including vulkan.h and making this one fail.
    Source/RHI.Loader/Glad/GladFunctionLoader.cpp
)
