project (TressFX_${GFX_API})

add_subdirectory(Math)
add_subdirectory(TressFX)

if(GFX_API STREQUAL DX12)
    add_compile_definitions(TRESSFX_DX12)
    add_subdirectory(DX12)
elseif(GFX_API STREQUAL VK)
    add_compile_definitions(TRESSFX_VK)
    find_package(Vulkan REQUIRED)
    add_subdirectory(VK)
else()
    message(STATUS "----------------------------------------------------------------------------------------")
    message(STATUS "")
    message(STATUS "** Almost there!!")
    message(STATUS "")
    message(STATUS " This framework supports DX12 or VULKAN, you need to invoke cmake in one of these ways:")
        message(STATUS "")
    message(STATUS " Examples:")
    message(STATUS "    cmake <project_root_dir> -DGFX_API=DX12")
    message(STATUS "    cmake <project_root_dir> -DGFX_API=VK")
    message(STATUS "")
    message(STATUS "----------------------------------------------------------------------------------------")
    message(FATAL_ERROR "")
endif()

set(common_sources 
	EngineInterface.h
	SceneGLTFImpl.cpp
	SceneGLTFImpl.h
	TressFXSample.cpp
	TressFXSample.h
	Simulation.cpp
	Simulation.h
	HairStrands.cpp
	HairStrands.h
	SDF.cpp
	SDF.h)



set(shaders_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/FullScreenRender.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/TressFXPPLL.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/TressFXLighting.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/TressFXRendering.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/TressFXSDFCollision.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/TressFXShadow.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/TressFXShortCut.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/TressFXSimulation.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/TressFXStrands.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/TressFXBoneSkinning.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/TressFXMarchingCubes.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/TressFXUtilities.hlsl)

source_group("Math Sources"            FILES ${math_sources})
source_group("TressFX Sources"            FILES ${tressfx_sources})
source_group("Common Sources"            FILES ${common_sources})
source_group("${GFX_API} Sources"            FILES ${sources})
source_group("Shader Sources"            FILES ${shaders_sources})

add_executable(${PROJECT_NAME} WIN32 ${sources} ${common_sources} ${tressfx_sources} ${math_sources} ${shaders_sources})

if(GFX_API STREQUAL DX12)
	target_link_libraries (${PROJECT_NAME} LINK_PUBLIC Framework_${GFX_API} ImGUI amd_ags DXC d3dcompiler D3D12)
elseif(GFX_API STREQUAL VK)
	target_link_libraries (${PROJECT_NAME} LINK_PUBLIC Framework_${GFX_API} ImGUI Vulkan::Vulkan)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}/bin")

if(GFX_API STREQUAL DX12)
    copyCommand("${shaders_sources}" ${CMAKE_HOME_DIRECTORY}/bin/ShaderLibDX)
elseif(GFX_API STREQUAL VK)
    copyCommand("${shaders_sources}" ${CMAKE_HOME_DIRECTORY}/bin/ShaderLibVK)
endif()

