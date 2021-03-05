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

namespace EMotionFX
{
    class Mesh;
}

namespace NvCloth
{
    //! Helper class to obtain cloth information from an Actor Asset.
    class ActorAssetHelper
        : public AssetHelper
    {
    public:
        AZ_RTTI(ActorAssetHelper, "{3246EAC6-595F-4AFB-BA10-44EB0B824398}", AssetHelper);

        explicit ActorAssetHelper(AZ::EntityId entityId);

        // AssetHelper overrides ...
        void GatherClothMeshNodes(MeshNodeList& meshNodes) override;
        bool ObtainClothMeshNodeInfo(
            const AZStd::string& meshNode,
            MeshNodeInfo& meshNodeInfo,
            MeshClothInfo& meshClothInfo) override;
        bool DoesSupportSkinnedAnimation() const override
        {
            return true;
        }

    private:
        bool CopyDataFromEMotionFXMesh(
            const EMotionFX::Mesh& emfxMesh,
            MeshClothInfo& meshClothInfo);
    };
} // namespace NvCloth
