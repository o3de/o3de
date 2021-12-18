/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Material/MaterialAssetCreatorCommon.h>
#include <Atom/RPI.Reflect/Material/MaterialAssetCreatorCommon.h>

namespace AZ
{
    namespace RPI
    {
        void MaterialAssetCreatorCommon::OnBegin(
            const MaterialPropertiesLayout* propertyLayout,
            AZStd::vector<MaterialPropertyValue>* propertyValues,
            const AZStd::function<void(const char*)>& warningFunc,
            const AZStd::function<void(const char*)>& errorFunc)
        {
            m_propertyLayout = propertyLayout;
            m_propertyValues = propertyValues;
            m_reportWarning = warningFunc;
            m_reportError = errorFunc;
        }

        void MaterialAssetCreatorCommon::OnEnd()
        {
            m_propertyLayout = nullptr;
            m_propertyValues = nullptr;
            m_reportWarning = nullptr;
            m_reportError = nullptr;
        }

        bool MaterialAssetCreatorCommon::PropertyCheck(TypeId typeId, const Name& name)
        {
            if (!m_reportWarning || !m_reportError)
            {
                AZ_Assert(false, "Call Begin() on the AssetCreator before using it.");
                return false;
            }

            MaterialPropertyIndex propertyIndex = m_propertyLayout->FindPropertyIndex(name);
            if (!propertyIndex.IsValid())
            {
                m_reportWarning(
                    AZStd::string::format("Material property '%s' not found",
                    name.GetCStr()
                    ).data());
                return false;
            }

            const MaterialPropertyDescriptor* materialPropertyDescriptor = m_propertyLayout->GetPropertyDescriptor(propertyIndex);
            if (!materialPropertyDescriptor)
            {
                m_reportError("A material property index was found but the property descriptor was null");
                return false;
            }

            if (!ValidateMaterialPropertyDataType(typeId, name, materialPropertyDescriptor, m_reportError))
            {
                return false;
            }

            return true;
        }

        void MaterialAssetCreatorCommon::SetPropertyValue(const Name& name, const Data::Asset<ImageAsset>& imageAsset)
        {
            return SetPropertyValue(name, MaterialPropertyValue(imageAsset));
        }

        void MaterialAssetCreatorCommon::SetPropertyValue(const Name& name, const MaterialPropertyValue& value)
        {
            if (PropertyCheck(value.GetTypeId(), name))
            {
                MaterialPropertyIndex propertyIndex = m_propertyLayout->FindPropertyIndex(name);
                (*m_propertyValues)[propertyIndex.GetIndex()] = value;
            }
        }

        void MaterialAssetCreatorCommon::SetPropertyValue(const Name& name, const Data::Asset<StreamingImageAsset>& imageAsset)
        {
            SetPropertyValue(name, Data::Asset<ImageAsset>(imageAsset));
        }

        void MaterialAssetCreatorCommon::SetPropertyValue(const Name& name, const Data::Asset<AttachmentImageAsset>& imageAsset)
        {
            SetPropertyValue(name, Data::Asset<ImageAsset>(imageAsset));
        }
    } // namespace RPI
} // namespace AZ
