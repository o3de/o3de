/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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

        MaterialPropertyDataType MaterialAssetCreatorCommon::GetMaterialPropertyDataType(TypeId typeId) const
        {
            if (typeId == azrtti_typeid<bool>()) { return MaterialPropertyDataType::Bool; }
            if (typeId == azrtti_typeid<int32_t>()) { return MaterialPropertyDataType::Int; }
            if (typeId == azrtti_typeid<uint32_t>()) { return MaterialPropertyDataType::UInt; }
            if (typeId == azrtti_typeid<float>()) { return MaterialPropertyDataType::Float; }
            if (typeId == azrtti_typeid<Vector2>()) { return MaterialPropertyDataType::Vector2; }
            if (typeId == azrtti_typeid<Vector3>()) { return MaterialPropertyDataType::Vector3; }
            if (typeId == azrtti_typeid<Vector4>()) { return MaterialPropertyDataType::Vector4; }
            if (typeId == azrtti_typeid<Color>()) { return MaterialPropertyDataType::Color; }
            if (typeId == azrtti_typeid<Data::Asset<ImageAsset>>()) { return MaterialPropertyDataType::Image; }
            else
            {
                return MaterialPropertyDataType::Invalid;
            }
        }

        bool MaterialAssetCreatorCommon::ValidateDataType(TypeId typeId, const Name& propertyName, const MaterialPropertyDescriptor* materialPropertyDescriptor)
        {
            auto expectedDataType = materialPropertyDescriptor->GetDataType();
            auto actualDataType = GetMaterialPropertyDataType(typeId);

            if (expectedDataType == MaterialPropertyDataType::Enum)
            {
                if (actualDataType != MaterialPropertyDataType::UInt)
                {
                    m_reportError(
                        AZStd::string::format("Material property '%s' is a Enum type, can only accept UInt value, input value is %s",
                            propertyName.GetCStr(),
                            ToString(actualDataType)
                        ).data());
                    return false;
                }
            }
            else
            {
                if (expectedDataType != actualDataType)
                {
                    m_reportError(
                        AZStd::string::format("Material property '%s': Type mismatch. Expected %s but was %s",
                            propertyName.GetCStr(),
                            ToString(expectedDataType),
                            ToString(actualDataType)
                        ).data());
                    return false;
                }
            }

            return true;
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

            if (!ValidateDataType(typeId, name, materialPropertyDescriptor))
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
