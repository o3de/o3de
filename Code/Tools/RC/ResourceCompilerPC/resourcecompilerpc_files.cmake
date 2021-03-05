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

set(FILES
    CGF/CGFNodeMerger.cpp
    CGF/DataWriter.cpp
    CGF/AssetWriter.cpp
    CGF/CGFNodeMerger.h
    CGF/ChunkData.h
    CGF/DataWriter.h
    CGF/AssetWriter.h
    ChunkCompiler.cpp
    ChunkCompiler.h
    LuaCompiler.cpp
    LuaCompiler.h
    ResourceCompilerPC_precompiled.h
    ResourceCompilerPC_precompiled.cpp
    StatCGFCompiler.cpp
    StaticObjectCompiler.cpp
    StatCGFCompiler.h
    StaticObjectCompiler.h
    PhysWorld.h
)

set(SKIP_UNITY_BUILD_INCLUSION_FILES
    StatCGFCompiler.h
    StatCGFCompiler.cpp
)
