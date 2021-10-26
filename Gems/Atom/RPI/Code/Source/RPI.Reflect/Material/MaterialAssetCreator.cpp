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
        void MaterialAssetCreator::Begin(const Data::AssetId& assetId, MaterialAsset& parentMaterial, bool includeMaterialPropertyNames)
        {
            BeginCommon(assetId);
            
            if (ValidateIsReady())
            {
                m_asset->m_materialTypeAsset = parentMaterial.m_materialTypeAsset;
                m_asset->m_materialTypeVersion = m_asset->m_materialTypeAsset->GetVersion();

                if (!m_asset->m_materialTypeAsset)
                {
                    ReportError("MaterialTypeAsset is null");
                    return;
                }

                m_materialPropertiesLayout = m_asset->GetMaterialPropertiesLayout();
                if (!m_materialPropertiesLayout)
                {
                    ReportError("MaterialPropertiesLayout is null");
                    return;
                }
                if (includeMaterialPropertyNames)
                {
                    PopulatePropertyNameList();
                }

                // Note we don't have to check the validity of these property values because the parent material's AssetCreator already did that.
                m_asset->m_propertyValues.assign(parentMaterial.GetPropertyValues().begin(), parentMaterial.GetPropertyValues().end());

                auto warningFunc = [this](const char* message)
                {
                    ReportWarning("%s", message);
                };
                auto errorFunc = [this](const char* message)
                {
                    ReportError("%s", message);
                };
                MaterialAssetCreatorCommon::OnBegin(m_materialPropertiesLayout, &(m_asset->m_propertyValues), warningFunc, errorFunc);
            }
        }

        void MaterialAssetCreator::Begin(const Data::AssetId& assetId, MaterialTypeAsset& materialType, bool includeMaterialPropertyNames)
        {
            BeginCommon(assetId);

            if (ValidateIsReady())
            {
                m_asset->m_materialTypeAsset = { &materialType, AZ::Data::AssetLoadBehavior::PreLoad };

                if (!m_asset->m_materialTypeAsset)
                {
                    ReportError("MaterialTypeAsset is null");
                    return;
                }
                m_asset->m_materialTypeVersion = m_asset->m_materialTypeAsset->GetVersion();

                m_materialPropertiesLayout = m_asset->GetMaterialPropertiesLayout();
                if (includeMaterialPropertyNames)
                {
                    PopulatePropertyNameList();
                }

                if (!m_materialPropertiesLayout)
                {
                    ReportError("MaterialPropertiesLayout is null");
                    return;
                }

                // Note we don't have to check the validity of these property values because the parent material's AssetCreator already did that.
                m_asset->m_propertyValues.assign(materialType.GetDefaultPropertyValues().begin(), materialType.GetDefaultPropertyValues().end());

                auto warningFunc = [this](const char* message)
                {
                    ReportWarning("%s", message);
                };
                auto errorFunc = [this](const char* message)
                {
                    ReportError("%s", message);
                };
                MaterialAssetCreatorCommon::OnBegin(m_materialPropertiesLayout, &(m_asset->m_propertyValues), warningFunc, errorFunc);
            }
        }

        bool MaterialAssetCreator::End(Data::Asset<MaterialAsset>& result)
        {
            if (!ValidateIsReady())
            {
                return false;
            }

            m_materialPropertiesLayout = nullptr;
            MaterialAssetCreatorCommon::OnEnd();

            m_asset->SetReady();
            return EndCommon(result);
        }

        void MaterialAssetCreator::PopulatePropertyNameList()
        {
            for (int i = 0; i < m_materialPropertiesLayout->GetPropertyCount(); ++i)
            {
                MaterialPropertyIndex propertyIndex{ i };
                auto& propertyName = m_materialPropertiesLayout->GetPropertyDescriptor(propertyIndex)->GetName();
                m_asset->m_propertyNames.emplace_back(propertyName);
            }
        }

    } // namespace RPI
} // namespace AZ
