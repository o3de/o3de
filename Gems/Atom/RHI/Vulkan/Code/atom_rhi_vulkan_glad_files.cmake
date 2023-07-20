#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Source/RHI.Loader/LoaderContext.cpp
    Include/Atom/RHI.Loader/LoaderContext.h
    Include/Atom/RHI.Loader/Glad/vulkan/vulkan.h
)

set(SKIP_UNITY_BUILD_INCLUSION_FILES
    # The following file defines GLAD_VULKAN_IMPLEMENTATION before including vulkan.h changing
    # the behavior inside vulkan.h. LoaderContext.cpp also includes vulkan.h so this file cannot
    # be added to unity, other files could end up including vulkan.h and making this one fail.
    Source/RHI.Loader/LoaderContext.cpp
)
