#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
# Builder-tier file list. Sources are appended in subsequent tasks
# (SourceMeshSet.h in Task 7; MeshletPackBuilderCore.{h,cpp} in Task 8;
# MeshletPackRule.{h,cpp} in Task 14; SceneAPI exporter in Task 15;
# JSON sidecar in Task 16). Keeping this empty in Task 6 is deliberate:
# O3DE's `ly_include_cmake_file_list` emits SEND_ERROR for any listed
# file that doesn't exist on disk yet, which would break `cmake
# configure`. Empty list means cmake configure stays green; Visual
# Studio's lib.exe accepts the resulting empty static archive without
# warning.

set(FILES
    Source/Builders/MeshletPackBuilderCore.h
    Source/Builders/MeshletPackBuilderCore.cpp
    Source/Builders/SourceMeshSet.h
    Source/Builders/MeshletPackRule.h
    Source/Builders/MeshletPackRule.cpp
    Source/Builders/SceneApiMeshletPackExporter.h
    Source/Builders/SceneApiMeshletPackExporter.cpp
    Source/Builders/MeshletPackRuleBehavior.h
    Source/Builders/MeshletPackRuleBehavior.cpp
    Source/Builders/JsonSidecarDescriptor.h
    Source/Builders/JsonSidecarDescriptor.cpp
    Source/Builders/JsonSidecarBuilder.h
    Source/Builders/JsonSidecarBuilder.cpp
)
