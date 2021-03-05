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

set(DIRECTXSHADERCOMPILER_BINARY_BASE_PATH ${BASE_PATH}/bin/darwin_x64/$<IF:$<CONFIG:Debug>,debug,release>)

set(DIRECTXSHADERCOMPILER_DXCGL_RUNTIME_DEPENDENCIES ${DIRECTXSHADERCOMPILER_BINARY_BASE_PATH}/dxcGL\nCompiler/LLVMGL/$<IF:$<CONFIG:Debug>,debug,release>)

set(DIRECTXSHADERCOMPILER_DXCMETAL_RUNTIME_DEPENDENCIES ${DIRECTXSHADERCOMPILER_BINARY_BASE_PATH}/dxcMetal\nCompiler/LLVMMETAL/$<IF:$<CONFIG:Debug>,debug,release>)


set(input_path "${BASE_PATH}/bin/darwin_x64/$<IF:$<CONFIG:Debug>,Debug,Release>") 
set(output_subfolder Builders/DirectXShaderCompilerAz)
set(output_subfolder_lib Builders/DirectXShaderCompilerAz/lib)
set(DIRECTXSHADERCOMPILER_DXCAZ_RUNTIME_DEPENDENCIES
        ${input_path}/bin/dxc\n${output_subfolder}/bin
        ${input_path}/lib/libdxcompiler.3.7.dylib\n${output_subfolder_lib}
        ${input_path}/lib/libglcompiler.3.7.dylib\n${output_subfolder_lib}
        ${input_path}/lib/libmetalcompiler.3.7.dylib\n${output_subfolder_lib}
)
