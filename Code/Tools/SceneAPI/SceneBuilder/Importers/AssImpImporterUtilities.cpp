/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <assimp/scene.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <SceneAPI/SDKWrapper/AssImpTypeConverter.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpImporterUtilities.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
        {
            bool IsSkinnedMesh(const aiNode& node, const aiScene& scene)
            {
                unsigned boneCount = 0;

                for (unsigned mesh = 0; mesh < node.mNumMeshes; ++mesh)
                {
                    if (scene.mMeshes[node.mMeshes[mesh]]->HasBones())
                    {
                        ++boneCount;
                    }
                }

                if (boneCount > 1 && boneCount != node.mNumMeshes)
                {
                    AZ_Error("AssImpImporterUtilities", false, "Node has %d meshes but only %d are skinned.  "
                        "This is unexpected and may result in errors", node.mNumMeshes, boneCount);
                }

                return boneCount > 0;
            }

            bool IsPivotNode(const aiString& nodeName, size_t* pos)
            {
                AZStd::string_view name(nodeName.C_Str(), nodeName.length);

                size_t myPos;

                if (!pos)
                {
                    pos = &myPos;
                }

                *pos = AZ::StringFunc::Find(name, PivotNodeMarker);

                return *pos != name.npos;
            }

            void SplitPivotNodeName(const aiString& nodeName, size_t pivotPos, AZStd::string_view& baseNodeName, AZStd::string_view& pivotType)
            {
                AZStd::string_view nodeNameView(nodeName.data, nodeName.length);
                baseNodeName = AZStd::string_view(nodeNameView.substr(0, pivotPos));
                pivotType = AZStd::string_view(nodeNameView.substr(pivotPos + sizeof PivotNodeMarker - 1));
            }

            aiMatrix4x4 GetConcatenatedLocalTransform(const aiNode* currentNode)
            {
                const aiNode* parent = currentNode->mParent;
                aiMatrix4x4 combinedTransform = currentNode->mTransformation;

                while (parent)
                {
                    size_t pos;

                    if (IsPivotNode(parent->mName, &pos))
                    {
                        combinedTransform = parent->mTransformation * combinedTransform;
                        parent = parent->mParent;
                    }
                    else
                    {
                        break;
                    }
                }

                return combinedTransform;
            }

            void FindAllBones(const aiScene* scene, AZStd::unordered_multimap<AZStd::string, const aiBone*>& outBoneByNameMap)
            {
                outBoneByNameMap.clear();
                AZStd::queue<const aiNode*> queue;
                AZStd::unordered_set<AZStd::string> nodesWithNoMesh;

                queue.push(scene->mRootNode);

                while (!queue.empty())
                {
                    const aiNode* currentNode = queue.front();
                    queue.pop();

                    if (currentNode->mNumMeshes == 0)
                    {
                        nodesWithNoMesh.emplace(currentNode->mName.C_Str());
                    }

                    for (unsigned int childIndex = 0; childIndex < currentNode->mNumChildren; ++childIndex)
                    {
                        queue.push(currentNode->mChildren[childIndex]);
                    }
                }

                for (unsigned meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
                {
                    const aiMesh* mesh = scene->mMeshes[meshIndex];

                    for (unsigned boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
                    {
                        const aiBone* bone = mesh->mBones[boneIndex];

                        if (nodesWithNoMesh.contains(bone->mName.C_Str()))
                        {
                            outBoneByNameMap.emplace(bone->mName.C_Str(), bone);
                        }
                    }
                }
            }

            DataTypes::MatrixType GetLocalSpaceBindPoseTransform(const aiScene* scene, const aiNode* node)
            {
                AZStd::unordered_multimap<AZStd::string, const aiBone*> boneByNameMap;
                FindAllBones(scene, boneByNameMap);

                const aiBone* bone = FindFirstBoneByNodeName(node, boneByNameMap);
                if (bone)
                {
                    const aiBone* parentBone = FindFirstBoneByNodeName(node->mParent, boneByNameMap);
                    if (parentBone)
                    {
                        DataTypes::MatrixType inverseOffsetMatrix = AssImpSDKWrapper::AssImpTypeConverter::ToTransform(bone->mOffsetMatrix).GetInverseFull();
                        const DataTypes::MatrixType parentBoneOffsetMatrix = AssImpSDKWrapper::AssImpTypeConverter::ToTransform(parentBone->mOffsetMatrix);
                        return parentBoneOffsetMatrix * inverseOffsetMatrix;
                    }
                }

                return AssImpSDKWrapper::AssImpTypeConverter::ToTransform(GetConcatenatedLocalTransform(node));
            }

            const aiBone* FindFirstBoneByNodeName(const aiNode* node, AZStd::unordered_multimap<AZStd::string, const aiBone*>& boneByNameMap)
            {
                if (!node)
                {
                    return nullptr;
                }

                auto boneIterator = boneByNameMap.find(node->mName.C_Str());
                if (boneIterator != boneByNameMap.end())
                {
                    return boneIterator->second;
                }

                return nullptr;
            }

            bool RecursiveHasChildBone(const aiNode* node, const AZStd::unordered_multimap<AZStd::string, const aiBone*>& boneByNameMap, const AZStd::unordered_set<AZStd::string>& animatedNodesMap)
            {
                const bool isBone = boneByNameMap.contains(node->mName.C_Str()) || animatedNodesMap.contains(node->mName.C_Str());
                if (isBone)
                {
                    return true;
                }

                for (unsigned int childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
                {
                    const aiNode* childNode = node->mChildren[childIndex];
                    if (RecursiveHasChildBone(childNode, boneByNameMap, animatedNodesMap))
                    {
                        return true;
                    }
                }

                return false;
            }
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
