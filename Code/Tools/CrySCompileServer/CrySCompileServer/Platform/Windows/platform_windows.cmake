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

set(LY_RUNTIME_DEPENDENCIES
    3rdParty::DirectXShaderCompiler::dxcGL
    3rdParty::DirectXShaderCompiler::dxcMetal
)

file(TO_CMAKE_PATH "$ENV{ProgramFiles\(x86\)}" program_files_path)

ly_add_target_files(
    TARGETS CrySCompileServer
    OUTPUT_SUBDIRECTORY Compiler/PCD3D11/v006
    FILES
        "${program_files_path}/Windows Kits/10/bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/x64/fxc.exe"
        "${program_files_path}/Windows Kits/10/bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/x64/d3dcompiler_47.dll"
        "${program_files_path}/Windows Kits/10/bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/x64/d3dcsx_47.dll"
        "${program_files_path}/Windows Kits/10/bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/x64/d3dcsxd_47.dll"
)

ly_add_target_files(
    TARGETS CrySCompileServer
    OUTPUT_SUBDIRECTORY Compiler/PCGL/V006
    FILES
        "${LY_ROOT_FOLDER}/Tools/CrySCompileServer/Compiler/PCGL/V006/D3DCompiler_47.dll"
        "${LY_ROOT_FOLDER}/Tools/CrySCompileServer/Compiler/PCGL/V006/HLSLcc.exe"
)

ly_add_target_files(
    TARGETS CrySCompileServer
    OUTPUT_SUBDIRECTORY Compiler/PCGMETAL/HLSLcc
    FILES
        "${LY_ROOT_FOLDER}/Tools/CrySCompileServer/Compiler/PCGMETAL/HLSLcc/HLSLcc_d.exe"
        "${LY_ROOT_FOLDER}/Tools/CrySCompileServer/Compiler/PCGMETAL/HLSLcc/HLSLcc.exe"
)

