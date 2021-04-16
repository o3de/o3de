cmake_minimum_required(VERSION 3.4)
set(CMAKE_GENERATOR_PLATFORM x64)

project (Framework_${GFX_API})

list( APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} )

# ouput exe to bin directory
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_HOME_DIRECTORY}/bin )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

# reference libs used by both backends
add_subdirectory(src/Common)
add_subdirectory(libs/json)
add_subdirectory(libs/ImGUI)

if(GFX_API STREQUAL DX12)        
    add_subdirectory(libs/d3d12x)
    add_subdirectory(libs/ags)    
    add_subdirectory(libs/dxc)
    add_subdirectory(src/DX12)   
elseif(GFX_API STREQUAL VK)
    find_package(Vulkan REQUIRED)
    add_subdirectory(libs/VulkanMemoryAllocator)    
    add_subdirectory(src/VK)
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

