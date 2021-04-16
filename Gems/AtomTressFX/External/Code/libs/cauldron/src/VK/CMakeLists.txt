project (Framework_VK)

add_compile_options(/MP)

include(common)

set(GLTF_src
    GLTF/GltfBBoxPass.cpp
    GLTF/GltfBBoxPass.h
    GLTF/GltfDepthPass.cpp
    GLTF/GltfDepthPass.h
    GLTF/glTFHelpers.cpp
    GLTF/glTFHelpers.h
    GLTF/GltfPbrPass.cpp
    GLTF/GltfPbrPass.h
    GLTF/GLTFTexturesAndBuffers.cpp
    GLTF/GLTFTexturesAndBuffers.h)

set(PostProc_src
    PostProc/Bloom.cpp
    PostProc/Bloom.h
    PostProc/BlurPS.cpp
    PostProc/BlurPS.h
    PostProc/DownSamplePS.cpp
    PostProc/DownSamplePS.h
    PostProc/PostProcPS.cpp
    PostProc/PostProcPS.h
    PostProc/PostProcCS.cpp
    PostProc/PostProcCS.h
    PostProc/SkyDome.cpp
    PostProc/SkyDome.h
    PostProc/SkyDomeProc.cpp
    PostProc/SkyDomeProc.h
    PostProc/Tonemapping.cpp
    PostProc/Tonemapping.h)

set(Base_src
    base/CommandListRing.cpp
    base/CommandListRing.h
    base/Device.cpp
    base/Device.h
    base/DeviceProperties.cpp
    base/DeviceProperties.h
    base/DynamicBufferRing.cpp
    base/DynamicBufferRing.h
    base/GPUTimestamps.cpp
    base/GPUTimestamps.h
    base/FreeSync2.cpp
    base/FreeSync2.h
    base/ExtDebugMarkers.cpp
    base/ExtDebugMarkers.h
    base/ExtValidation.cpp
    base/ExtValidation.h
    base/ExtFreeSync2.cpp
    base/ExtFreeSync2.h
    base/ExtFp16.cpp
    base/ExtFp16.h
    base/Helper.cpp
    base/Helper.h
    base/Imgui.cpp
    base/Imgui.h
    base/Instance.cpp
    base/Instance.h
    base/InstanceProperties.cpp
    base/InstanceProperties.h
    base/ResourceViewHeaps.cpp
    base/ResourceViewHeaps.h
    base/ShaderCompilerHelper.cpp
    base/ShaderCompilerHelper.h
    base/StaticBufferPool.cpp
    base/StaticBufferPool.h
    base/SwapChain.cpp
    base/SwapChain.h
    base/Texture.cpp
    base/Texture.h
    base/UploadHeap.cpp
    base/UploadHeap.h)

set(Widgets_src
    widgets/Axis.cpp
    widgets/Axis.h
    widgets/CheckerBoardFloor.cpp
    widgets/CheckerboardFloor.h
    widgets/Wireframe.cpp
    widgets/Wireframe.h
    widgets/WireframeBox.h
    widgets/WireframeSphere.h)

set(Shaders_src
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/blend.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/blur.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/DownSamplePS.glsl
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTF_VS2PS_IO.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFPbrPass-frag.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFPbrPass-vert.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shadowFiltering.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFDepthPass-frag.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFDepthPass-vert.glsl
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFVertexFactory.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/skinning.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/SkyDome.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/SkyDome.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/SkyDomeProc.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/SkyDomeProc.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/tonemapping.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/functions.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/textures.glsl
    )

set(media_src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../media/brdfLut.dds
)    

copyCommand("${media_src}" ${CMAKE_HOME_DIRECTORY}/bin)
copyCommand("${Shaders_src}" ${CMAKE_HOME_DIRECTORY}/bin/ShaderLibVK)

add_library (${PROJECT_NAME} STATIC ${GLTF_src} ${PostProc_src} ${Base_src} ${Widgets_src} ${Shaders_src} ${media_src})

target_link_libraries (${PROJECT_NAME} Framework_Common Vulkan::Vulkan NJSON)
target_include_directories (${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

source_group("GLTF"            FILES ${GLTF_src})
source_group("PostProc"        FILES ${PostProc_src})
source_group("Base"            FILES ${Base_src})
source_group("Widgets"         FILES ${Widgets_src})
source_group("Shaders"         FILES ${Shaders_src})
source_group("media"           FILES ${media_src})

set_source_files_properties(${Shaders_src} PROPERTIES VS_TOOL_OVERRIDE "Text")
set_source_files_properties(${media_src} PROPERTIES VS_TOOL_OVERRIDE "Media")

