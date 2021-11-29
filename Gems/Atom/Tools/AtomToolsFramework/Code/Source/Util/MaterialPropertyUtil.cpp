/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/DynamicProperty/DynamicProperty.h>
#include <AtomToolsFramework/Util/MaterialPropertyUtil.h>

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Reflect/Image/ImageAsset.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>

#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
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

    bool ConvertToExportFormat(
        const AZStd::string& exportPath,
        [[maybe_unused]] const AZ::Name& propertyId,
        const AZ::RPI::MaterialTypeSourceData::PropertyDefinition& propertyDefinition,
        AZ::RPI::MaterialPropertyValue& propertyValue)
    {
        if (propertyDefinition.m_dataType == AZ::RPI::MaterialPropertyDataType::Enum && propertyValue.Is<uint32_t>())
        {
            const uint32_t index = propertyValue.GetValue<uint32_t>();
            if (index >= propertyDefinition.m_enumValues.size())
            {
                AZ_Error("AtomToolsFramework", false, "Invalid value for material enum property: '%s'.", propertyId.GetCStr());
                return false;
            }

            propertyValue = propertyDefinition.m_enumValues[index];
            return true;
        }

        // Image asset references must be converted from asset IDs to a relative source file path
        if (propertyDefinition.m_dataType == AZ::RPI::MaterialPropertyDataType::Image)
        {
            AZStd::string imagePath;
            AZ::Data::AssetId imageAssetId;

            if (propertyValue.Is<AZ::Data::Asset<AZ::RPI::ImageAsset>>())
            {
                const auto& imageAsset = propertyValue.GetValue<AZ::Data::Asset<AZ::RPI::ImageAsset>>();
                imageAssetId = imageAsset.GetId();
            }

            if (propertyValue.Is<AZ::Data::Instance<AZ::RPI::Image>>())
            {
                const auto& image = propertyValue.GetValue<AZ::Data::Instance<AZ::RPI::Image>>();
                if (image)
                {
                    imageAssetId = image->GetAssetId();
                }
            }
            
            imagePath = AZ::RPI::AssetUtils::GetSourcePathByAssetId(imageAssetId);

            if (imageAssetId.IsValid() && imagePath.empty())
            {
                AZ_Error("AtomToolsFramework", false, "Image asset could not be found for property: '%s'.", propertyId.GetCStr());
                return false;
            }
            else
            {
                propertyValue = GetExteralReferencePath(exportPath, imagePath);
                return true;
            }
        }

        return true;
    }

    AZStd::string GetExteralReferencePath(const AZStd::string& exportPath, const AZStd::string& referencePath, const uint32_t maxPathDepth)
    {
        if (referencePath.empty())
        {
            return {};
        }

        AZ::IO::BasicPath<AZStd::string> exportFolder(exportPath);
        exportFolder.RemoveFilename();

        const AZStd::string relativePath = AZ::IO::PathView(referencePath).LexicallyRelative(exportFolder).StringAsPosix();

        // Count the difference in depth between the export file path and the referenced file path.
        uint32_t parentFolderCount = 0;
        AZStd::string::size_type pos = 0;
        const AZStd::string parentFolderToken = "..";
        while ((pos = relativePath.find(parentFolderToken, pos)) != AZStd::string::npos)
        {
            parentFolderCount++;
            pos += parentFolderToken.length();
        }

        // If the difference in depth is too great then revert to using the asset folder relative path.
        // We could change this to only use relative paths for references in subfolders.
        if (parentFolderCount > maxPathDepth)
        {
            AZStd::string watchFolder;
            AZ::Data::AssetInfo assetInfo;
            bool sourceInfoFound = false;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                sourceInfoFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, referencePath.c_str(),
                assetInfo, watchFolder);
            if (sourceInfoFound)
            {
                return assetInfo.m_relativePath;
            }
        }

        return relativePath;
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
                if (context->CanDowncast(
                        classData->m_typeId, azrtti_typeid<AtomToolsFramework::DynamicProperty>(), classData->m_azRtti, nullptr))
                {
                    return static_cast<const AtomToolsFramework::DynamicProperty*>(currentNode->FirstInstance());
                }
            }
        }

        return nullptr;
    }
} // namespace AtomToolsFramework
