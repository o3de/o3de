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

            AZ_CLASS_ALLOCATOR(WhiteBoxMeshAsset, AZ::SystemAllocator, 0);
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
