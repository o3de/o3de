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
                
                serializeContext->RegisterGenericType<AZStd::unique_ptr<PropertySet>>();
                serializeContext->RegisterGenericType<AZStd::unique_ptr<PropertyDefinition>>();
                serializeContext->RegisterGenericType<AZStd::vector<AZStd::unique_ptr<PropertySet>>>();
                serializeContext->RegisterGenericType<AZStd::vector<AZStd::unique_ptr<PropertyDefinition>>>();
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

                serializeContext->Class<PropertySet>()
                    ->Version(1)
                    ->Field("name", &PropertySet::m_name)
                    ->Field("displayName", &PropertySet::m_displayName)
                    ->Field("description", &PropertySet::m_description)
                    ->Field("properties", &PropertySet::m_properties)
                    ->Field("propertySets", &PropertySet::m_propertySets)
                    ->Field("functors", &PropertySet::m_materialFunctorSourceData)
                    ;

                serializeContext->Class<PropertyLayout>()
                    ->Version(3) // Added propertySets
                    ->Field("version", &PropertyLayout::m_versionOld)       //< Deprecated, preserved for backward compatibility, replaced by MaterialTypeSourceData::version
                    ->Field("groups", &PropertyLayout::m_groupsOld)         //< Deprecated, preserved for backward compatibility, replaced by propertySets
                    ->Field("properties", &PropertyLayout::m_propertiesOld) //< Deprecated, preserved for backward compatibility, replaced by propertySets
                    ->Field("propertySets", &PropertyLayout::m_propertySets)
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
        
        /*static*/ MaterialTypeSourceData::PropertySet* MaterialTypeSourceData::PropertySet::AddPropertySet(AZStd::string_view name, AZStd::vector<AZStd::unique_ptr<PropertySet>>& toPropertySetList)
        {
            auto iter = AZStd::find_if(toPropertySetList.begin(), toPropertySetList.end(), [name](const AZStd::unique_ptr<PropertySet>& existingPropertySet)
                {
                    return existingPropertySet->m_name == name;
                });

            if (iter != toPropertySetList.end())
            {
                AZ_Error("Material source data", false, "PropertySet named '%.*s' already exists", AZ_STRING_ARG(name));
                return nullptr;
            }
            
            if (!MaterialPropertyId::IsValidName(name))
            {
                AZ_Error("Material source data", false, "'%.*s' is not a valid identifier", AZ_STRING_ARG(name));
                return nullptr;
            }

            toPropertySetList.push_back(AZStd::make_unique<PropertySet>());
            toPropertySetList.back()->m_name = name;
            return toPropertySetList.back().get();
        }

        MaterialTypeSourceData::PropertyDefinition* MaterialTypeSourceData::PropertySet::AddProperty(AZStd::string_view name)
        {
            auto propertyIter = AZStd::find_if(m_properties.begin(), m_properties.end(), [name](const AZStd::unique_ptr<PropertyDefinition>& existingProperty)
                {
                    return existingProperty->GetName() == name;    
                });

            if (propertyIter != m_properties.end())
            {
                AZ_Error("Material source data", false, "PropertySet '%s' already contains a property named '%.*s'", m_name.c_str(), AZ_STRING_ARG(name));
                return nullptr;
            }
            
            auto propertySetIter = AZStd::find_if(m_propertySets.begin(), m_propertySets.end(), [name](const AZStd::unique_ptr<PropertySet>& existingPropertySet)
                {
                    return existingPropertySet->m_name == name;
                });

            if (propertySetIter != m_propertySets.end())
            {
                AZ_Error("Material source data", false, "Property name '%.*s' collides with a PropertySet of the same name", AZ_STRING_ARG(name));
                return nullptr;
            }

            if (!MaterialPropertyId::IsValidName(name))
            {
                AZ_Error("Material source data", false, "'%.*s' is not a valid identifier", AZ_STRING_ARG(name));
                return nullptr;
            }

            m_properties.emplace_back(AZStd::make_unique<PropertyDefinition>(name));
            return m_properties.back().get();
        }

        MaterialTypeSourceData::PropertySet* MaterialTypeSourceData::PropertySet::AddPropertySet(AZStd::string_view name)
        {
            auto iter = AZStd::find_if(m_properties.begin(), m_properties.end(), [name](const AZStd::unique_ptr<PropertyDefinition>& existingProperty)
                {
                    return existingProperty->GetName() == name;
                });

            if (iter != m_properties.end())
            {
                AZ_Error("Material source data", false, "PropertySet name '%.*s' collides with a Property of the same name", AZ_STRING_ARG(name));
                return nullptr;
            }

            return AddPropertySet(name, m_propertySets);
        }
        
        MaterialTypeSourceData::PropertySet* MaterialTypeSourceData::AddPropertySet(AZStd::string_view propertySetId)
        {
            AZStd::vector<AZStd::string_view> splitPropertySetId = SplitId(propertySetId);

            if (splitPropertySetId.size() == 1)
            {
                return PropertySet::AddPropertySet(propertySetId, m_propertyLayout.m_propertySets);
            }

            PropertySet* parentPropertySet = FindPropertySet(splitPropertySetId[0]);
            
            if (!parentPropertySet)
            {
                AZ_Error("Material source data", false, "PropertySet '%.*s' does not exists", AZ_STRING_ARG(splitPropertySetId[0]));
                return nullptr;
            }

            return parentPropertySet->AddPropertySet(splitPropertySetId[1]);
        }
        
        MaterialTypeSourceData::PropertyDefinition* MaterialTypeSourceData::AddProperty(AZStd::string_view propertyId)
        {
            AZStd::vector<AZStd::string_view> splitPropertyId = SplitId(propertyId);

            if (splitPropertyId.size() == 1)
            {
                AZ_Error("Material source data", false, "Property id '%.*s' is invalid. Properties must be added to a PropertySet (i.e. \"general.%.*s\").", AZ_STRING_ARG(propertyId), AZ_STRING_ARG(propertyId));
                return nullptr;
            }

            PropertySet* parentPropertySet = FindPropertySet(splitPropertyId[0]);
            
            if (!parentPropertySet)
            {
                AZ_Error("Material source data", false, "PropertySet '%.*s' does not exists", AZ_STRING_ARG(splitPropertyId[0]));
                return nullptr;
            }

            return parentPropertySet->AddProperty(splitPropertyId[1]);
        }
        
        const MaterialTypeSourceData::PropertySet* MaterialTypeSourceData::FindPropertySet(AZStd::array_view<AZStd::string_view> parsedPropertySetId, AZStd::array_view<AZStd::unique_ptr<PropertySet>> inPropertySetList) const
        {
            for (const auto& propertySet : inPropertySetList)
            {
                if (propertySet->m_name != parsedPropertySetId[0])
                {
                    continue;
                }
                else if (parsedPropertySetId.size() == 1)
                {
                    return propertySet.get();
                }
                else
                {
                    AZStd::array_view<AZStd::string_view> subPath{parsedPropertySetId.begin() + 1, parsedPropertySetId.end()};

                    if (!subPath.empty())
                    {
                        const MaterialTypeSourceData::PropertySet* propertySubset = FindPropertySet(subPath, propertySet->m_propertySets);
                        if (propertySubset)
                        {
                            return propertySubset;
                        }
                    }
                }
            }

            return nullptr;
        }
        
        MaterialTypeSourceData::PropertySet* MaterialTypeSourceData::FindPropertySet(AZStd::array_view<AZStd::string_view> parsedPropertySetId, AZStd::array_view<AZStd::unique_ptr<PropertySet>> inPropertySetList)
        {
            return const_cast<PropertySet*>(const_cast<const MaterialTypeSourceData*>(this)->FindPropertySet(parsedPropertySetId, inPropertySetList));
        }

        const MaterialTypeSourceData::PropertySet* MaterialTypeSourceData::FindPropertySet(AZStd::string_view propertySetId) const
        {
            AZStd::vector<AZStd::string_view> tokens = TokenizeId(propertySetId);
            return FindPropertySet(tokens, m_propertyLayout.m_propertySets);
        }

        MaterialTypeSourceData::PropertySet* MaterialTypeSourceData::FindPropertySet(AZStd::string_view propertySetId)
        {
            AZStd::vector<AZStd::string_view> tokens = TokenizeId(propertySetId);
            return FindPropertySet(tokens, m_propertyLayout.m_propertySets);
        }
        
        const MaterialTypeSourceData::PropertyDefinition* MaterialTypeSourceData::FindProperty(
            AZStd::array_view<AZStd::string_view> parsedPropertyId,
            AZStd::array_view<AZStd::unique_ptr<PropertySet>> inPropertySetList) const
        {
            for (const auto& propertySet : inPropertySetList)
            {
                if (propertySet->m_name == parsedPropertyId[0])
                {
                    AZStd::array_view<AZStd::string_view> subPath {parsedPropertyId.begin() + 1, parsedPropertyId.end()};

                    if (subPath.size() == 1)
                    {
                        for (AZStd::unique_ptr<PropertyDefinition>& property : propertySet->m_properties)
                        {
                            if (property->GetName() == subPath[0])
                            {
                                return property.get();
                            }
                        }
                    }
                    else if(subPath.size() > 1)
                    {
                        const MaterialTypeSourceData::PropertyDefinition* property = FindProperty(subPath, propertySet->m_propertySets);
                        if (property)
                        {
                            return property;
                        }
                    }
                }
            }

            return nullptr;
        }
        
        MaterialTypeSourceData::PropertyDefinition* MaterialTypeSourceData::FindProperty(AZStd::array_view<AZStd::string_view> parsedPropertyId, AZStd::array_view<AZStd::unique_ptr<PropertySet>> inPropertySetList)
        {
            return const_cast<MaterialTypeSourceData::PropertyDefinition*>(const_cast<const MaterialTypeSourceData*>(this)->FindProperty(parsedPropertyId, inPropertySetList));
        }

        const MaterialTypeSourceData::PropertyDefinition* MaterialTypeSourceData::FindProperty(AZStd::string_view propertyId) const
        {
            AZStd::vector<AZStd::string_view> tokens = TokenizeId(propertyId);
            return FindProperty(tokens, m_propertyLayout.m_propertySets);
        }
        
        MaterialTypeSourceData::PropertyDefinition* MaterialTypeSourceData::FindProperty(AZStd::string_view propertyId)
        {
            AZStd::vector<AZStd::string_view> tokens = TokenizeId(propertyId);
            return FindProperty(tokens, m_propertyLayout.m_propertySets);
        }

        AZStd::vector<AZStd::string_view> MaterialTypeSourceData::TokenizeId(AZStd::string_view id)
        {
            AZStd::vector<AZStd::string_view> tokens;

            AzFramework::StringFunc::TokenizeVisitor(id, [&tokens](AZStd::string_view t)
                {
                    tokens.push_back(t);
                },
                "./", true, true);

            return tokens;
        }
        
        AZStd::vector<AZStd::string_view> MaterialTypeSourceData::SplitId(AZStd::string_view id)
        {
            AZStd::vector<AZStd::string_view> parts;
            parts.reserve(2);
            size_t lastDelim = id.rfind('.', id.size()-1);
            if (lastDelim == AZStd::string::npos)
            {
                parts.push_back(id);
            }
            else
            {
                parts.push_back(AZStd::string_view{id.begin(), id.begin()+lastDelim});
                parts.push_back(AZStd::string_view{id.begin()+lastDelim+1, id.end()});
            }

            return parts;
        }

        bool MaterialTypeSourceData::EnumeratePropertySets(const EnumeratePropertySetsCallback& callback, AZStd::string propertyNameContext, const AZStd::vector<AZStd::unique_ptr<PropertySet>>& inPropertySetList) const
        {
            for (auto& propertySet : inPropertySetList)
            {
                if (!callback(propertyNameContext, propertySet.get()))
                {
                    return false; // Stop processing
                }

                const AZStd::string propertyNameContext2 = propertyNameContext + propertySet->m_name + ".";

                if (!EnumeratePropertySets(callback, propertyNameContext2, propertySet->m_propertySets))
                {
                    return false; // Stop processing
                }
            }

            return true;
        }

        bool MaterialTypeSourceData::EnumeratePropertySets(const EnumeratePropertySetsCallback& callback) const
        {
            if (!callback)
            {
                return false;
            }

            return EnumeratePropertySets(callback, {}, m_propertyLayout.m_propertySets);
        }

        bool MaterialTypeSourceData::EnumerateProperties(const EnumeratePropertiesCallback& callback, AZStd::string propertyNameContext, const AZStd::vector<AZStd::unique_ptr<PropertySet>>& inPropertySetList) const
        {

            for (auto& propertySet : inPropertySetList)
            {
                const AZStd::string propertyNameContext2 = propertyNameContext + propertySet->m_name + ".";

                for (auto& property : propertySet->m_properties)
                {
                    if (!callback(propertyNameContext2, property.get()))
                    {
                        return false; // Stop processing
                    }
                }

                if (!EnumerateProperties(callback, propertyNameContext2, propertySet->m_propertySets))
                {
                    return false; // Stop processing
                }
            }

            return true;
        }

        bool MaterialTypeSourceData::EnumerateProperties(const EnumeratePropertiesCallback& callback) const
        {
            if (!callback)
            {
                return false;
            }

            return EnumerateProperties(callback, {}, m_propertyLayout.m_propertySets);
        }

        bool MaterialTypeSourceData::ConvertToNewDataFormat()
        {            
            for (const auto& group : GetOldFormatGroupDefinitionsInDisplayOrder())
            {
                auto propertyListItr = m_propertyLayout.m_propertiesOld.find(group.m_name);
                if (propertyListItr != m_propertyLayout.m_propertiesOld.end())
                {
                    const auto& propertyList = propertyListItr->second;
                    for (auto& propertyDefinition : propertyList)
                    {
                        PropertySet* propertySet = FindPropertySet(group.m_name);

                        if (!propertySet)
                        {
                            m_propertyLayout.m_propertySets.emplace_back(AZStd::make_unique<PropertySet>());
                            m_propertyLayout.m_propertySets.back()->m_name = group.m_name;
                            m_propertyLayout.m_propertySets.back()->m_displayName = group.m_displayName;
                            m_propertyLayout.m_propertySets.back()->m_description = group.m_description;
                            propertySet = m_propertyLayout.m_propertySets.back().get();
                        }

                        PropertyDefinition* newProperty = propertySet->AddProperty(propertyDefinition.GetName());
                        
                        *newProperty = propertyDefinition; 
                    }
                }
            }

            m_propertyLayout.m_groupsOld.clear();
            m_propertyLayout.m_propertiesOld.clear();

            return true;
        }

        void MaterialTypeSourceData::ResolveUvEnums()
        {
            AZStd::vector<AZStd::string> enumValues;
            enumValues.reserve(m_uvNameMap.size());
            for (const auto& uvNamePair : m_uvNameMap)
            {
                enumValues.push_back(uvNamePair.second);
            }
            
            EnumerateProperties([&enumValues](const AZStd::string&, const MaterialTypeSourceData::PropertyDefinition* property)
                {
                    if (property->m_dataType == AZ::RPI::MaterialPropertyDataType::Enum && property->m_enumIsUv)
                    {
                        // const_cast is safe because this is internal to the MaterialTypeSourceData. It isn't worth complicating things
                        // by adding another version of EnumerateProperties.
                        const_cast<MaterialTypeSourceData::PropertyDefinition*>(property)->m_enumValues = enumValues;
                    }
                    return true;
                });
        }

        AZStd::vector<MaterialTypeSourceData::GroupDefinition> MaterialTypeSourceData::GetOldFormatGroupDefinitionsInDisplayOrder() const
        {
            AZStd::vector<MaterialTypeSourceData::GroupDefinition> groupDefinitions;
            groupDefinitions.reserve(m_propertyLayout.m_propertiesOld.size());

            // Some groups are defined explicitly in the .materialtype file's "groups" section. This is the primary way groups are sorted in the UI.
            AZStd::unordered_set<AZStd::string> foundGroups;
            for (const auto& groupDefinition : m_propertyLayout.m_groupsOld)
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
            for (const auto& propertyListPair : m_propertyLayout.m_propertiesOld)
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

        bool MaterialTypeSourceData::BuildPropertyList(
            const AZStd::string& materialTypeSourceFilePath,
            MaterialTypeAssetCreator& materialTypeAssetCreator,
            AZStd::vector<AZStd::string>& propertyNameContext,
            const MaterialTypeSourceData::PropertySet* propertySet) const
        {            
            for (const AZStd::unique_ptr<PropertyDefinition>& property : propertySet->m_properties)
            {
                // Register the property...

                MaterialPropertyId propertyId{propertyNameContext, property->GetName()};

                if (!propertyId.IsValid())
                {
                    // MaterialPropertyId reports an error message
                    return false;
                }

                auto propertySetIter = AZStd::find_if(propertySet->GetPropertySets().begin(), propertySet->GetPropertySets().end(),
                    [&property](const AZStd::unique_ptr<PropertySet>& existingPropertySet)
                    {
                        return existingPropertySet->GetName() == property->GetName();
                    });

                if (propertySetIter != propertySet->GetPropertySets().end())
                {
                    AZ_Error("Material source data", false, "Material property '%s' collides with a PropertySet with the same ID.", propertyId.GetCStr());
                    return false;
                }

                materialTypeAssetCreator.BeginMaterialProperty(propertyId, property->m_dataType);
                
                if (property->m_dataType == MaterialPropertyDataType::Enum)
                {
                    materialTypeAssetCreator.SetMaterialPropertyEnumNames(property->m_enumValues);
                }

                for (auto& output : property->m_outputConnections)
                {
                    switch (output.m_type)
                    {
                    case MaterialPropertyOutputType::ShaderInput:
                    {
                        materialTypeAssetCreator.ConnectMaterialPropertyToShaderInput(Name{output.m_fieldName});
                        break;
                    }
                    case MaterialPropertyOutputType::ShaderOption:
                    {
                        if (output.m_shaderIndex >= 0)
                        {
                            materialTypeAssetCreator.ConnectMaterialPropertyToShaderOption(Name{output.m_fieldName}, output.m_shaderIndex);
                        }
                        else
                        {
                            materialTypeAssetCreator.ConnectMaterialPropertyToShaderOptions(Name{output.m_fieldName});
                        }
                        break;
                    }
                    case MaterialPropertyOutputType::Invalid:
                        // Don't add any output mappings, this is the case when material functors are expected to process the property
                        break;
                    default:
                        AZ_Assert(false, "Unsupported MaterialPropertyOutputType");
                        return false;
                    }
                }

                materialTypeAssetCreator.EndMaterialProperty();

                // Parse and set the property's value...
                if (!property->m_value.IsValid())
                {
                    AZ_Warning("Material source data", false, "Source data for material property value is invalid.");
                }
                else
                {
                    switch (property->m_dataType)
                    {
                    case MaterialPropertyDataType::Image:
                    {
                        Data::Asset<ImageAsset> imageAsset;

                        MaterialUtils::GetImageAssetResult result = MaterialUtils::GetImageAssetReference(
                            imageAsset, materialTypeSourceFilePath, property->m_value.GetValue<AZStd::string>());

                        if (result == MaterialUtils::GetImageAssetResult::Missing)
                        {
                            materialTypeAssetCreator.ReportError(
                                "Material property '%s': Could not find the image '%s'", propertyId.GetCStr(),
                                property->m_value.GetValue<AZStd::string>().data());
                        }
                        else
                        {
                            materialTypeAssetCreator.SetPropertyValue(propertyId, imageAsset);
                        }
                    }
                    break;
                    case MaterialPropertyDataType::Enum:
                    {
                        MaterialPropertyIndex propertyIndex = materialTypeAssetCreator.GetMaterialPropertiesLayout()->FindPropertyIndex(propertyId);
                        const MaterialPropertyDescriptor* propertyDescriptor = materialTypeAssetCreator.GetMaterialPropertiesLayout()->GetPropertyDescriptor(propertyIndex);

                        AZ::Name enumName = AZ::Name(property->m_value.GetValue<AZStd::string>());
                        uint32_t enumValue = propertyDescriptor->GetEnumValue(enumName);
                        if (enumValue == MaterialPropertyDescriptor::InvalidEnumValue)
                        {
                            materialTypeAssetCreator.ReportError("Enum value '%s' couldn't be found in the 'enumValues' list", enumName.GetCStr());
                        }
                        else
                        {
                            materialTypeAssetCreator.SetPropertyValue(propertyId, enumValue);
                        }
                    }
                    break;
                    default:
                        materialTypeAssetCreator.SetPropertyValue(propertyId, property->m_value);
                        break;
                    }
                }
            }
            
            for (const AZStd::unique_ptr<PropertySet>& propertySubset : propertySet->m_propertySets)
            {
                propertyNameContext.push_back(propertySubset->m_name);

                bool success = BuildPropertyList(
                    materialTypeSourceFilePath,
                    materialTypeAssetCreator,
                    propertyNameContext,
                    propertySubset.get());

                propertyNameContext.pop_back();

                if (!success)
                {
                    return false;
                }
            }

            // We cannot create the MaterialFunctor until after all the properties are added because 
            // CreateFunctor() may need to look up properties in the MaterialPropertiesLayout
            for (auto& functorData : propertySet->m_materialFunctorSourceData)
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
                            materialTypeAssetCreator.ClaimShaderOptionOwnership(Name{optionName.GetCStr()});
                        }
                    }
                }
                else
                {
                    materialTypeAssetCreator.ReportError("Failed to create MaterialFunctor");
                    return false;
                }
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
                auto shaderAssetResult = AssetUtils::LoadAsset<ShaderAsset>(materialTypeSourceFilePath, shaderFile, 0);

                if (shaderAssetResult)
                {
                    auto shaderAsset = shaderAssetResult.GetValue();
                    auto optionsLayout = shaderAsset->GetShaderOptionGroupLayout();
                    ShaderOptionGroup options{ optionsLayout };
                    for (auto& iter : shaderRef.m_shaderOptionValues)
                    {
                        if (!options.SetValue(iter.first, iter.second))
                        {
                            return Failure();
                        }
                    }

                    materialTypeAssetCreator.AddShader(
                        shaderAsset, options.GetShaderVariantId(),
                        shaderRef.m_shaderTag.IsEmpty() ? Uuid::CreateRandom().ToString<AZ::Name>() : shaderRef.m_shaderTag);

                    // Gather UV names
                    const ShaderInputContract& shaderInputContract = shaderAsset->GetInputContract();
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
            
            for (const AZStd::unique_ptr<PropertySet>& propertySet : m_propertyLayout.m_propertySets)
            {
                AZStd::vector<AZStd::string> propertyNameContext;
                propertyNameContext.push_back(propertySet->m_name);
                bool success = BuildPropertyList(materialTypeSourceFilePath, materialTypeAssetCreator, propertyNameContext, propertySet.get());

                if (!success)
                {
                    return Failure();
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
