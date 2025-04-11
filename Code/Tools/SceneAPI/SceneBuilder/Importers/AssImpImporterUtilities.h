/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <assimp/matrix4x4.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/DataTypes/MatrixType.h>

struct aiBone;
struct aiNode;
struct aiScene;
struct aiString;

namespace AZ::SceneAPI::SceneBuilder
{
    inline constexpr char PivotNodeMarker[] = "_$AssimpFbx$_";

    bool IsSkinnedMesh(const aiNode& node, const aiScene& scene);

    // Checks if a node name is a pivot node and optionally returns the position in the name of the pivot marker (for splitting out the parts later)
    bool IsPivotNode(const aiString& nodeName, size_t* pos = nullptr);

    // Returns the bone name and type of a pivot node
    void SplitPivotNodeName(const aiString& nodeName, size_t pivotPos, AZStd::string_view& baseNodeName, AZStd::string_view& pivotType);

    // Gets the entire, combined local transform for a node taking pivot nodes into account.  When pivot nodes are not used, this just returns the node's transform
    aiMatrix4x4 GetConcatenatedLocalTransform(const aiNode* currentNode);

    DataTypes::MatrixType GetLocalSpaceBindPoseTransform(const aiScene* scene, const aiNode* node);

    // Gather all bones from the scene. (Bone in AssImp corresponds to nodes that influence any of the vertices).
    void FindAllBones(const aiScene* scene, AZStd::unordered_multimap<AZStd::string, const aiBone*>& outBoneByNameMap);

    // Find the first bone with the name of the given node.
    const aiBone* FindFirstBoneByNodeName(const aiNode* node, AZStd::unordered_multimap<AZStd::string, const aiBone*>& boneByNameMap);

    // Check if the given node or any of its children, or children of children, is a bone by checking if the node name is part of the given maps.
    bool RecursiveHasChildBone(const aiNode* node, const AZStd::unordered_multimap<AZStd::string, const aiBone*>& boneByNameMap, const AZStd::unordered_set<AZStd::string>& animatedNodesMap);
} // namespace AZ

