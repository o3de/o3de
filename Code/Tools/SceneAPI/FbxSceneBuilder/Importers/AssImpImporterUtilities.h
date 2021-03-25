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

#include <assimp/matrix4x4.h>
#include <AzCore/std/string/string.h>

struct aiNode;
struct aiScene;
struct aiString;


namespace AZ::SceneAPI::FbxSceneBuilder
{
    inline constexpr char PivotNodeMarker[] = "_$AssimpFbx$_";

    bool IsSkinnedMesh(const aiNode& node, const aiScene& scene);

    // Checks if a node name is a pivot node and optionally returns the position in the name of the pivot marker (for splitting out the parts later)
    bool IsPivotNode(const aiString& nodeName, size_t* pos = nullptr);

    // Returns the bone name and type of a pivot node
    void SplitPivotNodeName(const aiString& nodeName, size_t pivotPos, AZStd::string_view& baseNodeName, AZStd::string_view& pivotType);

    // Gets the entire, combined local transform for a node taking pivot nodes into account.  When pivot nodes are not used, this just returns the node's transform
    aiMatrix4x4 GetConcatenatedLocalTransform(const aiNode* currentNode);
} // namespace AZ

