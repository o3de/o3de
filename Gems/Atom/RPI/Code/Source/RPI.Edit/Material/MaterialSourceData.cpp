/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyValueSerializer.h>
#include <Atom/RPI.Edit/Material/MaterialSourceDataSerializer.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyId.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <Atom/RPI.Edit/Material/MaterialConverterBus.h>

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Common/JsonFileLoadContext.h>
#include <Atom/RPI.Edit/Common/JsonReportingHelper.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>

#include <Atom/RPI.Reflect/Material/MaterialAssetCreator.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>

#include <AzCore/Serialization/Json/JsonUtils.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/IO/TextStreamWriters.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

namespace AZ
{
    namespace RPI
    {
        void MaterialSourceData::Reflect(ReflectContext* context)
        {
            if (JsonRegistrationContext* jsonContext = azrtti_cast<JsonRegistrationContext*>(context))
            {
                jsonContext->Serializer<JsonMaterialSourceDataSerializer>()->HandlesType<MaterialSourceData>();
                jsonContext->Serializer<JsonMaterialPropertyValueSerializer>()->HandlesType<MaterialSourceData::Property>();
            }
            else if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<MaterialSourceData>()
                    ->Version(1)
                    ;

                serializeContext->RegisterGenericType<PropertyMap>();
                serializeContext->RegisterGenericType<PropertyGroupMap>();

                serializeContext->Class<MaterialSourceData::Property>()
                    ->Version(1)
                    ;
            }
        }

        // Helper function for CreateMaterialAsset, for applying basic material property values
        template<typename T>
        void ApplyMaterialValues(MaterialAssetCreator& materialAssetCreator, const AZStd::map<Name, T>& values)
        {
            for (auto& entry : values)
            {
                const Name& propertyId = entry.first;
                materialAssetCreator.SetPropertyValue(propertyId, entry.second);
            }
        }
        
        MaterialSourceData::ApplyVersionUpdatesResult MaterialSourceData::ApplyVersionUpdates(AZStd::string_view materialSourceFilePath)
        {
            AZStd::string materialTypeFullPath = AssetUtils::ResolvePathReference(materialSourceFilePath, m_materialType);
            auto materialTypeSourceDataOutcome = MaterialUtils::LoadMaterialTypeSourceData(materialTypeFullPath);
            if (!materialTypeSourceDataOutcome.IsSuccess())
            {
                return ApplyVersionUpdatesResult::Failed;
            }
            
            MaterialTypeSourceData materialTypeSourceData = materialTypeSourceDataOutcome.TakeValue();

            if (m_materialTypeVersion == materialTypeSourceData.m_version)
            {
                return ApplyVersionUpdatesResult::NoUpdates;
            }

            bool changesWereApplied = false;

            // Note that the only kind of property update currently supported is rename...

            PropertyGroupMap newPropertyGroups;
            for (auto& groupPair : m_properties)
            {
                PropertyMap& propertyMap = groupPair.second;

                for (auto& propertyPair : propertyMap)
                {
                    MaterialPropertyId propertyId{groupPair.first, propertyPair.first};

                    if (materialTypeSourceData.ApplyPropertyRenames(propertyId, m_materialTypeVersion))
                    {
                        changesWereApplied = true;
                    }
                    
                    newPropertyGroups[propertyId.GetGroupName().GetStringView()][propertyId.GetPropertyName().GetStringView()] = propertyPair.second;
                }
            }

            if (changesWereApplied)
            {
                m_properties = AZStd::move(newPropertyGroups);

                AZ_Warning(
                    "MaterialSourceData", false,
                    "This material is based on version '%u' of '%s', but the material type is now at version '%u'. "
                    "Automatic updates are available. Consider updating the .material source file: '%s'.",
                    m_materialTypeVersion, materialTypeFullPath.c_str(), materialTypeSourceData.m_version, materialSourceFilePath.data());
            }

            m_materialTypeVersion = materialTypeSourceData.m_version;
            
            return changesWereApplied ? ApplyVersionUpdatesResult::UpdatesApplied : ApplyVersionUpdatesResult::NoUpdates;
        }

        Outcome<Data::Asset<MaterialAsset>> MaterialSourceData::CreateMaterialAsset(
            Data::AssetId assetId, AZStd::string_view materialSourceFilePath, bool elevateWarnings, bool includeMaterialPropertyNames) const
        {
            MaterialAssetCreator materialAssetCreator;
            materialAssetCreator.SetElevateWarnings(elevateWarnings);

            if (m_parentMaterial.empty())
            {
                auto materialTypeAsset = AssetUtils::LoadAsset<MaterialTypeAsset>(materialSourceFilePath, m_materialType);
                if (!materialTypeAsset.IsSuccess())
                {
                    return Failure();
                }

                materialAssetCreator.Begin(assetId, *materialTypeAsset.GetValue().Get(), includeMaterialPropertyNames);
            }
            else
            {
                auto parentMaterialAsset = AssetUtils::LoadAsset<MaterialAsset>(materialSourceFilePath, m_parentMaterial);
                if (!parentMaterialAsset.IsSuccess())
                {
                    return Failure();
                }

                // Make sure the parent material has the same material type
                {
                    auto materialTypeIdOutcome = AssetUtils::MakeAssetId(materialSourceFilePath, m_materialType, 0);
                    if (!materialTypeIdOutcome.IsSuccess())
                    {
                        return Failure();
                    }

                    Data::AssetId expectedMaterialTypeId = materialTypeIdOutcome.GetValue();
                    Data::AssetId parentMaterialId = parentMaterialAsset.GetValue().GetId();
                    // This will only be valid if the parent material is not a material type
                    Data::AssetId parentsMaterialTypeId = parentMaterialAsset.GetValue()->GetMaterialTypeAsset().GetId();

                    if (expectedMaterialTypeId != parentMaterialId && expectedMaterialTypeId != parentsMaterialTypeId)
                    {
                        AZ_Error("MaterialSourceData", false, "This material and its parent material do not share the same material type.");
                        return Failure();
                    }
                }

                materialAssetCreator.Begin(assetId, *parentMaterialAsset.GetValue().Get(), includeMaterialPropertyNames);
            }

            ApplyPropertiesToAssetCreator(materialAssetCreator, materialSourceFilePath);

            Data::Asset<MaterialAsset> material;
            if (materialAssetCreator.End(material))
            {
                return Success(material);
            }
            else
            {
                return Failure();
            }
        }

        Outcome<Data::Asset<MaterialAsset>> MaterialSourceData::CreateMaterialAssetFromSourceData(
            Data::AssetId assetId,
            AZStd::string_view materialSourceFilePath,
            bool elevateWarnings,
            bool includeMaterialPropertyNames,
            AZStd::unordered_set<AZStd::string>* sourceDependencies) const
        {
            const auto materialTypeSourcePath = AssetUtils::ResolvePathReference(materialSourceFilePath, m_materialType);
            const auto materialTypeAssetId = AssetUtils::MakeAssetId(materialTypeSourcePath, 0);
            if (!materialTypeAssetId.IsSuccess())
            {
                AZ_Error("MaterialSourceData", false, "Failed to create material type asset ID: '%s'.", materialTypeSourcePath.c_str());
                return Failure();
            }

            MaterialTypeSourceData materialTypeSourceData;
            if (!AZ::RPI::JsonUtils::LoadObjectFromFile(materialTypeSourcePath, materialTypeSourceData))
            {
                AZ_Error("MaterialSourceData", false, "Failed to load MaterialTypeSourceData: '%s'.", materialTypeSourcePath.c_str());
                return Failure();
            }

            materialTypeSourceData.ResolveUvEnums();

            const auto materialTypeAsset =
                materialTypeSourceData.CreateMaterialTypeAsset(materialTypeAssetId.GetValue(), materialTypeSourcePath, elevateWarnings);
            if (!materialTypeAsset.IsSuccess())
            {
                AZ_Error("MaterialSourceData", false, "Failed to create material type asset from source data: '%s'.", materialTypeSourcePath.c_str());
                return Failure();
            }

            // Track all of the material and material type assets loaded while trying to create a material asset from source data. This will
            // be used for evaluating circular dependencies and returned for external monitoring or other use.
            AZStd::unordered_set<AZStd::string> dependencies;
            dependencies.insert(materialSourceFilePath);
            dependencies.insert(materialTypeSourcePath);

            // Load and build a stack of MaterialSourceData from all of the parent materials in the hierarchy. Properties from the source
            // data will be applied in reverse to the asset creator.
            AZStd::vector<MaterialSourceData> parentSourceDataStack;

            AZStd::string parentSourceRelPath = m_parentMaterial;
            AZStd::string parentSourceAbsPath = AssetUtils::ResolvePathReference(materialSourceFilePath, parentSourceRelPath);
            while (!parentSourceRelPath.empty())
            {
                if (!dependencies.insert(parentSourceAbsPath).second)
                {
                    AZ_Error("MaterialSourceData", false, "Detected circular dependency between materials: '%s' and '%s'.", materialSourceFilePath.data(), parentSourceAbsPath.c_str());
                    return Failure();
                }

                MaterialSourceData parentSourceData;
                if (!AZ::RPI::JsonUtils::LoadObjectFromFile(parentSourceAbsPath, parentSourceData))
                {
                    AZ_Error("MaterialSourceData", false, "Failed to load MaterialSourceData for parent material: '%s'.", parentSourceAbsPath.c_str());
                    return Failure();
                }

                // Make sure that all materials in the hierarchy share the same material type
                const auto parentTypeAssetId = AssetUtils::MakeAssetId(parentSourceAbsPath, parentSourceData.m_materialType, 0);
                if (!parentTypeAssetId)
                {
                    AZ_Error("MaterialSourceData", false, "Parent material asset ID wasn't found: '%s'.", parentSourceAbsPath.c_str());
                    return Failure();
                }

                if (parentTypeAssetId.GetValue() != materialTypeAssetId.GetValue())
                {
                    AZ_Error("MaterialSourceData", false, "This material and its parent material do not share the same material type.");
                    return Failure();
                }

                // Get the location of the next parent material and push the source data onto the stack 
                parentSourceRelPath = parentSourceData.m_parentMaterial;
                parentSourceAbsPath = AssetUtils::ResolvePathReference(parentSourceAbsPath, parentSourceRelPath);
                parentSourceDataStack.emplace_back(AZStd::move(parentSourceData));
            }

            // Create the material asset from all the previously loaded source data 
            MaterialAssetCreator materialAssetCreator;
            materialAssetCreator.SetElevateWarnings(elevateWarnings);
            materialAssetCreator.Begin(assetId, *materialTypeAsset.GetValue().Get(), includeMaterialPropertyNames);

            while (!parentSourceDataStack.empty())
            {
                parentSourceDataStack.back().ApplyPropertiesToAssetCreator(materialAssetCreator, materialSourceFilePath);
                parentSourceDataStack.pop_back();
            }

            ApplyPropertiesToAssetCreator(materialAssetCreator, materialSourceFilePath);

            Data::Asset<MaterialAsset> material;
            if (materialAssetCreator.End(material))
            {
                if (sourceDependencies)
                {
                    sourceDependencies->insert(dependencies.begin(), dependencies.end());
                }

                return Success(material);
            }

            return Failure();
        }

        void MaterialSourceData::ApplyPropertiesToAssetCreator(
            AZ::RPI::MaterialAssetCreator& materialAssetCreator, const AZStd::string_view& materialSourceFilePath) const
        {
            for (auto& group : m_properties)
            {
                for (auto& property : group.second)
                {
                    MaterialPropertyId propertyId{ group.first, property.first };
                    if (!property.second.m_value.IsValid())
                    {
                        materialAssetCreator.ReportWarning("Source data for material property value is invalid.");
                    }
                    else
                    {
                        MaterialPropertyIndex propertyIndex =
                            materialAssetCreator.m_materialPropertiesLayout->FindPropertyIndex(propertyId.GetFullName());
                        if (propertyIndex.IsValid())
                        {
                            const MaterialPropertyDescriptor* propertyDescriptor =
                                materialAssetCreator.m_materialPropertiesLayout->GetPropertyDescriptor(propertyIndex);
                            switch (propertyDescriptor->GetDataType())
                            {
                            case MaterialPropertyDataType::Image:
                                {
                                    Data::Asset<ImageAsset> imageAsset;

                                    MaterialUtils::GetImageAssetResult result = MaterialUtils::GetImageAssetReference(
                                        imageAsset, materialSourceFilePath, property.second.m_value.GetValue<AZStd::string>());
                                    
                                    if (result == MaterialUtils::GetImageAssetResult::Missing)
                                    {
                                        materialAssetCreator.ReportWarning(
                                            "Material property '%s': Could not find the image '%s'", propertyId.GetFullName().GetCStr(),
                                            property.second.m_value.GetValue<AZStd::string>().data());
                                    }
                                    
                                    imageAsset.SetAutoLoadBehavior(Data::AssetLoadBehavior::PreLoad);
                                    materialAssetCreator.SetPropertyValue(propertyId.GetFullName(), imageAsset);
                                }
                                break;
                            case MaterialPropertyDataType::Enum:
                                {
                                    AZ::Name enumName = AZ::Name(property.second.m_value.GetValue<AZStd::string>());
                                    uint32_t enumValue = propertyDescriptor->GetEnumValue(enumName);
                                    if (enumValue == MaterialPropertyDescriptor::InvalidEnumValue)
                                    {
                                        materialAssetCreator.ReportError(
                                            "Enum value '%s' couldn't be found in the 'enumValues' list", enumName.GetCStr());
                                    }
                                    else
                                    {
                                        materialAssetCreator.SetPropertyValue(propertyId.GetFullName(), enumValue);
                                    }
                                }
                                break;
                            default:
                                materialAssetCreator.SetPropertyValue(propertyId.GetFullName(), property.second.m_value);
                                break;
                            }
                        }
                        else
                        {
                            materialAssetCreator.ReportWarning(
                                "Can not find property id '%s' in MaterialPropertyLayout", propertyId.GetFullName().GetStringView().data());
                        }
                    }
                }
            }
        }

    } // namespace RPI
} // namespace AZ
