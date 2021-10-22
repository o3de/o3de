/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialPropertySerializer.h>
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceDataSerializer.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyConnectionSerializer.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyGroupSerializer.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAssetCreator.h>
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/MaterialVersionUpdate.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyId.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

namespace AZ
{
    namespace RPI
    {
        MaterialFunctorSourceDataHolder::MaterialFunctorSourceDataHolder(Ptr<MaterialFunctorSourceData> actualSourceData)
            : m_actualSourceData(actualSourceData)
        {}

        void MaterialFunctorSourceDataHolder::Reflect(AZ::ReflectContext* context)
        {
            if (JsonRegistrationContext* jsonContext = azrtti_cast<JsonRegistrationContext*>(context))
            {
                jsonContext->Serializer<JsonMaterialFunctorSourceDataSerializer>()->HandlesType<MaterialFunctorSourceDataHolder>();
            }
            else if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<MaterialFunctorSourceDataHolder>();
            }
        }

        void MaterialTypeSourceData::Reflect(ReflectContext* context)
        {
            if (JsonRegistrationContext* jsonContext = azrtti_cast<JsonRegistrationContext*>(context))
            {
                jsonContext->Serializer<JsonMaterialPropertySerializer>()->HandlesType<MaterialTypeSourceData::PropertyDefinition>();
                jsonContext->Serializer<JsonMaterialPropertyConnectionSerializer>()->HandlesType<MaterialTypeSourceData::PropertyConnection>();
                jsonContext->Serializer<JsonMaterialPropertyGroupSerializer>()->HandlesType<MaterialTypeSourceData::GroupDefinition>();
            }
            else if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<PropertyConnection>()->Version(3);
                serializeContext->Class<GroupDefinition>()->Version(4);
                serializeContext->Class<PropertyDefinition>()->Version(1);

                serializeContext->RegisterGenericType<PropertyConnectionList>();

                serializeContext->Class<VersionUpdatesRenameOperationDefinition>()
                    ->Version(1)
                    ->Field("op", &VersionUpdatesRenameOperationDefinition::m_operation)
                    ->Field("from", &VersionUpdatesRenameOperationDefinition::m_renameFrom)
                    ->Field("to", &VersionUpdatesRenameOperationDefinition::m_renameTo)
                    ;

                serializeContext->RegisterGenericType<VersionUpdateActions>();

                serializeContext->Class<VersionUpdateDefinition>()
                    ->Version(1)
                    ->Field("toVersion", &VersionUpdateDefinition::m_toVersion)
                    ->Field("actions", &VersionUpdateDefinition::m_actions)
                    ;

                serializeContext->RegisterGenericType<VersionUpdates>();

                serializeContext->Class<ShaderVariantReferenceData>()
                    ->Version(2)
                    ->Field("file", &ShaderVariantReferenceData::m_shaderFilePath)
                    ->Field("tag", &ShaderVariantReferenceData::m_shaderTag)
                    ->Field("options", &ShaderVariantReferenceData::m_shaderOptionValues)
                    ;

                serializeContext->Class<PropertyLayout>()
                    ->Version(2) // Material Version Update
                    ->Field("version", &PropertyLayout::m_versionOld)
                    ->Field("groups", &PropertyLayout::m_groups)
                    ->Field("properties", &PropertyLayout::m_properties)
                    ;

                serializeContext->RegisterGenericType<UvNameMap>();

                serializeContext->Class<MaterialTypeSourceData>()
                    ->Version(4) // Material Version Update
                    ->Field("description", &MaterialTypeSourceData::m_description)
                    ->Field("version", &MaterialTypeSourceData::m_version)
                    ->Field("versionUpdates", &MaterialTypeSourceData::m_versionUpdates)
                    ->Field("propertyLayout", &MaterialTypeSourceData::m_propertyLayout)
                    ->Field("shaders", &MaterialTypeSourceData::m_shaderCollection)
                    ->Field("functors", &MaterialTypeSourceData::m_materialFunctorSourceData)
                    ->Field("uvNameMap", &MaterialTypeSourceData::m_uvNameMap)
                    ;
            }
        }

        MaterialTypeSourceData::PropertyConnection::PropertyConnection(MaterialPropertyOutputType type, AZStd::string_view fieldName, int32_t shaderIndex)
            : m_type(type)
            , m_fieldName(fieldName)
            , m_shaderIndex(shaderIndex)
        {
        }

        const float MaterialTypeSourceData::PropertyDefinition::DefaultMin = std::numeric_limits<float>::lowest();
        const float MaterialTypeSourceData::PropertyDefinition::DefaultMax = std::numeric_limits<float>::max();
        const float MaterialTypeSourceData::PropertyDefinition::DefaultStep = 0.1f;

        const MaterialTypeSourceData::GroupDefinition* MaterialTypeSourceData::FindGroup(AZStd::string_view groupName) const
        {
            for (const GroupDefinition& group : m_propertyLayout.m_groups)
            {
                if (group.m_name == groupName)
                {
                    return &group;
                }
            }

            return nullptr;
        }

        bool MaterialTypeSourceData::ApplyPropertyRenames(MaterialPropertyId& propertyId, uint32_t materialTypeVersion) const
        {
            bool renamed = false;

            for (const VersionUpdateDefinition& versionUpdate : m_versionUpdates)
            {
                if (materialTypeVersion >= versionUpdate.m_toVersion)
                {
                    continue;
                }

                for (const VersionUpdatesRenameOperationDefinition& action : versionUpdate.m_actions)
                {
                    if (action.m_operation == "rename")
                    {
                        if (action.m_renameFrom == propertyId.GetFullName().GetStringView())
                        {
                            propertyId = MaterialPropertyId::Parse(action.m_renameTo);
                            renamed = true;
                        }
                    }
                    else
                    {
                        AZ_Warning("Material source data", false, "Unsupported material version update operation '%s'", action.m_operation.c_str());
                    }
                }
            }

            return renamed;
        }

        const MaterialTypeSourceData::PropertyDefinition* MaterialTypeSourceData::FindProperty(AZStd::string_view groupName, AZStd::string_view propertyName, uint32_t materialTypeVersion) const
        {
            auto groupIter = m_propertyLayout.m_properties.find(groupName);
            if (groupIter == m_propertyLayout.m_properties.end())
            {
                return nullptr;
            }

            for (const PropertyDefinition& property : groupIter->second)
            {
                if (property.m_name == propertyName)
                {
                    return &property;
                }
            }

            // Property has not been found, try looking for renames in the version history

            MaterialPropertyId propertyId = MaterialPropertyId{groupName, propertyName};
            ApplyPropertyRenames(propertyId, materialTypeVersion);

            // Do the search again with the new names

            groupIter = m_propertyLayout.m_properties.find(propertyId.GetGroupName().GetStringView());
            if (groupIter == m_propertyLayout.m_properties.end())
            {
                return nullptr;
            }

            for (const PropertyDefinition& property : groupIter->second)
            {
                if (property.m_name == propertyId.GetPropertyName().GetStringView())
                {
                    return &property;
                }
            }

            return nullptr;
        }

        void MaterialTypeSourceData::ResolveUvEnums()
        {
            AZStd::vector<AZStd::string> enumValues;
            enumValues.reserve(m_uvNameMap.size());
            for (const auto& uvNamePair : m_uvNameMap)
            {
                enumValues.push_back(uvNamePair.second);
            }

            for (auto& group : m_propertyLayout.m_properties)
            {
                for (PropertyDefinition& property : group.second)
                {
                    if (property.m_dataType == AZ::RPI::MaterialPropertyDataType::Enum && property.m_enumIsUv)
                    {
                        property.m_enumValues = enumValues;
                    }
                }
            }
        }

        AZStd::vector<MaterialTypeSourceData::GroupDefinition> MaterialTypeSourceData::GetGroupDefinitionsInDisplayOrder() const
        {
            AZStd::vector<MaterialTypeSourceData::GroupDefinition> groupDefinitions;
            groupDefinitions.reserve(m_propertyLayout.m_properties.size());

            // Some groups are defined explicitly in the .materialtype file's "groups" section. This is the primary way groups are sorted in the UI.
            AZStd::unordered_set<AZStd::string> foundGroups;
            for (const auto& groupDefinition : m_propertyLayout.m_groups)
            {
                if (foundGroups.insert(groupDefinition.m_name).second)
                {
                    groupDefinitions.push_back(groupDefinition);
                }
                else
                {
                    AZ_Warning("Material source data", false, "Duplicate group '%s' found.", groupDefinition.m_name.c_str());
                }
            }

            // Some groups are defined implicitly, in the "properties" section where a group name is used but not explicitly defined in the "groups" section.
            for (const auto& propertyListPair : m_propertyLayout.m_properties)
            {
                const AZStd::string& groupName = propertyListPair.first;
                if (foundGroups.insert(groupName).second)
                {
                    MaterialTypeSourceData::GroupDefinition groupDefinition;
                    groupDefinition.m_name = groupName;
                    groupDefinitions.push_back(groupDefinition);
                }
            }

            return groupDefinitions;
        }

        void MaterialTypeSourceData::EnumerateProperties(const EnumeratePropertiesCallback& callback) const
        {
            if (!callback)
            {
                return;
            }

            for (const auto& propertyListPair : m_propertyLayout.m_properties)
            {
                const AZStd::string& groupName = propertyListPair.first;
                const auto& propertyList = propertyListPair.second;
                for (const auto& propertyDefinition : propertyList)
                {
                    const AZStd::string& propertyName = propertyDefinition.m_name;
                    if (!callback(groupName, propertyName, propertyDefinition))
                    {
                        return;
                    }
                }
            }
        }

        void MaterialTypeSourceData::EnumeratePropertiesInDisplayOrder(const EnumeratePropertiesCallback& callback) const
        {
            if (!callback)
            {
                return;
            }

            for (const auto& groupDefinition : GetGroupDefinitionsInDisplayOrder())
            {
                const AZStd::string& groupName = groupDefinition.m_name;
                const auto propertyListItr = m_propertyLayout.m_properties.find(groupName);
                if (propertyListItr != m_propertyLayout.m_properties.end())
                {
                    const auto& propertyList = propertyListItr->second;
                    for (const auto& propertyDefinition : propertyList)
                    {
                        const AZStd::string& propertyName = propertyDefinition.m_name;
                        if (!callback(groupName, propertyName, propertyDefinition))
                        {
                            return;
                        }
                    }
                }
            }
        }

        bool MaterialTypeSourceData::ConvertPropertyValueToSourceDataFormat(const PropertyDefinition& propertyDefinition, MaterialPropertyValue& propertyValue) const
        {
            if (propertyDefinition.m_dataType == AZ::RPI::MaterialPropertyDataType::Enum && propertyValue.Is<uint32_t>())
            {
                const uint32_t index = propertyValue.GetValue<uint32_t>();
                if (index >= propertyDefinition.m_enumValues.size())
                {
                    AZ_Error("Material source data", false, "Invalid value for material enum property: '%s'.", propertyDefinition.m_name.c_str());
                    return false;
                }

                propertyValue = propertyDefinition.m_enumValues[index];
                return true;
            }

            // Image asset references must be converted from asset IDs to a relative source file path
            if (propertyDefinition.m_dataType == AZ::RPI::MaterialPropertyDataType::Image && propertyValue.Is<Data::Asset<ImageAsset>>())
            {
                const Data::Asset<ImageAsset>& imageAsset = propertyValue.GetValue<Data::Asset<ImageAsset>>();

                Data::AssetInfo imageAssetInfo;
                if (imageAsset.GetId().IsValid())
                {
                    bool result = false;
                    AZStd::string rootFilePath;
                    const AZStd::string platformName = ""; // Empty for default
                    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(result, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetAssetInfoById,
                        imageAsset.GetId(), imageAsset.GetType(), platformName, imageAssetInfo, rootFilePath);
                    if (!result)
                    {
                        AZ_Error("Material source data", false, "Image asset could not be found for property: '%s'.", propertyDefinition.m_name.c_str());
                        return false;
                    }
                }

                propertyValue = imageAssetInfo.m_relativePath;
                return true;
            }

            return true;
        }

        Outcome<Data::Asset<MaterialTypeAsset>> MaterialTypeSourceData::CreateMaterialTypeAsset(Data::AssetId assetId, AZStd::string_view materialTypeSourceFilePath, bool elevateWarnings) const
        {
            MaterialTypeAssetCreator materialTypeAssetCreator;
            materialTypeAssetCreator.SetElevateWarnings(elevateWarnings);
            materialTypeAssetCreator.Begin(assetId);

            if (m_propertyLayout.m_versionOld != 0)
            {
                materialTypeAssetCreator.ReportError(
                    "The field '/propertyLayout/version' is deprecated and moved to '/version'. "
                    "Please edit this material type source file and move the '\"version\": %u' setting up one level.",
                    m_propertyLayout.m_versionOld);
                return Failure();
            }

            // Set materialtype version and add each version update object into MaterialTypeAsset.
            materialTypeAssetCreator.SetVersion(m_version);
            {
                const AZ::Name rename = AZ::Name{ "rename" };

                for (const auto& versionUpdate : m_versionUpdates)
                {
                    MaterialVersionUpdate materialVersionUpdate{versionUpdate.m_toVersion};
                    for (const auto& action : versionUpdate.m_actions)
                    {
                        if (action.m_operation == rename.GetStringView())
                        {
                            materialVersionUpdate.AddAction(MaterialVersionUpdate::RenamePropertyAction{
                                    AZ::Name{ action.m_renameFrom },
                                    AZ::Name{ action.m_renameTo }
                                });
                        }
                        else
                        {
                            materialTypeAssetCreator.ReportWarning("Unsupported material version update operation '%s'", action.m_operation.c_str());
                        }
                    }
                    materialTypeAssetCreator.AddVersionUpdate(materialVersionUpdate);
                }
            }

            // Used to gather all the UV streams used in this material type from its shaders in alphabetical order.
            auto semanticComp = [](const RHI::ShaderSemantic& lhs, const RHI::ShaderSemantic& rhs) -> bool
            {
                return lhs.ToString() < rhs.ToString();
            };
            AZStd::set<RHI::ShaderSemantic, decltype(semanticComp)> uvsInThisMaterialType(semanticComp);

            for (const ShaderVariantReferenceData& shaderRef : m_shaderCollection)
            {
                const auto& shaderFile = shaderRef.m_shaderFilePath;
                const auto& shaderAsset = AssetUtils::LoadAsset<ShaderAsset>(materialTypeSourceFilePath, shaderFile, 0);

                if (shaderAsset)
                {
                    auto optionsLayout = shaderAsset.GetValue()->GetShaderOptionGroupLayout();
                    ShaderOptionGroup options{ optionsLayout };
                    for (auto& iter : shaderRef.m_shaderOptionValues)
                    {
                        if (!options.SetValue(iter.first, iter.second))
                        {
                            return Failure();
                        }
                    }

                    materialTypeAssetCreator.AddShader(
                        shaderAsset.GetValue(), options.GetShaderVariantId(),
                        shaderRef.m_shaderTag.IsEmpty() ? Uuid::CreateRandom().ToString<AZ::Name>() : shaderRef.m_shaderTag
                    );

                    // Gather UV names
                    const ShaderInputContract& shaderInputContract = shaderAsset.GetValue()->GetInputContract();
                    for (const ShaderInputContract::StreamChannelInfo& channel : shaderInputContract.m_streamChannels)
                    {
                        const RHI::ShaderSemantic& semantic = channel.m_semantic;

                        if (semantic.m_name.GetStringView().starts_with(RHI::ShaderSemantic::UvStreamSemantic))
                        {
                            uvsInThisMaterialType.insert(semantic);
                        }
                    }
                }
                else
                {
                    materialTypeAssetCreator.ReportError("Shader '%s' not found", shaderFile.data());
                    return Failure();
                }
            }

            for (auto& groupIter : m_propertyLayout.m_properties)
            {
                const AZStd::string& groupName = groupIter.first;

                for (const PropertyDefinition& property : groupIter.second)
                {
                    // Register the property...

                    MaterialPropertyId propertyId{ groupName, property.m_name };

                    if (!propertyId.IsValid())
                    {
                        materialTypeAssetCreator.ReportWarning("Cannot create material property with invalid ID '%s'.", propertyId.GetCStr());
                        continue;
                    }

                    materialTypeAssetCreator.BeginMaterialProperty(propertyId.GetFullName(), property.m_dataType);

                    if (property.m_dataType == MaterialPropertyDataType::Enum)
                    {
                        materialTypeAssetCreator.SetMaterialPropertyEnumNames(property.m_enumValues);
                    }

                    for (auto& output : property.m_outputConnections)
                    {
                        switch (output.m_type)
                        {
                        case MaterialPropertyOutputType::ShaderInput:
                            materialTypeAssetCreator.ConnectMaterialPropertyToShaderInput(Name{ output.m_fieldName.data() });
                            break;
                        case MaterialPropertyOutputType::ShaderOption:
                            if (output.m_shaderIndex >= 0)
                            {
                                materialTypeAssetCreator.ConnectMaterialPropertyToShaderOption(Name{ output.m_fieldName.data() }, output.m_shaderIndex);
                            }
                            else
                            {
                                materialTypeAssetCreator.ConnectMaterialPropertyToShaderOptions(Name{ output.m_fieldName.data() });
                            }
                            break;
                        case MaterialPropertyOutputType::Invalid:
                            // Don't add any output mappings, this is the case when material functors are expected to process the property
                            break;
                        default:
                            AZ_Assert(false, "Unsupported MaterialPropertyOutputType");
                            return Failure();
                        }
                    }

                    materialTypeAssetCreator.EndMaterialProperty();

                    // Parse and set the property's value...
                    if (!property.m_value.IsValid())
                    {
                        AZ_Warning("Material source data", false, "Source data for material property value is invalid.");
                    }
                    else
                    {
                        switch (property.m_dataType)
                        {
                        case MaterialPropertyDataType::Image:
                        {
                            Outcome<Data::Asset<ImageAsset>> imageAssetResult = MaterialUtils::GetImageAssetReference(materialTypeSourceFilePath, property.m_value.GetValue<AZStd::string>());

                            if (imageAssetResult.IsSuccess())
                            {
                                materialTypeAssetCreator.SetPropertyValue(propertyId.GetFullName(), imageAssetResult.GetValue());
                            }
                            else
                            {
                                materialTypeAssetCreator.ReportError("Material property '%s': Could not find the image '%s'", propertyId.GetFullName().GetCStr(), property.m_value.GetValue<AZStd::string>().data());
                            }
                        }
                        break;
                        case MaterialPropertyDataType::Enum:
                        {
                            MaterialPropertyIndex propertyIndex = materialTypeAssetCreator.GetMaterialPropertiesLayout()->FindPropertyIndex(propertyId.GetFullName());
                            const MaterialPropertyDescriptor* propertyDescriptor = materialTypeAssetCreator.GetMaterialPropertiesLayout()->GetPropertyDescriptor(propertyIndex);

                            AZ::Name enumName = AZ::Name(property.m_value.GetValue<AZStd::string>());
                            uint32_t enumValue = propertyDescriptor ? propertyDescriptor->GetEnumValue(enumName) : MaterialPropertyDescriptor::InvalidEnumValue;
                            if (enumValue == MaterialPropertyDescriptor::InvalidEnumValue)
                            {
                                materialTypeAssetCreator.ReportError("Enum value '%s' couldn't be found in the 'enumValues' list", enumName.GetCStr());
                            }
                            else
                            {
                                materialTypeAssetCreator.SetPropertyValue(propertyId.GetFullName(), enumValue);
                            }
                        }
                        break;
                        default:
                            materialTypeAssetCreator.SetPropertyValue(propertyId.GetFullName(), property.m_value);
                            break;
                        }
                    }
                }
            }

            // We cannot create the MaterialFunctor until after all the properties are added because 
            // CreateFunctor() may need to look up properties in the MaterialPropertiesLayout
            for (auto& functorData : m_materialFunctorSourceData)
            {
                MaterialFunctorSourceData::FunctorResult result = functorData->CreateFunctor(
                    MaterialFunctorSourceData::RuntimeContext(
                        materialTypeSourceFilePath,
                        materialTypeAssetCreator.GetMaterialPropertiesLayout(),
                        materialTypeAssetCreator.GetMaterialShaderResourceGroupLayout(),
                        materialTypeAssetCreator.GetShaderCollection()
                    )
                );

                if (result.IsSuccess())
                {
                    Ptr<MaterialFunctor>& functor = result.GetValue();
                    if (functor != nullptr)
                    {
                        materialTypeAssetCreator.AddMaterialFunctor(functor);

                        for (const AZ::Name& optionName : functorData->GetActualSourceData()->GetShaderOptionDependencies())
                        {
                            materialTypeAssetCreator.ClaimShaderOptionOwnership(optionName);
                        }
                    }
                }
                else
                {
                    materialTypeAssetCreator.ReportError("Failed to create MaterialFunctor");
                    return Failure();
                }
            }

            // Only add the UV mapping related to this material type.
            for (const auto& uvInput : uvsInThisMaterialType)
            {
                // We may have cases where the uv map is empty or inconsistent (exported from other projects),
                // So we use semantic if mapping is not found.
                auto iter = m_uvNameMap.find(uvInput.ToString());
                if (iter != m_uvNameMap.end())
                {
                    materialTypeAssetCreator.AddUvName(uvInput, Name(iter->second));
                }
                else
                {
                    materialTypeAssetCreator.AddUvName(uvInput, Name(uvInput.ToString()));
                }
            }

            Data::Asset<MaterialTypeAsset> materialTypeAsset;
            if (materialTypeAssetCreator.End(materialTypeAsset))
            {
                return Success(AZStd::move(materialTypeAsset));
            }
            else
            {
                return Failure();
            }
        }

    } // namespace RPI
} // namespace AZ
