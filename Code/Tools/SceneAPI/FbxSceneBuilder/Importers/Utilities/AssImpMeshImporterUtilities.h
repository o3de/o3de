/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <SceneAPI/SceneCore/Events/ProcessingResult.h>

struct aiNode;
struct aiScene;

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class FbxMeshWrapper;
    }

    namespace SceneData
    {
        namespace GraphData
        {
            class MeshData;
            class BlendShapeData;
        }
    }

    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IGraphObject;
        }
        struct AssImpSceneNodeAppendedContext;
        class FbxSceneSystem;

        namespace FbxSceneBuilder
        {
            bool BuildSceneMeshFromAssImpMesh(const aiNode* currentNode, const aiScene* scene, const FbxSceneSystem& sceneSystem, AZStd::vector<AZStd::shared_ptr<DataTypes::IGraphObject>>& meshes,
                const AZStd::function<AZStd::shared_ptr<SceneData::GraphData::MeshData>()>& makeMeshFunc);

            typedef AZ::Outcome<const SceneData::GraphData::MeshData* const, Events::ProcessingResult> GetMeshDataFromParentResult;
            GetMeshDataFromParentResult GetMeshDataFromParent(AssImpSceneNodeAppendedContext& context);

            // If a node in the original scene file has a mesh with multiple materials on it, the associated AssImp
            // node will have multiple meshes on it, broken apart per material. This returns the total number
            // of vertices on all meshes on the given node.
            uint64_t GetVertexCountForAllMeshesOnNode(const aiNode& node, const aiScene& scene);
        }
    }
}
