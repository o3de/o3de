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

# Shader Headers
ly_add_target_files(
   TARGETS Atom_Asset_Shader.Builders
   FILES 
       ${CMAKE_CURRENT_SOURCE_DIR}/AZSL/Platform/Windows/DX12/AzslcHeader.azsli
       ${CMAKE_CURRENT_SOURCE_DIR}/AZSL/Platform/Windows/DX12/PlatformHeader.hlsli
   OUTPUT_SUBDIRECTORY
       Builders/ShaderHeaders/Platform/Windows/DX12
)

ly_add_target_files(
   TARGETS Atom_Asset_Shader.Builders
   FILES 
       ${CMAKE_CURRENT_SOURCE_DIR}/AZSL/Platform/Windows/Vulkan/AzslcHeader.azsli
       ${CMAKE_CURRENT_SOURCE_DIR}/AZSL/Platform/Windows/Vulkan/PlatformHeader.hlsli
   OUTPUT_SUBDIRECTORY
       Builders/ShaderHeaders/Platform/Windows/Vulkan
)

ly_add_target_files(
   TARGETS Atom_Asset_Shader.Builders
   FILES 
       ${CMAKE_CURRENT_SOURCE_DIR}/AZSL/Platform/Mac/Metal/AzslcHeader.azsli
       ${CMAKE_CURRENT_SOURCE_DIR}/AZSL/Platform/Mac/Metal/PlatformHeader.hlsli
   OUTPUT_SUBDIRECTORY
       Builders/ShaderHeaders/Platform/Mac/Metal
)

ly_add_target_files(
   TARGETS Atom_Asset_Shader.Builders
   FILES 
       ${CMAKE_CURRENT_SOURCE_DIR}/AZSL/Platform/iOS/Metal/AzslcHeader.azsli
       ${CMAKE_CURRENT_SOURCE_DIR}/AZSL/Platform/iOS/Metal/PlatformHeader.hlsli
   OUTPUT_SUBDIRECTORY
       Builders/ShaderHeaders/Platform/iOS/Metal
)

ly_add_target_files(
   TARGETS Atom_Asset_Shader.Builders
   FILES 
       ${CMAKE_CURRENT_SOURCE_DIR}/AZSL/Platform/Android/Vulkan/AzslcHeader.azsli
       ${CMAKE_CURRENT_SOURCE_DIR}/AZSL/Platform/Android/Vulkan/PlatformHeader.hlsli
   OUTPUT_SUBDIRECTORY
       Builders/ShaderHeaders/Platform/Android/Vulkan
)
