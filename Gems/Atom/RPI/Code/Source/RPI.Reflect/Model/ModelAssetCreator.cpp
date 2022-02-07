/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Model/ModelAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelLodAssetCreator.h>

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
        
        void ModelAssetCreator::AddMaterialSlot(const ModelMaterialSlot& materialSlot)
        {
            if (ValidateIsReady())
            {
                auto iter = m_asset->m_materialSlots.find(materialSlot.m_stableId);

                if (iter == m_asset->m_materialSlots.end())
                {
                    m_asset->m_materialSlots[materialSlot.m_stableId] = materialSlot;
                }
                else
                {
                    if (materialSlot.m_displayName != iter->second.m_displayName)
                    {
                        ReportWarning("Material slot %u was already added with a different name.", materialSlot.m_stableId);
                    }

                    if (materialSlot.m_defaultMaterialAsset != iter->second.m_defaultMaterialAsset)
                    {
                        ReportWarning("Material slot %u was already added with a different default MaterialAsset.", materialSlot.m_stableId);
                    }

                    iter->second = materialSlot;
                }
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

        bool ModelAssetCreator::Clone(const Data::Asset<ModelAsset>& sourceAsset, Data::Asset<ModelAsset>& clonedResult, const Data::AssetId& cloneAssetId)
        {
            if (!sourceAsset.IsReady())
            {
                return false;
            }

            ModelAssetCreator creator;
            creator.Begin(cloneAssetId);
            creator.SetName(sourceAsset->GetName().GetStringView());

            AZ::Data::AssetId lastUsedId = cloneAssetId;
            const AZStd::array_view<Data::Asset<ModelLodAsset>> sourceLodAssets = sourceAsset->GetLodAssets();
            for (const Data::Asset<ModelLodAsset>& sourceLodAsset : sourceLodAssets)
            {
                Data::Asset<ModelLodAsset> lodAsset;
                if (!ModelLodAssetCreator::Clone(sourceLodAsset, lodAsset, lastUsedId))
                {
                    AZ_Error("ModelAssetCreator", false,
                        "Cannot clone model lod asset for '%s'.", sourceLodAsset.GetHint().c_str());
                    return false;
                }

                if (lodAsset.IsReady())
                {
                    creator.AddLodAsset(AZStd::move(lodAsset));
                }
            }

            const ModelMaterialSlotMap &sourceMaterialSlotMap = sourceAsset->GetMaterialSlots();
            for (const auto& sourceMaterialSlot : sourceMaterialSlotMap)
            {
                creator.AddMaterialSlot(sourceMaterialSlot.second);
            }

            return creator.End(clonedResult);
        }
    } // namespace RPI
} // namespace AZ
