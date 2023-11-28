#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Shader Headers
ly_add_target_files(
   TARGETS ${gem_name}.Builders
   FILES 
       ${CMAKE_CURRENT_SOURCE_DIR}/AZSL/Platform/Windows/DX12/AzslcHeader.azsli
       ${CMAKE_CURRENT_SOURCE_DIR}/AZSL/Platform/Windows/DX12/PlatformHeader.hlsli
   OUTPUT_SUBDIRECTORY
       Builders/ShaderHeaders/Platform/Windows/DX12
)

ly_add_target_files(
   TARGETS ${gem_name}.Builders
   FILES 
       ${CMAKE_CURRENT_SOURCE_DIR}/AZSL/Platform/Windows/Vulkan/AzslcHeader.azsli
       ${CMAKE_CURRENT_SOURCE_DIR}/AZSL/Platform/Windows/Vulkan/PlatformHeader.hlsli
   OUTPUT_SUBDIRECTORY
       Builders/ShaderHeaders/Platform/Windows/Vulkan
)

ly_add_target_files(
   TARGETS ${gem_name}.Builders
   FILES 
       ${CMAKE_CURRENT_SOURCE_DIR}/AZSL/Platform/Windows/Null/AzslcHeader.azsli
   OUTPUT_SUBDIRECTORY
       Builders/ShaderHeaders/Platform/Windows/Null
)

ly_add_target_files(
   TARGETS ${gem_name}.Builders
   FILES 
       ${CMAKE_CURRENT_SOURCE_DIR}/AZSL/Platform/Mac/Metal/AzslcHeader.azsli
       ${CMAKE_CURRENT_SOURCE_DIR}/AZSL/Platform/Mac/Metal/PlatformHeader.hlsli
   OUTPUT_SUBDIRECTORY
       Builders/ShaderHeaders/Platform/Mac/Metal
)

ly_add_target_files(
   TARGETS ${gem_name}.Builders
   FILES 
       ${CMAKE_CURRENT_SOURCE_DIR}/AZSL/Platform/Mac/Null/AzslcHeader.azsli
   OUTPUT_SUBDIRECTORY
       Builders/ShaderHeaders/Platform/Mac/Null
)

ly_add_target_files(
   TARGETS ${gem_name}.Builders
   FILES 
       ${CMAKE_CURRENT_SOURCE_DIR}/AZSL/Platform/iOS/Metal/AzslcHeader.azsli
       ${CMAKE_CURRENT_SOURCE_DIR}/AZSL/Platform/iOS/Metal/PlatformHeader.hlsli
   OUTPUT_SUBDIRECTORY
       Builders/ShaderHeaders/Platform/iOS/Metal
)

ly_add_target_files(
   TARGETS ${gem_name}.Builders
   FILES 
       ${CMAKE_CURRENT_SOURCE_DIR}/AZSL/Platform/Android/Vulkan/AzslcHeader.azsli
       ${CMAKE_CURRENT_SOURCE_DIR}/AZSL/Platform/Android/Vulkan/PlatformHeader.hlsli
   OUTPUT_SUBDIRECTORY
       Builders/ShaderHeaders/Platform/Android/Vulkan
)

ly_add_target_files(
   TARGETS ${gem_name}.Builders
   FILES 
       ${CMAKE_CURRENT_SOURCE_DIR}/AZSL/Platform/Linux/Vulkan/AzslcHeader.azsli
       ${CMAKE_CURRENT_SOURCE_DIR}/AZSL/Platform/Linux/Vulkan/PlatformHeader.hlsli
   OUTPUT_SUBDIRECTORY
       Builders/ShaderHeaders/Platform/Linux/Vulkan
)

ly_add_target_files(
   TARGETS ${gem_name}.Builders
   FILES 
       ${CMAKE_CURRENT_SOURCE_DIR}/AZSL/Platform/Linux/Null/AzslcHeader.azsli
   OUTPUT_SUBDIRECTORY
       Builders/ShaderHeaders/Platform/Linux/Null
)
