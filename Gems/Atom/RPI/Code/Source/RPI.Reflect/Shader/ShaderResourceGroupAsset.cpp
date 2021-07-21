/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RPI.Reflect/Shader/ShaderResourceGroupAsset.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        const char* ShaderResourceGroupAsset::DisplayName = "ShaderResourceGroup";
        const char* ShaderResourceGroupAsset::Group = "";
        const char* ShaderResourceGroupAsset::Extension = ".azsrg";

        void ShaderResourceGroupAsset::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderResourceGroupAsset>()
                    ->Version(0)
                    ->Field("m_name", &ShaderResourceGroupAsset::m_name)
                    ->Field("m_perAPILayout", &ShaderResourceGroupAsset::m_perAPILayout)
                    ;
            }
        }

        const Name& ShaderResourceGroupAsset::GetName() const
        {
            return m_name;
        }

        const RHI::ShaderResourceGroupLayout* ShaderResourceGroupAsset::GetLayout() const
        {
            AZ_Error("RHI::ShaderResourceGroupLayout", m_currentAPITypeIndex < m_perAPILayout.size(), "Invalid API Type index");
            if (m_currentAPITypeIndex >= m_perAPILayout.size())
            {
                return nullptr;
            }
            return m_perAPILayout[m_currentAPITypeIndex].second.get();
        }

        const RHI::ShaderResourceGroupLayout * ShaderResourceGroupAsset::GetLayout(const RHI::APIType type) const
        {
            size_t index = FindAPITypeIndex(type);
            AZ_Assert(index < m_perAPILayout.size(), "Invalid API Type index");
            return m_perAPILayout[index].second.get();
        }

        bool ShaderResourceGroupAsset::IsValid() const
        {
            return !m_perAPILayout.empty() && !m_name.IsEmpty();
        }

        void ShaderResourceGroupAsset::SetReady()
        {
            m_status = AssetStatus::Ready;
        }

        bool ShaderResourceGroupAsset::FinalizeAfterLoad()
        {
            if (RHI::Factory::IsReady())
            {
                auto rhiType = RHI::Factory::Get().GetType();
                m_currentAPITypeIndex = FindAPITypeIndex(rhiType);
                if (m_currentAPITypeIndex >= m_perAPILayout.size())
                {
                    AZ_Assert(false, "Could not find shader resource group layout for API %s", RHI::Factory::Get().GetName().GetCStr());
                    return false;
                }
            }
            else
            {
                m_currentAPITypeIndex = 0;
            }

            return true;
        }

        size_t ShaderResourceGroupAsset::FindAPITypeIndex(RHI::APIType type) const
        {
            auto findIt = AZStd::find_if(m_perAPILayout.begin(), m_perAPILayout.end(), [&type](const auto& shaderResourceGroupLayoutData)
            {
                return shaderResourceGroupLayoutData.first == type;
            });

            return findIt != m_perAPILayout.end() ? AZStd::distance(m_perAPILayout.begin(), findIt) : InvalidAPITypeIndex;
        }

        Data::AssetHandler::LoadResult ShaderResourceGroupAssetHandler::LoadAssetData(
            const Data::Asset<Data::AssetData>& asset,
            AZStd::shared_ptr<Data::AssetDataStream> stream,
            const Data::AssetFilterCB& assetLoadFilterCB)
        {
            if (Base::LoadAssetData(asset, stream, assetLoadFilterCB) == Data::AssetHandler::LoadResult::LoadComplete)
            {
                return PostLoadInit(asset);
            }
            return Data::AssetHandler::LoadResult::Error;
        }

        Data::AssetHandler::LoadResult ShaderResourceGroupAssetHandler::PostLoadInit(const Data::Asset<Data::AssetData>& asset)
        {
            if (ShaderResourceGroupAsset* shaderAsset = asset.GetAs<ShaderResourceGroupAsset>())
            {
                if (!shaderAsset->FinalizeAfterLoad())
                {
                    AZ_Error("ShaderAssetHandler", false, "Shader asset failed to finalize.");
                    return Data::AssetHandler::LoadResult::Error;
                }
                return Data::AssetHandler::LoadResult::LoadComplete;
            }
            return Data::AssetHandler::LoadResult::Error;
        }

    } // namespace RPI
} // namespace AZ
