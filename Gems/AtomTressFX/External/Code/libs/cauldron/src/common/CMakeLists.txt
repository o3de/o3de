project (Framework_Common)

add_compile_options(/MP)

set(base_src
    "base/ImGuiHelper.cpp"
    "base/ImGuiHelper.h"
    "base/ShaderCompiler.cpp"
    "base/ShaderCompiler.h"
)

set(GLTF_src
    "GLTF/GltfStructures.h"
    "GLTF/GltfCommon.cpp"
    "GLTF/GltfCommon.h"
    "GLTF/GltfPbrMaterial.cpp"
    "GLTF/GltfPbrMaterial.h"
    "GLTF/glTFHelpers.cpp"
    "GLTF/glTFHelpers.h"
)

file(GLOB_RECURSE Misc_src
    "Misc/*.cpp"
    "Misc/*.h"
)

add_library (${PROJECT_NAME} STATIC ${base_src} ${GLTF_src} ${Misc_src} )
target_link_libraries (${PROJECT_NAME} NJSON)
target_include_directories (${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

source_group("base"         FILES ${base_src})
source_group("GLTF"         FILES ${GLTF_src})
source_group("Misc"         FILES ${Misc_src})

