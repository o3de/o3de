#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(TARGET_WITH_NAMESPACE "Onnx")
if (TARGET ${TARGET_WITH_NAMESPACE})
    return()
endif()

set(Onnx_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/onnxruntime/include)
set(Onnx_LIB ${CMAKE_CURRENT_LIST_DIR}/onnxruntime/lib/${ONNXRUNTIME_DEPENDENCY})

add_library(${TARGET_WITH_NAMESPACE} STATIC IMPORTED GLOBAL)

set(Onnx_FOUND TRUE)