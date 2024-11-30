/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyValueSerializer.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyId.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <Atom/RPI.Edit/Material/MaterialConverterBus.h>

#include <Atom/RPI.Edit/Common/AssetUtils.h>
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
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<MaterialSourceData>()
                    ->Version(2)
                    ->Field("description", &MaterialSourceData::m_description)
                    ->Field("materialType", &MaterialSourceData::m_materialType)
                    ->Field("materialTypeVersion", &MaterialSourceData::m_materialTypeVersion)
                    ->Field("parentMaterial", &MaterialSourceData::m_parentMaterial)
                    ->Field("properties", &MaterialSourceData::m_propertiesOld)
                    ->Field("propertyValues", &MaterialSourceData::m_propertyValues)
                    ;

                serializeContext->RegisterGenericType<PropertyValueMap>();
                serializeContext->RegisterGenericType<PropertyGroupMap>();
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
        
        void MaterialSourceData::SetPropertyValue(const Name& propertyId, const MaterialPropertyValue& value)
        {
            if (!propertyId.IsEmpty())
            {
                m_propertyValues[propertyId] = value;
            }
        }

        const MaterialPropertyValue& MaterialSourceData::GetPropertyValue(const Name& propertyId) const
        {
            auto iter = m_propertyValues.find(propertyId);
            if (iter == m_propertyValues.end())
            {
                return m_invalidValue;
            }
            else
            {
                return iter->second;
            }
        }
        
        void MaterialSourceData::RemovePropertyValue(const Name& propertyId)
        {
            m_propertyValues.erase(propertyId);
        }
        
        const MaterialSourceData::PropertyValueMap& MaterialSourceData::GetPropertyValues() const
        {
            return m_propertyValues;
        }

        bool MaterialSourceData::HasPropertyValue(const Name& propertyId) const
        {
            return m_propertyValues.find(propertyId) != m_propertyValues.end();
        }
        
        void MaterialSourceData::UpgradeLegacyFormat()
        {
            for (const auto& [groupName, propertyList] : m_propertiesOld)
            {
                for (const auto& [propertyName, propertyValue] : propertyList)
                {
                    SetPropertyValue(MaterialPropertyId{groupName, propertyName}, propertyValue);
                }
            }

            m_propertiesOld.clear();
        }

        Outcome<Data::Asset<MaterialAsset>> MaterialSourceData::CreateMaterialAsset(
            Data::AssetId assetId, const AZStd::string& materialSourceFilePath, bool elevateWarnings) const
        {
            if (m_materialType.empty())
            {
                AZ_Error("MaterialSourceData", false, "materialType was not specified");
                return Failure();
            }

            const auto& materialTypeSourcePath =
                MaterialUtils::GetFinalMaterialTypeSourcePath(materialSourceFilePath, m_materialType);
            if (materialTypeSourcePath.empty())
            {
                AZ_Error("MaterialSourceData", false, "Could not find material type file: '%s'.", m_materialType.c_str());
                return Failure();
            }

            // Images are set to pre-load, so they will fully load when loading a material or material type asset.
            // To create the material asset, we don't need to fully load the images that are referenced.
            // So we use this filter to ignore the image assets
            Data::AssetLoadParameters dontLoadImageAssets{ [](const AZ::Data::AssetFilterInfo& filterInfo)
                                                           {
                                                               return
                                                                   filterInfo.m_assetType != AZ::AzTypeInfo<StreamingImageAsset>::Uuid() &&
                                                                   filterInfo.m_assetType != AZ::AzTypeInfo<AttachmentImageAsset>::Uuid() &&
                                                                   filterInfo.m_assetType != AZ::AzTypeInfo<ImageAsset>::Uuid();
                                                           } };

            // In this case we need to load the material type data in preparation for the material->Finalize() step below.
            const auto& materialTypeAssetOutcome =
                AssetUtils::LoadAsset<MaterialTypeAsset>(materialTypeSourcePath, 0, AssetUtils::TraceLevel::Error, dontLoadImageAssets);
            if (!materialTypeAssetOutcome)
            {
                return Failure();
            }

            const auto& materialTypeAsset = materialTypeAssetOutcome.GetValue();
            const auto& materialTypeAssetId = materialTypeAsset.GetId();

            MaterialAssetCreator materialAssetCreator;
            materialAssetCreator.SetElevateWarnings(elevateWarnings);
            materialAssetCreator.Begin(assetId, materialTypeAsset);
            materialAssetCreator.SetMaterialTypeVersion(m_materialTypeVersion);

            if (!m_parentMaterial.empty())
            {
                const auto& parentMaterialAssetOutcome = AssetUtils::LoadAsset<MaterialAsset>(
                    materialSourceFilePath, m_parentMaterial, 0, AssetUtils::TraceLevel::Error, dontLoadImageAssets);
                if (!parentMaterialAssetOutcome.IsSuccess())
                {
                    return Failure();
                }

                const auto& parentMaterialAsset = parentMaterialAssetOutcome.GetValue();
                const auto& parentMaterialTypeAsset = parentMaterialAsset->GetMaterialTypeAsset();
                const auto& parentsMaterialTypeId = parentMaterialTypeAsset.GetId();

                // Make sure the parent material has the same material type
                if (materialTypeAssetId != parentsMaterialTypeId)
                {
                    AZ_Error("MaterialSourceData", false, "This material and its parent material do not share the same material type.");
                    return Failure();
                }

                // Inherit the parent's property values...
                const MaterialPropertiesLayout* propertiesLayout = parentMaterialAsset->GetMaterialPropertiesLayout();

                if (parentMaterialAsset->GetPropertyValues().size() != propertiesLayout->GetPropertyCount())
                {
                    AZ_Assert(
                        false,
                        "The parent material should have been finalized with %zu properties but it has %zu. Something is out of sync.",
                        propertiesLayout->GetPropertyCount(),
                        parentMaterialAsset->GetPropertyValues().size());
                    return Failure();
                }

                for (size_t propertyIndex = 0; propertyIndex < propertiesLayout->GetPropertyCount(); ++propertyIndex)
                {
                    materialAssetCreator.SetPropertyValue(
                        propertiesLayout->GetPropertyDescriptor(MaterialPropertyIndex{ propertyIndex })->GetName(),
                        parentMaterialAsset->GetPropertyValues()[propertyIndex]);
                }
            }

            ApplyPropertiesToAssetCreator(materialAssetCreator, materialSourceFilePath);

            Data::Asset<MaterialAsset> material;
            if (materialAssetCreator.End(material))
            {
                return Success(material);
            }

            return Failure();
        }

        Outcome<Data::Asset<MaterialAsset>> MaterialSourceData::CreateMaterialAssetFromSourceData(
            Data::AssetId assetId,
            AZStd::string_view materialSourceFilePath,
            bool elevateWarnings,
            MaterialUtils::ImportedJsonFiles* sourceDependencies) const
        {
            if (m_materialType.empty())
            {
                AZ_Error("MaterialSourceData", false, "materialType was not specified");
                return Failure();
            }

            const auto& materialTypeSourcePath = MaterialUtils::GetFinalMaterialTypeSourcePath(materialSourceFilePath, m_materialType);
            const auto& materialTypeAssetId = MaterialUtils::GetFinalMaterialTypeAssetId(materialSourceFilePath, m_materialType);

            if (!materialTypeAssetId.IsSuccess() || materialTypeSourcePath.empty())
            {
                AZ_Error("MaterialSourceData", false, "Could not find material type file: '%s'.", m_materialType.c_str());
                return Failure();
            }

            const auto& materialTypeLoadOutcome =
                MaterialUtils::LoadMaterialTypeSourceData(materialTypeSourcePath, nullptr, sourceDependencies);
            if (!materialTypeLoadOutcome)
            {
                AZ_Error("MaterialSourceData", false, "Failed to load MaterialTypeSourceData: '%s'.", materialTypeSourcePath.c_str());
                return Failure();
            }

            const auto& materialTypeSourceData = materialTypeLoadOutcome.GetValue();

            const auto& materialTypeAsset =
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
            AZStd::vector<AZStd::pair<AZStd::string, MaterialSourceData>> parentSourceDataStack;

            AZStd::string parentSourceRelPath = m_parentMaterial;
            AZStd::string parentSourceAbsPath = AssetUtils::ResolvePathReference(materialSourceFilePath, parentSourceRelPath);
            while (!parentSourceRelPath.empty())
            {
                if (!dependencies.insert(parentSourceAbsPath).second)
                {
                    AZ_Error("MaterialSourceData", false, "Detected circular dependency between materials: '%s' and '%s'.", materialSourceFilePath.data(), parentSourceAbsPath.c_str());
                    return Failure();
                }

                const auto& loadParentResult = MaterialUtils::LoadMaterialSourceData(parentSourceAbsPath);
                if (!loadParentResult)
                {
                    AZ_Error("MaterialSourceData", false, "Failed to load MaterialSourceData for parent material: '%s'.", parentSourceAbsPath.c_str());
                    return Failure();
                }
                
                const auto& parentSourceData = loadParentResult.GetValue();

                // Make sure that all materials in the hierarchy share the same material type
                const auto& parentTypeAssetId = MaterialUtils::GetFinalMaterialTypeAssetId(parentSourceAbsPath, parentSourceData.m_materialType);
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

                // Record the material source data and its absolute path so that asset references can be resolved relative to it
                parentSourceDataStack.emplace_back(parentSourceAbsPath, parentSourceData);

                // Get the location of the next parent material and push the source data onto the stack 
                parentSourceRelPath = parentSourceData.m_parentMaterial;
                parentSourceAbsPath = AssetUtils::ResolvePathReference(parentSourceAbsPath, parentSourceRelPath);
            }
            
            // Create the material asset from all the previously loaded source data 
            MaterialAssetCreator materialAssetCreator;
            materialAssetCreator.SetElevateWarnings(elevateWarnings);
            materialAssetCreator.Begin(assetId, materialTypeAsset.GetValue());
            materialAssetCreator.SetMaterialTypeVersion(m_materialTypeVersion);

            // Traverse the parent source data stack in reverse, applying properties from each material parent source data on to the asset
            // creator. This will manually accumulate all material property values in the hierarchy.
            while (!parentSourceDataStack.empty())
            {
                // Images and other assets must be resolved relative to the parent source data absolute path, not the path passed into this
                // function that is the final material being created.
                const auto& parentPath = parentSourceDataStack.back().first;
                const auto& parentData = parentSourceDataStack.back().second;
                parentData.ApplyPropertiesToAssetCreator(materialAssetCreator, parentPath);
                parentSourceDataStack.pop_back();
            }

            // Finally, apply properties from the source data that was initially requested. This could also go into the stack but is being
            // used for other purposes.
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
            for (const auto& [propertyId, propertyValue] : m_propertyValues)
            {
                if (!propertyValue.IsValid())
                {
                    materialAssetCreator.ReportWarning("Value for material property '%s' is invalid.", propertyId.GetCStr());
                }
                else
                {
                    // If the source value type is a string, there are two possible property types: Image and Enum. If there is a "." in
                    // the string (for the extension) we assume it's an Image and look up the referenced Asset. Otherwise, we can assume
                    // it's an Enum value and just preserve the original string.
                    if (MaterialUtils::LooksLikeImageFileReference(propertyValue))
                    {
                        Data::Asset<ImageAsset> imageAsset;

                        MaterialUtils::GetImageAssetResult result = MaterialUtils::GetImageAssetReference(
                            imageAsset, materialSourceFilePath, propertyValue.GetValue<AZStd::string>());
                                    
                        if (result == MaterialUtils::GetImageAssetResult::Missing)
                        {
                            materialAssetCreator.ReportWarning(
                                "Material property '%s': Could not find the image '%s'", propertyId.GetCStr(),
                                propertyValue.GetValue<AZStd::string>().data());
                        }

                        materialAssetCreator.SetPropertyValue(propertyId, imageAsset);
                    }
                    else
                    {
                        materialAssetCreator.SetPropertyValue(propertyId, propertyValue);
                    }
                }
            }
        }
        
        MaterialSourceData MaterialSourceData::CreateAllPropertyDefaultsMaterial(const Data::Asset<MaterialTypeAsset>& materialType, const AZStd::string& materialTypeSourcePath)
        {
            MaterialSourceData material;
            material.m_materialType = materialTypeSourcePath;
            material.m_materialTypeVersion = materialType->GetVersion();
            material.m_description = "For reference, lists the default values for every available property in '" + materialTypeSourcePath + "'";
            auto propertyLayout = materialType->GetMaterialPropertiesLayout();
            for (size_t i = 0; i < propertyLayout->GetPropertyCount(); ++i)
            {
                const MaterialPropertyDescriptor* descriptor = propertyLayout->GetPropertyDescriptor(MaterialPropertyIndex{i});
                Name propertyId = descriptor->GetName();
                MaterialPropertyValue defaultValue = materialType->GetDefaultPropertyValues()[i];

                if (defaultValue.Is<Data::Asset<ImageAsset>>())
                {
                    Data::AssetId assetId = defaultValue.GetValue<Data::Asset<ImageAsset>>().GetId();

                    Data::AssetInfo assetInfo;
                    AZStd::string watchFolder;
                    bool result = false;
                    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(result, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourceUUID, assetId.m_guid, assetInfo, watchFolder);

                    if (result)
                    {
                        material.SetPropertyValue(propertyId, assetInfo.m_relativePath);
                    }
                    else
                    {
                        material.SetPropertyValue(propertyId, AZStd::string{});
                    }
                }
                else if (descriptor->GetDataType() == MaterialPropertyDataType::Enum)
                {
                    AZ_Assert(defaultValue.Is<uint32_t>(), "Enum property definitions should always have a default value of type unsigned int");
                    material.SetPropertyValue(propertyId, descriptor->GetEnumName(defaultValue.GetValue<uint32_t>()));
                }
                else
                {
                    material.SetPropertyValue(propertyId, defaultValue);
                }
            }
            return material;
        }

    } // namespace RPI
} // namespace AZ
