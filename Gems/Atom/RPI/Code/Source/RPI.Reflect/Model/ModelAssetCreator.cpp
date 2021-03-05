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

#include <Atom/RPI.Reflect/Model/ModelAssetCreator.h>

#include <AzCore/Asset/AssetManager.h>

namespace AZ
{
    namespace RPI
    {
        void ModelAssetCreator::Begin(const Data::AssetId& assetId)
        {
            BeginCommon(assetId);

            m_modelAabb = Aabb::CreateNull();
        }

        void ModelAssetCreator::SetName(AZStd::string_view name)
        {
            if (ValidateIsReady())
            {
                m_asset->m_name = name;
            }
        }

        void ModelAssetCreator::AddLodAsset(Data::Asset<ModelLodAsset>&& lodAsset)
        {
            if (ValidateIsReady())
            {
                m_asset->m_lodAssets.push_back(AZStd::move(lodAsset));
                m_modelAabb.AddAabb(m_asset->m_lodAssets.back()->GetAabb());
            }
        }

        bool ModelAssetCreator::End(Data::Asset<ModelAsset>& result)
        {
            if (!ValidateIsReady())
            {
                return false;
            }

            if (m_asset->GetLodCount() == 0)
            {
                ReportError("No valid ModelLodAssets have been added to this ModelAsset.");
                return false;
            }

            // Create Model Aabb as it wraps all ModelLod Aabbs
            for (const auto& modelLodAsset : m_asset->GetLodAssets())
            {
                m_asset->m_aabb.AddAabb(modelLodAsset->GetAabb());
            }

            m_asset->SetReady();
            return EndCommon(result);
        }
    } // namespace RPI
} // namespace AZ