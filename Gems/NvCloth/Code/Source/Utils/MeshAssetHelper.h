/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Model/ModelLodAsset.h>

#include <Utils/AssetHelper.h>

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

    private:
        bool CopyDataFromMeshes(
            const AZStd::vector<const AZ::RPI::ModelLodAsset::Mesh*>& meshes,
            MeshClothInfo& meshClothInfo);
    };
} // namespace NvCloth
