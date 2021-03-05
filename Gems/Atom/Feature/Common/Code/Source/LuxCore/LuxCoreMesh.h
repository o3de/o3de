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
