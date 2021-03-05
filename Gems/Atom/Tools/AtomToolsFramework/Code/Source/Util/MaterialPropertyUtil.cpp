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

#include <AtomToolsFramework/Util/MaterialPropertyUtil.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

namespace AtomToolsFramework
{
    AZ::RPI::MaterialPropertyValue ConvertToRuntimeType(const AZStd::any& value)
    {
        return AZ::RPI::MaterialPropertyValue::FromAny(value);
    }

    AZStd::any ConvertToEditableType(const AZ::RPI::MaterialPropertyValue& value)
    {
        if (value.Is<AZ::Data::Asset<AZ::RPI::ImageAsset>>())
        {
            const AZ::Data::Asset<AZ::RPI::ImageAsset>& imageAsset = value.GetValue<AZ::Data::Asset<AZ::RPI::ImageAsset>>();
            return AZStd::any(AZ::Data::Asset<AZ::RPI::StreamingImageAsset>(
                imageAsset.GetId(),
                azrtti_typeid<AZ::RPI::StreamingImageAsset>(),
                imageAsset.GetHint()));
        }

        return AZ::RPI::MaterialPropertyValue::ToAny(value);
    }

    AtomToolsFramework::DynamicPropertyType ConvertToEditableType(const AZ::RPI::MaterialPropertyDataType dataType)
    {
        switch (dataType)
        {
        case AZ::RPI::MaterialPropertyDataType::Bool:
            return AtomToolsFramework::DynamicPropertyType::Bool;
        case AZ::RPI::MaterialPropertyDataType::Int:
            return AtomToolsFramework::DynamicPropertyType::Int;
        case AZ::RPI::MaterialPropertyDataType::UInt:
            return AtomToolsFramework::DynamicPropertyType::UInt;
        case AZ::RPI::MaterialPropertyDataType::Float:
            return AtomToolsFramework::DynamicPropertyType::Float;
        case AZ::RPI::MaterialPropertyDataType::Vector2:
            return AtomToolsFramework::DynamicPropertyType::Vector2;
        case AZ::RPI::MaterialPropertyDataType::Vector3:
            return AtomToolsFramework::DynamicPropertyType::Vector3;
        case AZ::RPI::MaterialPropertyDataType::Vector4:
            return AtomToolsFramework::DynamicPropertyType::Vector4;
        case AZ::RPI::MaterialPropertyDataType::Color:
            return AtomToolsFramework::DynamicPropertyType::Color;
        case AZ::RPI::MaterialPropertyDataType::Image:
            return AtomToolsFramework::DynamicPropertyType::Asset;
        case AZ::RPI::MaterialPropertyDataType::Enum:
            return AtomToolsFramework::DynamicPropertyType::Enum;
        }

        AZ_Assert(false, "Attempting to convert an unsupported property type.");
        return AtomToolsFramework::DynamicPropertyType::Invalid;
    }

    void ConvertToPropertyConfig(AtomToolsFramework::DynamicPropertyConfig& propertyConfig, const AZ::RPI::MaterialTypeSourceData::PropertyDefinition& propertyDefinition)
    {
        propertyConfig.m_dataType = ConvertToEditableType(propertyDefinition.m_dataType);
        propertyConfig.m_nameId = propertyDefinition.m_nameId;
        propertyConfig.m_displayName = propertyDefinition.m_displayName;
        propertyConfig.m_description = propertyDefinition.m_description;
        propertyConfig.m_defaultValue = ConvertToEditableType(propertyDefinition.m_value);
        propertyConfig.m_min = ConvertToEditableType(propertyDefinition.m_min);
        propertyConfig.m_max = ConvertToEditableType(propertyDefinition.m_max);
        propertyConfig.m_softMin = ConvertToEditableType(propertyDefinition.m_softMin);
        propertyConfig.m_softMax = ConvertToEditableType(propertyDefinition.m_softMax);
        propertyConfig.m_step = ConvertToEditableType(propertyDefinition.m_step);
        propertyConfig.m_enumValues = propertyDefinition.m_enumValues;
        propertyConfig.m_vectorLabels = propertyDefinition.m_vectorLabels;
        propertyConfig.m_visible = propertyDefinition.m_visibility != AZ::RPI::MaterialPropertyVisibility::Hidden;
        propertyConfig.m_readOnly = propertyDefinition.m_visibility == AZ::RPI::MaterialPropertyVisibility::Disabled;
    }

    void ConvertToPropertyConfig(AtomToolsFramework::DynamicPropertyConfig& propertyConfig, const AZ::RPI::MaterialPropertyDynamicMetadata& propertyMetaData)
    {
        propertyConfig.m_description = propertyMetaData.m_description;
        propertyConfig.m_min = ConvertToEditableType(propertyMetaData.m_propertyRange.m_min);
        propertyConfig.m_max = ConvertToEditableType(propertyMetaData.m_propertyRange.m_max);
        propertyConfig.m_softMin = ConvertToEditableType(propertyMetaData.m_propertyRange.m_softMin);
        propertyConfig.m_softMax = ConvertToEditableType(propertyMetaData.m_propertyRange.m_softMax);
        propertyConfig.m_visible = propertyMetaData.m_visibility != AZ::RPI::MaterialPropertyVisibility::Hidden;
        propertyConfig.m_readOnly = propertyMetaData.m_visibility == AZ::RPI::MaterialPropertyVisibility::Disabled;
    }

    void ConvertToPropertyMetaData(AZ::RPI::MaterialPropertyDynamicMetadata& propertyMetaData, const AtomToolsFramework::DynamicPropertyConfig& propertyConfig)
    {
        propertyMetaData.m_description = propertyConfig.m_description;
        propertyMetaData.m_propertyRange.m_min = ConvertToRuntimeType(propertyConfig.m_min);
        propertyMetaData.m_propertyRange.m_max = ConvertToRuntimeType(propertyConfig.m_max);
        propertyMetaData.m_propertyRange.m_softMin = ConvertToRuntimeType(propertyConfig.m_softMin);
        propertyMetaData.m_propertyRange.m_softMax = ConvertToRuntimeType(propertyConfig.m_softMax);
        if (!propertyConfig.m_visible)
        {
            propertyMetaData.m_visibility = AZ::RPI::MaterialPropertyVisibility::Hidden;
        }
        else if (propertyConfig.m_readOnly)
        {
            propertyMetaData.m_visibility = AZ::RPI::MaterialPropertyVisibility::Disabled;
        }
        else
        {
            propertyMetaData.m_visibility = AZ::RPI::MaterialPropertyVisibility::Enabled;
        }
    }
} // namespace AtomToolsFramework
