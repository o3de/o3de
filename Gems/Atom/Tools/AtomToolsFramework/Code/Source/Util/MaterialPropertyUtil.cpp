/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Util/MaterialPropertyUtil.h>
#include <AtomToolsFramework/DynamicProperty/DynamicProperty.h>

#include <Atom/RPI.Reflect/Image/ImageAsset.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>

#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>

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
        propertyConfig.m_name = propertyDefinition.m_name;
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

        // Update the description for material properties to include script name assuming id is set beforehand
        propertyConfig.m_description = AZStd::string::format(
            "%s%s(Script Name = '%s')",
            propertyConfig.m_description.c_str(),
            propertyConfig.m_description.empty() ? "" : "\n",
            propertyConfig.m_id.GetCStr());
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

    template<typename T>
    bool ComparePropertyValues(const AZStd::any& valueA, const AZStd::any& valueB)
    {
        return valueA.is<T>() && valueB.is<T>() && *AZStd::any_cast<T>(&valueA) == *AZStd::any_cast<T>(&valueB);
    }

    bool ArePropertyValuesEqual(const AZStd::any& valueA, const AZStd::any& valueB)
    {
        if (valueA.type() != valueB.type())
        {
            return false;
        }

        if (ComparePropertyValues<bool>(valueA, valueB) ||
            ComparePropertyValues<int32_t>(valueA, valueB) ||
            ComparePropertyValues<uint32_t>(valueA, valueB) ||
            ComparePropertyValues<float>(valueA, valueB) ||
            ComparePropertyValues<AZ::Vector2>(valueA, valueB) ||
            ComparePropertyValues<AZ::Vector3>(valueA, valueB) ||
            ComparePropertyValues<AZ::Vector4>(valueA, valueB) ||
            ComparePropertyValues<AZ::Color>(valueA, valueB) ||
            ComparePropertyValues<AZ::Data::AssetId>(valueA, valueB) ||
            ComparePropertyValues<AZ::Data::Asset<AZ::Data::AssetData>>(valueA, valueB) ||
            ComparePropertyValues<AZ::Data::Asset<AZ::RPI::ImageAsset>>(valueA, valueB) ||
            ComparePropertyValues<AZ::Data::Asset<AZ::RPI::StreamingImageAsset>>(valueA, valueB) ||
            ComparePropertyValues<AZ::Data::Asset<AZ::RPI::MaterialAsset>>(valueA, valueB) ||
            ComparePropertyValues<AZ::Data::Asset<AZ::RPI::MaterialTypeAsset>>(valueA, valueB) ||
            ComparePropertyValues<AZStd::string>(valueA, valueB))
        {
            return true;
        }

        return false;
    }

    const AtomToolsFramework::DynamicProperty* FindDynamicPropertyForInstanceDataNode(const AzToolsFramework::InstanceDataNode* pNode)
    {
        // Traverse up the hierarchy from the input node to search for an instance corresponding to material inspector property
        for (const AzToolsFramework::InstanceDataNode* currentNode = pNode; currentNode; currentNode = currentNode->GetParent())
        {
            const AZ::SerializeContext* context = currentNode->GetSerializeContext();
            const AZ::SerializeContext::ClassData* classData = currentNode->GetClassMetadata();
            if (context && classData)
            {
                if (context->CanDowncast(classData->m_typeId, azrtti_typeid<AtomToolsFramework::DynamicProperty>(), classData->m_azRtti, nullptr))
                {
                    return static_cast<const AtomToolsFramework::DynamicProperty*>(currentNode->FirstInstance());
                }
            }
        }

        return nullptr;
    }
} // namespace AtomToolsFramework
