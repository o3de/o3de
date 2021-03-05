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

set(OPENMESH_LIBS
    OpenMeshCore$<$<CONFIG:Debug>:d>
    OpenMeshTools$<$<CONFIG:Debug>:d>
)

# static
set(PATH_TO_LIBS ${BASE_PATH}/lib/$<IF:$<CONFIG:debug>,debug,release>/)

list(TRANSFORM OPENMESH_LIBS PREPEND ${PATH_TO_LIBS})
list(TRANSFORM OPENMESH_LIBS APPEND "${CMAKE_STATIC_LIBRARY_SUFFIX}")
