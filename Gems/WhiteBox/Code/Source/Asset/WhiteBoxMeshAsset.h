/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    namespace Pipeline
    {
        //! Represents a white box mesh asset.
        //! This asset can be modified in memory by the white box tool.
        class WhiteBoxMeshAsset : public AZ::Data::AssetData
        {
        public:
            friend class WhiteBoxMeshAssetHandler;

            AZ_CLASS_ALLOCATOR(WhiteBoxMeshAsset, AZ::SystemAllocator);
            AZ_RTTI(WhiteBoxMeshAsset, "{6784304A-4ED6-42FD-A5C9-316265F071F2}", AZ::Data::AssetData);

            ~WhiteBoxMeshAsset()
            {
                ReleaseMemory();
            }

            void SetMesh(Api::WhiteBoxMeshPtr mesh)
            {
                m_mesh = AZStd::move(mesh);
                m_status = AssetStatus::Ready;
            }

            WhiteBoxMesh* GetMesh()
            {
                return m_mesh.get();
            }

            Api::WhiteBoxMeshPtr ReleaseMesh()
            {
                return AZStd::move(m_mesh);
            }

            void SetWhiteBoxData(Api::WhiteBoxMeshStream whiteBoxData)
            {
                m_whiteBoxData = AZStd::move(whiteBoxData);
            }

            const Api::WhiteBoxMeshStream& GetWhiteBoxData() const
            {
                return m_whiteBoxData;
            }

            void Serialize()
            {
                Api::WriteMesh(*m_mesh, m_whiteBoxData);
            }

        private:
            void ReleaseMemory()
            {
                m_mesh.reset();
            }

            Api::WhiteBoxMeshPtr m_mesh;
            Api::WhiteBoxMeshStream m_whiteBoxData; //! Data used for creating undo commands.
        };
    } // namespace Pipeline
} // namespace WhiteBox
