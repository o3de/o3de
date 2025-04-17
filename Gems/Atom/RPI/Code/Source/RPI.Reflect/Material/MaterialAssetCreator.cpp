/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Material/MaterialAssetCreator.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/Image/AttachmentImageAsset.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/bind/bind.h>

namespace AZ
{
    namespace RPI
    {
        void MaterialAssetCreator::Begin(const Data::AssetId& assetId, const Data::Asset<MaterialTypeAsset>& materialType)
        {
            BeginCommon(assetId);

            if (ValidateIsReady())
            {
                m_asset->m_materialTypeAsset = materialType;
                m_asset->m_materialTypeAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::PreLoad);
                
                if (!m_asset->m_materialTypeAsset)
                {
                    ReportError("MaterialTypeAsset is null, the MaterialAsset cannot be finalized");
                }
            }
        }
        
        bool MaterialAssetCreator::End(Data::Asset<MaterialAsset>& result)
        {
            if (!ValidateIsReady())
            {
                return false;
            }

            m_asset->Finalize(
                [this](const char* message) { ReportWarning("%s", message); },
                [this](const char* message) { ReportError("%s", message); });

            // Finalize() doesn't clear the raw property data because that's the same function used at runtime, which does need to maintain the raw data
            // to support hot reload. But here we are pre-baking with the assumption that AP build dependencies will keep the material type
            // and material asset in sync, so we can discard the raw property data and just rely on the data in the material type asset.
            m_asset->m_rawPropertyValues.clear();

            m_asset->SetReady();

            return EndCommon(result);
        }
        
        void MaterialAssetCreator::SetMaterialTypeVersion(uint32_t version)
        {
            if (ValidateIsReady())
            {
                m_asset->m_materialTypeVersion = version;
            }
        }
        
        void MaterialAssetCreator::SetPropertyValue(const Name& name, const MaterialPropertyValue& value)
        {
            if (ValidateIsReady())
            {
                // Here we are careful to keep the properties in the same order they were encountered. When the MaterialAsset
                // is later finalized with a MaterialTypeAsset, there could be a version update procedure that includes renamed
                // properties. So it's possible that the same property could be encountered twice but with two different names.
                // Preserving the original order will ensure that the later properties still overwrite the earlier ones even after
                // renames have been applied.

                AZStd::erase_if(
                    m_asset->m_rawPropertyValues,
                    [&name](const AZStd::pair<Name, MaterialPropertyValue>& pair)
                    {
                        return pair.first == name;
                    });

                m_asset->m_rawPropertyValues.emplace_back(name, value);
            }
        }
        
        void MaterialAssetCreator::SetPropertyValue(const Name& name, const Data::Asset<ImageAsset>& imageAsset)
        {
            SetPropertyValue(name, MaterialPropertyValue{imageAsset});
        }

        void MaterialAssetCreator::SetPropertyValue(const Name& name, const Data::Asset<StreamingImageAsset>& imageAsset)
        {
            SetPropertyValue(name, Data::Asset<ImageAsset>(imageAsset));
        }

        void MaterialAssetCreator::SetPropertyValue(const Name& name, const Data::Asset<AttachmentImageAsset>& imageAsset)
        {
            SetPropertyValue(name, Data::Asset<ImageAsset>(imageAsset));
        }

    } // namespace RPI
} // namespace AZ
