set(TARGET_WITH_NAMESPACE "Onnx")
if (TARGET ${TARGET_WITH_NAMESPACE})
    return()
endif()

set(Onnx_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/onnxruntime-win-x64-gpu-1.9.0/include)
set(Onnx_LIBS_DIR ${CMAKE_CURRENT_LIST_DIR}/onnxruntime-win-x64-gpu-1.9.0/lib)

add_library(${TARGET_WITH_NAMESPACE} STATIC IMPORTED GLOBAL)