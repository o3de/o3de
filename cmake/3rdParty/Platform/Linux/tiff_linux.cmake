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

# In some compilers, the build dependency has to be added after in linking. Because we represent 3rdParty with
# interface libraries, dependencies are transitive and do not represent dependencies between the 3rdParties themselves.
# Refer to the comment here: https://gitlab.kitware.com/cmake/cmake/-/blob/master/Source/cmComputeLinkDepends.cxx#L29
# To workaround this problem, we wrap the static lib in an imported lib and mark the dependency there. That makes
# the DAG algorithm to sort them in the order we need.

add_library(3rdParty::tiff::imported STATIC IMPORTED)
set_target_properties(3rdParty::tiff::imported
    PROPERTIES
        IMPORTED_LOCATION ${BASE_PATH}/libtiff/linux_gcc/libtiff.a
)
target_link_libraries(3rdParty::tiff::imported
  INTERFACE 
    3rdParty::zlib
)
set(TIFF_BUILD_DEPENDENCIES 
    3rdParty::tiff::imported
    jpeg 
    jbig
)