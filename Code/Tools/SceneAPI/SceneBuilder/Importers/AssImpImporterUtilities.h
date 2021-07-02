/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <assimp/matrix4x4.h>
#include <AzCore/std/string/string.h>

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
} // namespace AZ

