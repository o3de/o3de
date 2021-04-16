project (Framework_DX12)

add_compile_options(/MP)

include(common)

set(GLTF_src
    GLTF/GltfBBoxPass.cpp
    GLTF/GltfBBoxPass.h
    GLTF/GltfDepthPass.cpp
    GLTF/GltfDepthPass.h
    GLTF/GltfHelpers.cpp
    GLTF/GltfHelpers.h
    GLTF/GltfPbrPass.cpp
    GLTF/GltfPbrPass.h
    GLTF/GLTFTexturesAndBuffers.cpp
    GLTF/GLTFTexturesAndBuffers.h
)

set(PostProc_src
    PostProc/Bloom.cpp
    PostProc/Bloom.h
    PostProc/BlurPS.cpp
    PostProc/BlurPS.h
    PostProc/Debug.cpp
    PostProc/Debug.h
    PostProc/DownSamplePS.cpp
    PostProc/DownSamplePS.h
    PostProc/PostProcCS.cpp
    PostProc/PostProcCS.h
    PostProc/PostProcPS.cpp
    PostProc/PostProcPS.h
    PostProc/SkyDome.cpp
    PostProc/SkyDome.h
    PostProc/SkyDomeProc.cpp
    PostProc/SkyDomeProc.h
    PostProc/Tonemapping.cpp
    PostProc/Tonemapping.h
)

set(Base_src
    base/CommandListRing.cpp
    base/CommandListRing.h
    base/Device.cpp
    base/Device.h
    base/DynamicBufferRing.cpp
    base/DynamicBufferRing.h
    base/Fence.cpp
    base/Fence.h
    base/FreeSync2.cpp
    base/FreeSync2.h
    base/GPUTimestamps.cpp
    base/GPUTimestamps.h
    base/Helper.cpp
    base/Helper.h
    base/Imgui.cpp
    base/Imgui.h
    base/ResourceViewHeaps.cpp
    base/ResourceViewHeaps.h
    base/ShaderCompilerHelper.cpp
    base/ShaderCompilerHelper.h
    base/StaticBufferPool.cpp
    base/StaticBufferPool.h
    base/StaticConstantBufferPool.cpp
    base/StaticConstantBufferPool.h
    base/SwapChain.cpp
    base/SwapChain.h
    base/Texture.cpp
    base/Texture.h
    base/UploadHeap.cpp
    base/UploadHeap.h
    base/UserMarkers.h
    base/UserMarkers.cpp
)

set(Widgets_src
    widgets/Wireframe.cpp
    widgets/Wireframe.h
    widgets/WireframeBox.h
    widgets/WireframeSphere.h
)

set(Shaders_src
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/common.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/blend.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/blur.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/Debug.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/DownSamplePS.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFPbrPass-PS.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFPbrPass-VS.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFPbrPass-IO.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFDepthPass.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/SkyDome.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/SkyDomeProc.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/Tonemapping.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/functions.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/textures.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shadowFiltering.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/PixelParams.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/Skinning.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/GLTFVertexFactory.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/perFrameStruct.h
)

set(media_src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../media/brdfLut.dds
)

copyCommand("${media_src}" ${CMAKE_HOME_DIRECTORY}/bin)
copyCommand("${Shaders_src}" ${CMAKE_HOME_DIRECTORY}/bin/ShaderLibDX)

add_library (${PROJECT_NAME} STATIC ${GLTF_src} ${PostProc_src} ${Base_src} ${Widgets_src} ${Shaders_src} ${media_src})

target_link_libraries (${PROJECT_NAME} Framework_Common NJSON D3D12X)
target_include_directories (${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

source_group("GLTF"            FILES ${GLTF_src})
source_group("PostProc"        FILES ${PostProc_src})
source_group("Base"            FILES ${Base_src})
source_group("Widgets"         FILES ${Widgets_src})
source_group("Shaders"         FILES ${Shaders_src})
source_group("media"           FILES ${media_src})

set_source_files_properties(${Shaders_src} PROPERTIES VS_TOOL_OVERRIDE "Text")
set_source_files_properties(${media_src} PROPERTIES VS_TOOL_OVERRIDE "Media")


