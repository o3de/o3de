/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once


#include <Atom_Feature_Traits_Platform.h>
#if AZ_TRAIT_LUXCORE_SUPPORTED

#include <Atom/RPI.Reflect/Model/ModelAsset.h>

namespace AZ
{
    namespace Render
    {
        // Extract vertex and index data from source model
        class LuxCoreMesh final
        {
        public:
            LuxCoreMesh() = default;
            LuxCoreMesh(AZ::Data::Asset<AZ::RPI::ModelAsset> modelAsset);
            LuxCoreMesh(const LuxCoreMesh &mesh);
            ~LuxCoreMesh();

            AZ::Data::AssetId GetMeshId();
            const float* GetPositionData();
            const float* GetNormalData();
            const float* GetUVData();
            const unsigned int* GetIndexData();

            uint32_t GetVertexCount();
            uint32_t GetTriangleCount();

        private:
            void Init(AZ::Data::Asset<AZ::RPI::ModelAsset> modelAsset);

            float* m_position = nullptr;
            float* m_normal = nullptr;
            float* m_uv = nullptr;
            unsigned int* m_index = nullptr;
            AZ::Data::Asset<AZ::RPI::ModelAsset> m_modelAsset;
        };
    }
}
#endif
