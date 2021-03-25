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

        class FbxSceneSystem;
        namespace FbxSceneBuilder
        {
            bool BuildSceneMeshFromAssImpMesh(aiNode* currentNode, const aiScene* scene, const FbxSceneSystem& sceneSystem, AZStd::vector<AZStd::shared_ptr<DataTypes::IGraphObject>>& meshes,
                const AZStd::function<AZStd::shared_ptr<SceneData::GraphData::MeshData>()>& makeMeshFunc);
        }
    }
}
