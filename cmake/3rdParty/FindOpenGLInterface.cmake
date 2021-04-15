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

find_package(OpenGL QUIET REQUIRED)
# Imported targets (like OpenGL::GL) are scoped to a directory. Add a
# a global scope
add_library(3rdParty::OpenGLInterface INTERFACE IMPORTED GLOBAL)
target_link_libraries(3rdParty::OpenGLInterface INTERFACE OpenGL::GL)

set(pal_file ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}/OpenGLInterface_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)
if(EXISTS ${pal_file})
    include(${pal_file})
endif()