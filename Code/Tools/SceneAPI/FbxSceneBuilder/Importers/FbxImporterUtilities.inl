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

#include <SceneAPI/FbxSceneBuilder/ImportContexts/FbxImportContexts.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxImporterUtilities.h>
#include <SceneAPI/FbxSDKWrapper/FbxNodeWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxSkinWrapper.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            bool NodeIsOfType(const CoreSceneGraph& sceneGraph, CoreGraphNodeIndex nodeIndex, const AZ::Uuid& uuid)
            {
                if (nodeIndex.IsValid() && sceneGraph.HasNodeContent(nodeIndex) &&
                    sceneGraph.GetNodeContent(nodeIndex)->RTTI_IsTypeOf(uuid))
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }

            bool NodeParentIsOfType(const CoreSceneGraph& sceneGraph, CoreGraphNodeIndex nodeIndex, const AZ::Uuid& uuid)
            {
                CoreGraphNodeIndex parentIndex = sceneGraph.GetNodeParent(nodeIndex);
                return NodeIsOfType(sceneGraph, parentIndex, uuid);
            }

            bool NodeHasAncestorOfType(const CoreSceneGraph& sceneGraph, CoreGraphNodeIndex nodeIndex, const AZ::Uuid& uuid)
            {
                CoreGraphNodeIndex parentIndex = sceneGraph.GetNodeParent(nodeIndex);
                while (parentIndex.IsValid())
                {
                    if (NodeIsOfType(sceneGraph, parentIndex, uuid))
                    {
                        return true;
                    }
                    parentIndex = sceneGraph.GetNodeParent(parentIndex);
                }

                return false;
            }

            bool IsSkinnedMesh(const FbxSDKWrapper::FbxNodeWrapper& sourceNode)
            {
                const std::shared_ptr<FbxSDKWrapper::FbxMeshWrapper> fbxMesh = sourceNode.GetMesh();
                return (fbxMesh && (fbxMesh->GetDeformerCount(FbxDeformer::eSkin) > 0 || fbxMesh->GetDeformerCount(FbxDeformer::eBlendShape) > 0));
            }

            bool AreScenesEqual(const CoreScene& lhs, const CoreScene& rhs)
            {
                if (lhs.GetGraph().GetNodeCount() != rhs.GetGraph().GetNodeCount())
                {
                    return false;
                }

                if (!AreSceneGraphsEqual(lhs.GetGraph(), rhs.GetGraph()))
                {
                    return false;
                }

                return true;
            }
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ