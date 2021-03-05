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
    FbxSceneBuilderConfiguration.h
    FbxImportRequestHandler.h
    FbxImportRequestHandler.cpp
    FbxImporter.h
    FbxImporter.cpp
    FbxSceneSystem.h
    FbxSceneSystem.cpp
    ImportContexts/ImportContexts.h
    ImportContexts/ImportContexts.cpp
    ImportContexts/FbxImportContexts.h
    ImportContexts/FbxImportContexts.cpp
    Importers/FbxAnimationImporter.h
    Importers/FbxAnimationImporter.cpp
    Importers/FbxBoneImporter.h
    Importers/FbxBoneImporter.cpp
    Importers/FbxBlendShapeImporter.h
    Importers/FbxBlendShapeImporter.cpp
    Importers/FbxColorStreamImporter.h
    Importers/FbxColorStreamImporter.cpp
    Importers/FbxTangentStreamImporter.h
    Importers/FbxTangentStreamImporter.cpp
    Importers/FbxBitangentStreamImporter.h
    Importers/FbxBitangentStreamImporter.cpp
    Importers/FbxImporterUtilities.h
    Importers/FbxImporterUtilities.inl
    Importers/FbxImporterUtilities.cpp
    Importers/FbxMaterialImporter.h
    Importers/FbxMaterialImporter.cpp
    Importers/FbxMeshImporter.h
    Importers/FbxMeshImporter.cpp
    Importers/FbxSkinImporter.h
    Importers/FbxSkinImporter.cpp
    Importers/FbxSkinWeightsImporter.h
    Importers/FbxSkinWeightsImporter.cpp
    Importers/FbxTransformImporter.h
    Importers/FbxTransformImporter.cpp
    Importers/FbxUvMapImporter.h
    Importers/FbxUvMapImporter.cpp
    Importers/Utilities/FbxMeshImporterUtilities.h
    Importers/Utilities/FbxMeshImporterUtilities.cpp
    Importers/Utilities/RenamedNodesMap.h
    Importers/Utilities/RenamedNodesMap.cpp
)
