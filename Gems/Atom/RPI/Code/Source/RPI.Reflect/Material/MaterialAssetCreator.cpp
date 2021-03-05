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

#include <Atom/RPI.Reflect/Material/MaterialAssetCreator.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/Image/AttachmentImageAsset.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/bind/bind.h>

namespace AZ
{
    namespace RPI
    {
        void MaterialAssetCreator::Begin(const Data::AssetId& assetId, MaterialAsset& parentMaterial)
        {
            BeginCommon(assetId);
            
            if (ValidateIsReady())
            {
                m_asset->m_materialTypeAsset = parentMaterial.m_materialTypeAsset;

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

        void MaterialAssetCreator::Begin(const Data::AssetId& assetId, MaterialTypeAsset& materialType)
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

                m_materialPropertiesLayout = m_asset->GetMaterialPropertiesLayout();
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
    } // namespace RPI
} // namespace AZ
