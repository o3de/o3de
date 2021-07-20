/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Atom_Feature_Traits_Platform.h>
#if AZ_TRAIT_LUXCORE_SUPPORTED

#include "LuxCoreMesh.h"
#include <luxcore/luxcore.h>

namespace AZ
{
    namespace Render
    {
        LuxCoreMesh::LuxCoreMesh(AZ::Data::Asset<AZ::RPI::ModelAsset> modelAsset)
        {
            Init(modelAsset);
        }

        LuxCoreMesh::LuxCoreMesh(const LuxCoreMesh &model)
        {
            Init(model.m_modelAsset);
        }

        LuxCoreMesh::~LuxCoreMesh()
        {
            if (m_position)
            {
                delete m_position;
                m_position = nullptr;
            }

            if (m_normal)
            {
                delete m_normal;
                m_normal = nullptr;
            }

            if (m_uv)
            {
                delete m_uv;
                m_uv = nullptr;
            }

            if (m_index)
            {
                delete m_index;
                m_index = nullptr;
            }

            if (m_modelAsset)
            {
                m_modelAsset.Reset();
            }
        }

        void LuxCoreMesh::Init(AZ::Data::Asset<AZ::RPI::ModelAsset> modelAsset)
        {
            m_modelAsset = modelAsset;
            // [TODO ATOM-3547] Multiple meshes handling
            AZ::RPI::ModelLodAsset::Mesh mesh = m_modelAsset->GetLodAssets()[0]->GetMeshes()[0];

            // index data
            AZStd::array_view<uint8_t> indexBuffer = mesh.GetIndexBufferAssetView().GetBufferAsset()->GetBuffer();
            m_index = luxcore::Scene::AllocTrianglesBuffer(mesh.GetIndexCount() / 3);
            memcpy(m_index, indexBuffer.data(), indexBuffer.size());

            // vertices data
            for (AZ::RPI::ModelLodAsset::Mesh::StreamBufferInfo streamBufferInfo : mesh.GetStreamBufferInfoList())
            {
                AZStd::array_view<uint8_t> dataBuffer = streamBufferInfo.m_bufferAssetView.GetBufferAsset()->GetBuffer();

                if (streamBufferInfo.m_semantic == RHI::ShaderSemantic{ "POSITION" })
                {
                    m_position = luxcore::Scene::AllocVerticesBuffer(mesh.GetVertexCount());
                    memcpy(m_position, dataBuffer.data(), dataBuffer.size());
                }
                else if (streamBufferInfo.m_semantic == RHI::ShaderSemantic{ "NORMAL" })
                {
                    m_normal = new float[mesh.GetVertexCount() * 3];
                    memcpy(m_normal, dataBuffer.data(), dataBuffer.size());
                }
                else if (streamBufferInfo.m_semantic == RHI::ShaderSemantic{ "UV", 0 })
                {
                    m_uv = new float[mesh.GetVertexCount() * 2];
                    memcpy(m_uv, dataBuffer.data(), dataBuffer.size());
                }
            }
        }


        uint32_t LuxCoreMesh::GetVertexCount()
        {
            // [TODO ATOM-3547] Multiple meshes handling
            return m_modelAsset->GetLodAssets()[0]->GetMeshes()[0].GetVertexCount();
        }

        uint32_t LuxCoreMesh::GetTriangleCount()
        {
            // [TODO ATOM-3547] Multiple meshes handling
            return (m_modelAsset->GetLodAssets()[0]->GetMeshes()[0].GetIndexCount() / 3);
        }

        AZ::Data::AssetId LuxCoreMesh::GetMeshId()
        {
            return m_modelAsset->GetId();
        }

        const float* LuxCoreMesh::GetPositionData()
        {
            return m_position;
        }

        const float* LuxCoreMesh::GetNormalData()
        {
            return m_normal;
        }

        const float* LuxCoreMesh::GetUVData()
        {
            return m_uv;
        }

        const unsigned int* LuxCoreMesh::GetIndexData()
        {
            return m_index;
        }
    }
}
#endif
