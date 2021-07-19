/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Asset/AssetCommon.h>

#include <NvCloth/Types.h>

namespace NvCloth
{
    extern const int InvalidIndex;

    //! List of mesh nodes (names) inside an Asset.
    using MeshNodeList = AZStd::vector<AZStd::string>;

    //! Structure holding information about the submeshes of a render mesh node.
    //! While the simulation data is a single buffer for vertices and indices,
    //! this structure knows how to separate them in different submeshes, this will be
    //! be used when the MeshModificationNotificationBus request the modification
    //! of a specific submesh (lodLevel and primitiveIndex).
    struct MeshNodeInfo
    {
        //! LOD level of the mesh node inside the asset.
        int m_lodLevel = InvalidIndex;

        //! Identifies a submesh inside the render mesh.
        struct SubMesh
        {
            //! Primitive index inside the asset.
            int m_primitiveIndex = InvalidIndex;

            //! First vertex of the submesh.
            int m_verticesFirstIndex = InvalidIndex;

            //! Number of vertices of the submesh after the first vertex.
            int m_numVertices = 0;

            //! First index inside the asset.
            int m_indicesFirstIndex = InvalidIndex;

            //! Number of indices of the submesh after the first index.
            int m_numIndices = 0;
        };

        //! List of submeshes.
        AZStd::vector<SubMesh> m_subMeshes;
    };

    //! Structure with all the cloth information asset helper can obtain from the mesh.
    struct MeshClothInfo
    {
        AZStd::vector<SimParticleFormat> m_particles;
        AZStd::vector<SimIndexType> m_indices;
        AZStd::vector<SimUVType> m_uvs;
        AZStd::vector<float> m_motionConstraints;
        AZStd::vector<AZ::Vector2> m_backstopData; //!< X contains offset, Y contains radius.
        AZStd::vector<AZ::Vector3> m_tangents;
        AZStd::vector<AZ::Vector3> m_bitangents;
        AZStd::vector<AZ::Vector3> m_normals;
    };

    //! Interface to obtain cloth information from inside an Asset.
    class AssetHelper
    {
    public:
        AZ_RTTI(AssetHelper, "{8BBDFB6C-4615-4092-B38A-A1FEFEBD1A1F}");

        explicit AssetHelper(AZ::EntityId entityId);
        virtual ~AssetHelper() = default;

        //! Creates the appropriate asset helper depending on the entity's render service.
        static AZStd::unique_ptr<AssetHelper> CreateAssetHelper(AZ::EntityId entityId);

        //! Populates the list of mesh nodes inside the Asset that contains cloth information.
        virtual void GatherClothMeshNodes(MeshNodeList& meshNodes) = 0;

        //! Extracts the cloth mesh information of a node inside the Asset.
        virtual bool ObtainClothMeshNodeInfo(
            const AZStd::string& meshNode,
            MeshNodeInfo& meshNodeInfo,
            MeshClothInfo& meshClothInfo) = 0;

    protected:
        static float ConvertBackstopOffset(float backstopOffset);

        AZ::EntityId m_entityId;
    };
} // namespace NvCloth
