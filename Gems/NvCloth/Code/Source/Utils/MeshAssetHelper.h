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

#include <Utils/AssetHelper.h>

struct IRenderMesh;
struct SMeshColor;

namespace NvCloth
{
    //! Helper class to obtain cloth information from a Mesh Asset.
    class MeshAssetHelper
        : public AssetHelper
    {
    public:
        AZ_RTTI(MeshAssetHelper, "{292066E4-DEB8-47C6-94CA-7BF1D75129F7}", AssetHelper);

        explicit MeshAssetHelper(AZ::EntityId entityId);

        // AssetHelper overrides ...
        void GatherClothMeshNodes(MeshNodeList& meshNodes) override;
        bool ObtainClothMeshNodeInfo(
            const AZStd::string& meshNode,
            MeshNodeInfo& meshNodeInfo,
            MeshClothInfo& meshClothInfo) override;
        bool DoesSupportSkinnedAnimation() const override
        {
            return false;
        }

    private:
        bool CopyDataFromRenderMesh(
            IRenderMesh& renderMesh,
            const AZStd::vector<SMeshColor>& renderMeshClothData,
            MeshClothInfo& meshClothInfo);
    };
} // namespace NvCloth
