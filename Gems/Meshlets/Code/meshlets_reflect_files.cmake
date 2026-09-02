#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
# Reflect-tier file list. Sources are appended in subsequent tasks (the
# header lands in Task 2; writer/reader/asset class follow). Keeping this
# empty in Task 1 is deliberate: O3DE's `ly_include_cmake_file_list`
# emits SEND_ERROR for any listed file that doesn't exist on disk yet,
# which would break `cmake configure`. Empty list means cmake configure
# stays green; Visual Studio's lib.exe accepts the resulting empty static
# archive without warning.

set(FILES
    Source/Meshlets/Reflect/MeshletPackFormat.h
    Source/Meshlets/Reflect/MeshletPackWriter.h
    Source/Meshlets/Reflect/MeshletPackWriter.cpp
    Source/Meshlets/Reflect/MeshletPackReader.h
    Source/Meshlets/Reflect/MeshletPackReader.cpp
    Source/Meshlets/Reflect/MeshletPackAsset.h
    Source/Meshlets/Reflect/MeshletPackAsset.cpp
    Source/Meshlets/Reflect/MeshletPackAssetHandler.h
    Source/Meshlets/Reflect/MeshletPackAssetHandler.cpp
)
