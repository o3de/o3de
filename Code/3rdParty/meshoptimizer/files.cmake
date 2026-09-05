#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

set(FILES
    "${meshoptimizer_SOURCE_DIR}/src/meshoptimizer.h"
    "${meshoptimizer_SOURCE_DIR}/src/allocator.cpp"
    "${meshoptimizer_SOURCE_DIR}/src/clusterizer.cpp"
    "${meshoptimizer_SOURCE_DIR}/src/indexanalyzer.cpp"
    "${meshoptimizer_SOURCE_DIR}/src/indexcodec.cpp"
    "${meshoptimizer_SOURCE_DIR}/src/indexgenerator.cpp"
    "${meshoptimizer_SOURCE_DIR}/src/meshletcodec.cpp"
    "${meshoptimizer_SOURCE_DIR}/src/meshletutils.cpp"
    "${meshoptimizer_SOURCE_DIR}/src/opacitymap.cpp"
    "${meshoptimizer_SOURCE_DIR}/src/overdrawoptimizer.cpp"
    "${meshoptimizer_SOURCE_DIR}/src/partition.cpp"
    "${meshoptimizer_SOURCE_DIR}/src/quantization.cpp"
    "${meshoptimizer_SOURCE_DIR}/src/rasterizer.cpp"
    "${meshoptimizer_SOURCE_DIR}/src/simplifier.cpp"
    "${meshoptimizer_SOURCE_DIR}/src/spatialorder.cpp"
    "${meshoptimizer_SOURCE_DIR}/src/stripifier.cpp"
    "${meshoptimizer_SOURCE_DIR}/src/tangentspace.cpp"
    "${meshoptimizer_SOURCE_DIR}/src/vcacheoptimizer.cpp"
    "${meshoptimizer_SOURCE_DIR}/src/vertexcodec.cpp"
    "${meshoptimizer_SOURCE_DIR}/src/vertexfilter.cpp"
    "${meshoptimizer_SOURCE_DIR}/src/vfetchoptimizer.cpp"
)
