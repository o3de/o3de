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
#include <Atom/RPI.Edit/Material/MaterialPropertyValueSerializer.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAssetCreator.h>
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/MaterialVersionUpdate.h>
#include <Atom/RPI.Reflect/Material/MaterialNameContext.h>
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
                jsonContext->Serializer<JsonMaterialPropertyValueSerializer>()->HandlesType<MaterialPropertyValue>();
            }
            else if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<PropertyConnection>()->Version(3);
                serializeContext->Class<GroupDefinition>()->Version(4);
                serializeContext->Class<PropertyDefinition>()->Version(1);
                
                serializeContext->RegisterGenericType<AZStd::unique_ptr<PropertyGroup>>();
                serializeContext->RegisterGenericType<AZStd::unique_ptr<PropertyDefinition>>();
                serializeContext->RegisterGenericType<AZStd::vector<AZStd::unique_ptr<PropertyGroup>>>();
                serializeContext->RegisterGenericType<AZStd::vector<AZStd::unique_ptr<PropertyDefinition>>>();
                serializeContext->RegisterGenericType<PropertyConnectionList>();

                serializeContext->RegisterGenericType<MaterialVersionUpdate::Action::ActionDefinition>();
                serializeContext->RegisterGenericType<VersionUpdateActions>();

                serializeContext->Class<VersionUpdateDefinition>()
                    ->Version(2) // Generic actions based on string -> MaterialPropertyValue map
                    ->Field("toVersion", &VersionUpdateDefinition::m_toVersion)
                    ->Field("actions", &VersionUpdateDefinition::m_actions)
                    ;

                serializeContext->Class<ShaderVariantReferenceData>()
                    ->Version(2)
                    ->Field("file", &ShaderVariantReferenceData::m_shaderFilePath)
                    ->Field("tag", &ShaderVariantReferenceData::m_shaderTag)
                    ->Field("options", &ShaderVariantReferenceData::m_shaderOptionValues)
                    ;

                serializeContext->Class<PropertyGroup>()
                    ->Version(2)
                    ->Field("name", &PropertyGroup::m_name)
                    ->Field("displayName", &PropertyGroup::m_displayName)
                    ->Field("description", &PropertyGroup::m_description)
                    ->Field("shaderInputsPrefix", &PropertyGroup::m_shaderInputsPrefix)
                    ->Field("shaderOptionsPrefix", &PropertyGroup::m_shaderOptionsPrefix)
                    ->Field("properties", &PropertyGroup::m_properties)
                    ->Field("propertyGroups", &PropertyGroup::m_propertyGroups)
                    ->Field("functors", &PropertyGroup::m_materialFunctorSourceData)
                    ;

                serializeContext->Class<PropertyLayout>()
                    ->Version(3) // Added propertyGroups
                    ->Field("version", &PropertyLayout::m_versionOld)       //< @deprecated: preserved for backward compatibility, replaced by MaterialTypeSourceData::version
                    ->Field("groups", &PropertyLayout::m_groupsOld)         //< @deprecated: preserved for backward compatibility, replaced by propertyGroups
                    ->Field("properties", &PropertyLayout::m_propertiesOld) //< @deprecated: preserved for backward compatibility, replaced by propertyGroups
                    ->Field("propertyGroups", &PropertyLayout::m_propertyGroups)
                    ;

                serializeContext->RegisterGenericType<VersionUpdates>();
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
        
        /*static*/ MaterialTypeSourceData::PropertyGroup* MaterialTypeSourceData::PropertyGroup::AddPropertyGroup(AZStd::string_view name, AZStd::vector<AZStd::unique_ptr<PropertyGroup>>& toPropertyGroupList)
        {
            if (!MaterialPropertyId::CheckIsValidName(name))
            {
                return nullptr;
            }

            auto iter = AZStd::find_if(toPropertyGroupList.begin(), toPropertyGroupList.end(), [name](const AZStd::unique_ptr<PropertyGroup>& existingPropertyGroup)
                {
                    return existingPropertyGroup->m_name == name;
                });
            
            if (iter != toPropertyGroupList.end())
            {
                AZ_Error("Material source data", false, "PropertyGroup named '%.*s' already exists", AZ_STRING_ARG(name));
                return nullptr;
            }

            toPropertyGroupList.push_back(AZStd::make_unique<PropertyGroup>());
            toPropertyGroupList.back()->m_name = name;
            return toPropertyGroupList.back().get();
        }
        
        const AZStd::string& MaterialTypeSourceData::PropertyGroup::GetName() const
        {
            return m_name;
        }

        const AZStd::string& MaterialTypeSourceData::PropertyGroup::GetDisplayName() const
        {
            return m_displayName;
        }

        const AZStd::string& MaterialTypeSourceData::PropertyGroup::GetDescription() const
        {
            return m_description;
        }

        const MaterialTypeSourceData::PropertyList& MaterialTypeSourceData::PropertyGroup::GetProperties() const
        {
            return m_properties;
        }

        const AZStd::string& MaterialTypeSourceData::PropertyGroup::GetShaderInputsPrefix() const
        {
            return m_shaderInputsPrefix;
        }

        const AZStd::string& MaterialTypeSourceData::PropertyGroup::GetShaderOptionsPrefix() const
        {
            return m_shaderOptionsPrefix;
        }

        const AZStd::vector<AZStd::unique_ptr<MaterialTypeSourceData::PropertyGroup>>& MaterialTypeSourceData::PropertyGroup::GetPropertyGroups() const
        {
            return m_propertyGroups;
        }

        const AZStd::vector<Ptr<MaterialFunctorSourceDataHolder>>& MaterialTypeSourceData::PropertyGroup::GetFunctors() const
        {
            return m_materialFunctorSourceData;
        }

        void MaterialTypeSourceData::PropertyGroup::SetDisplayName(AZStd::string_view displayName)
        {
            m_displayName = displayName;
        }

        void MaterialTypeSourceData::PropertyGroup::SetDescription(AZStd::string_view description)
        {
            m_description = description;
        }

        MaterialTypeSourceData::PropertyDefinition* MaterialTypeSourceData::PropertyGroup::AddProperty(AZStd::string_view name)
        {
            if (!MaterialPropertyId::CheckIsValidName(name))
            {
                return nullptr;
            }

            auto propertyIter = AZStd::find_if(m_properties.begin(), m_properties.end(), [name](const AZStd::unique_ptr<PropertyDefinition>& existingProperty)
                {
                    return existingProperty->GetName() == name;    
                });

            if (propertyIter != m_properties.end())
            {
                AZ_Error("Material source data", false, "PropertyGroup '%s' already contains a property named '%.*s'", m_name.c_str(), AZ_STRING_ARG(name));
                return nullptr;
            }
            
            auto propertyGroupIter = AZStd::find_if(m_propertyGroups.begin(), m_propertyGroups.end(), [name](const AZStd::unique_ptr<PropertyGroup>& existingPropertyGroup)
                {
                    return existingPropertyGroup->m_name == name;
                });

            if (propertyGroupIter != m_propertyGroups.end())
            {
                AZ_Error("Material source data", false, "Property name '%.*s' collides with a PropertyGroup of the same name", AZ_STRING_ARG(name));
                return nullptr;
            }

            m_properties.emplace_back(AZStd::make_unique<PropertyDefinition>(name));
            return m_properties.back().get();
        }

        MaterialTypeSourceData::PropertyGroup* MaterialTypeSourceData::PropertyGroup::AddPropertyGroup(AZStd::string_view name)
        {
            auto iter = AZStd::find_if(m_properties.begin(), m_properties.end(), [name](const AZStd::unique_ptr<PropertyDefinition>& existingProperty)
                {
                    return existingProperty->GetName() == name;
                });

            if (iter != m_properties.end())
            {
                AZ_Error("Material source data", false, "PropertyGroup name '%.*s' collides with a Property of the same name", AZ_STRING_ARG(name));
                return nullptr;
            }

            return AddPropertyGroup(name, m_propertyGroups);
        }
        
        MaterialTypeSourceData::PropertyGroup* MaterialTypeSourceData::AddPropertyGroup(AZStd::string_view propertyGroupId)
        {
            AZStd::vector<AZStd::string_view> splitPropertyGroupId = SplitId(propertyGroupId);

            if (splitPropertyGroupId.size() == 1)
            {
                return PropertyGroup::AddPropertyGroup(propertyGroupId, m_propertyLayout.m_propertyGroups);
            }

            PropertyGroup* parentPropertyGroup = FindPropertyGroup(splitPropertyGroupId[0]);
            
            if (!parentPropertyGroup)
            {
                AZ_Error("Material source data", false, "PropertyGroup '%.*s' does not exists", AZ_STRING_ARG(splitPropertyGroupId[0]));
                return nullptr;
            }

            return parentPropertyGroup->AddPropertyGroup(splitPropertyGroupId[1]);
        }
        
        MaterialTypeSourceData::PropertyDefinition* MaterialTypeSourceData::AddProperty(AZStd::string_view propertyId)
        {
            AZStd::vector<AZStd::string_view> splitPropertyId = SplitId(propertyId);

            if (splitPropertyId.size() == 1)
            {
                AZ_Error("Material source data", false, "Property id '%.*s' is invalid. Properties must be added to a PropertyGroup (i.e. \"general.%.*s\").", AZ_STRING_ARG(propertyId), AZ_STRING_ARG(propertyId));
                return nullptr;
            }

            PropertyGroup* parentPropertyGroup = FindPropertyGroup(splitPropertyId[0]);
            
            if (!parentPropertyGroup)
            {
                AZ_Error("Material source data", false, "PropertyGroup '%.*s' does not exists", AZ_STRING_ARG(splitPropertyId[0]));
                return nullptr;
            }

            return parentPropertyGroup->AddProperty(splitPropertyId[1]);
        }
        
        const MaterialTypeSourceData::PropertyGroup* MaterialTypeSourceData::FindPropertyGroup(AZStd::span<const AZStd::string_view> parsedPropertyGroupId, AZStd::span<const AZStd::unique_ptr<PropertyGroup>> inPropertyGroupList) const
        {
            for (const auto& propertyGroup : inPropertyGroupList)
            {
                if (propertyGroup->m_name != parsedPropertyGroupId[0])
                {
                    continue;
                }
                else if (parsedPropertyGroupId.size() == 1)
                {
                    return propertyGroup.get();
                }
                else
                {
                    AZStd::span<const AZStd::string_view> subPath{parsedPropertyGroupId.begin() + 1, parsedPropertyGroupId.end()};

                    if (!subPath.empty())
                    {
                        const MaterialTypeSourceData::PropertyGroup* propertySubgroup = FindPropertyGroup(subPath, propertyGroup->m_propertyGroups);
                        if (propertySubgroup)
                        {
                            return propertySubgroup;
                        }
                    }
                }
            }

            return nullptr;
        }
        
        MaterialTypeSourceData::PropertyGroup* MaterialTypeSourceData::FindPropertyGroup(AZStd::span<AZStd::string_view> parsedPropertyGroupId, AZStd::span<AZStd::unique_ptr<PropertyGroup>> inPropertyGroupList)
        {
            return const_cast<PropertyGroup*>(const_cast<const MaterialTypeSourceData*>(this)->FindPropertyGroup(parsedPropertyGroupId, inPropertyGroupList));
        }

        const MaterialTypeSourceData::PropertyGroup* MaterialTypeSourceData::FindPropertyGroup(AZStd::string_view propertyGroupId) const
        {
            AZStd::vector<AZStd::string_view> tokens = TokenizeId(propertyGroupId);
            return FindPropertyGroup(tokens, m_propertyLayout.m_propertyGroups);
        }

        MaterialTypeSourceData::PropertyGroup* MaterialTypeSourceData::FindPropertyGroup(AZStd::string_view propertyGroupId)
        {
            AZStd::vector<AZStd::string_view> tokens = TokenizeId(propertyGroupId);
            return FindPropertyGroup(tokens, m_propertyLayout.m_propertyGroups);
        }
        
        const MaterialTypeSourceData::PropertyDefinition* MaterialTypeSourceData::FindProperty(
            AZStd::span<const AZStd::string_view> parsedPropertyId,
            AZStd::span<const AZStd::unique_ptr<PropertyGroup>> inPropertyGroupList) const
        {
            for (const auto& propertyGroup : inPropertyGroupList)
            {
                if (propertyGroup->m_name == parsedPropertyId[0])
                {
                    AZStd::span<const AZStd::string_view> subPath {parsedPropertyId.begin() + 1, parsedPropertyId.end()};

                    if (subPath.size() == 1)
                    {
                        for (AZStd::unique_ptr<PropertyDefinition>& property : propertyGroup->m_properties)
                        {
                            if (property->GetName() == subPath[0])
                            {
                                return property.get();
                            }
                        }
                    }
                    else if(subPath.size() > 1)
                    {
                        const MaterialTypeSourceData::PropertyDefinition* property = FindProperty(subPath, propertyGroup->m_propertyGroups);
                        if (property)
                        {
                            return property;
                        }
                    }
                }
            }

            return nullptr;
        }
        
        MaterialTypeSourceData::PropertyDefinition* MaterialTypeSourceData::FindProperty(AZStd::span<AZStd::string_view> parsedPropertyId, AZStd::span<AZStd::unique_ptr<PropertyGroup>> inPropertyGroupList)
        {
            return const_cast<MaterialTypeSourceData::PropertyDefinition*>(const_cast<const MaterialTypeSourceData*>(this)->FindProperty(parsedPropertyId, inPropertyGroupList));
        }

        const MaterialTypeSourceData::PropertyDefinition* MaterialTypeSourceData::FindProperty(AZStd::string_view propertyId) const
        {
            AZStd::vector<AZStd::string_view> tokens = TokenizeId(propertyId);
            return FindProperty(tokens, m_propertyLayout.m_propertyGroups);
        }
        
        MaterialTypeSourceData::PropertyDefinition* MaterialTypeSourceData::FindProperty(AZStd::string_view propertyId)
        {
            AZStd::vector<AZStd::string_view> tokens = TokenizeId(propertyId);
            return FindProperty(tokens, m_propertyLayout.m_propertyGroups);
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

        bool MaterialTypeSourceData::EnumeratePropertyGroups(const EnumeratePropertyGroupsCallback& callback, PropertyGroupStack& propertyGroupStack, const AZStd::vector<AZStd::unique_ptr<PropertyGroup>>& inPropertyGroupList) const
        {
            for (auto& propertyGroup : inPropertyGroupList)
            {
                propertyGroupStack.push_back(propertyGroup.get());

                if (!callback(propertyGroupStack))
                {
                    return false;  // Stop processing
                }
                
                if (!EnumeratePropertyGroups(callback, propertyGroupStack, propertyGroup->m_propertyGroups))
                {
                    return false; // Stop processing
                }

                propertyGroupStack.pop_back();
            }

            return true;
        }

        bool MaterialTypeSourceData::EnumeratePropertyGroups(const EnumeratePropertyGroupsCallback& callback) const
        {
            if (!callback)
            {
                return false;
            }

            PropertyGroupStack propertyGroupStack;
            return EnumeratePropertyGroups(callback, propertyGroupStack, m_propertyLayout.m_propertyGroups);
        }

        bool MaterialTypeSourceData::EnumerateProperties(const EnumeratePropertiesCallback& callback, MaterialNameContext nameContext, const AZStd::vector<AZStd::unique_ptr<PropertyGroup>>& inPropertyGroupList) const
        {
            for (auto& propertyGroup : inPropertyGroupList)
            {
                MaterialNameContext groupNameContext = nameContext;
                ExtendNameContext(groupNameContext, *propertyGroup);

                for (auto& property : propertyGroup->m_properties)
                {
                    if (!callback(property.get(), groupNameContext))
                    {
                        return false; // Stop processing
                    }
                }

                if (!EnumerateProperties(callback, groupNameContext, propertyGroup->m_propertyGroups))
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

            return EnumerateProperties(callback, {}, m_propertyLayout.m_propertyGroups);
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
                        PropertyGroup* propertyGroup = FindPropertyGroup(group.m_name);

                        if (!propertyGroup)
                        {
                            m_propertyLayout.m_propertyGroups.emplace_back(AZStd::make_unique<PropertyGroup>());
                            m_propertyLayout.m_propertyGroups.back()->m_name = group.m_name;
                            m_propertyLayout.m_propertyGroups.back()->m_displayName = group.m_displayName;
                            m_propertyLayout.m_propertyGroups.back()->m_description = group.m_description;
                            propertyGroup = m_propertyLayout.m_propertyGroups.back().get();
                        }

                        PropertyDefinition* newProperty = propertyGroup->AddProperty(propertyDefinition.GetName());
                        
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
            
            EnumerateProperties([&enumValues](const MaterialTypeSourceData::PropertyDefinition* property, const MaterialNameContext&)
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

        void MaterialTypeSourceData::ExtendNameContext(MaterialNameContext& nameContext, const MaterialTypeSourceData::PropertyGroup& propertyGroup)
        {
            nameContext.ExtendPropertyIdContext(propertyGroup.m_name);
            nameContext.ExtendShaderOptionContext(propertyGroup.m_shaderOptionsPrefix);
            nameContext.ExtendSrgInputContext(propertyGroup.m_shaderInputsPrefix);
        }
        
        /*static*/ MaterialNameContext MaterialTypeSourceData::MakeMaterialNameContext(const MaterialTypeSourceData::PropertyGroupStack& propertyGroupStack)
        {
            MaterialNameContext nameContext;
            for (auto& group : propertyGroupStack)
            {
                ExtendNameContext(nameContext, *group);
            }
            return nameContext;
        }

        MaterialPropertyValue MaterialTypeSourceData::ResolveSourceValue(
            const Name& propertyId,
            const MaterialPropertyValue& sourceValue,
            const AZStd::string& materialTypeSourceFilePath,
            const MaterialPropertiesLayout* materialPropertiesLayout,
            AZStd::function<void(const char*)> onError)
        {
            MaterialPropertyIndex propertyIndex = materialPropertiesLayout->FindPropertyIndex(propertyId);
            if (!propertyIndex.IsValid())
            {
                onError(AZStd::string::format("Could not resolve '%s': unknown property", propertyId.GetCStr()).c_str());
                return sourceValue;
            }
            const MaterialPropertyDescriptor* descriptor = materialPropertiesLayout->GetPropertyDescriptor(propertyIndex);

            switch (descriptor->GetDataType())
            {
            case MaterialPropertyDataType::Image:
            {
                Data::Asset<ImageAsset> imageAsset;

                MaterialUtils::GetImageAssetResult result = MaterialUtils::GetImageAssetReference(
                    imageAsset, materialTypeSourceFilePath, sourceValue.GetValue<AZStd::string>());

                if (result == MaterialUtils::GetImageAssetResult::Missing)
                {
                    onError(AZStd::string::format(
                                "Material property '%s': Could not find the image '%s'", propertyId.GetCStr(),
                                sourceValue.GetValue<AZStd::string>().c_str())
                                .c_str());
                }
                else
                {
                    return imageAsset;
                }
            }
            break;
            case MaterialPropertyDataType::Enum:
            {
                AZ::Name enumName = AZ::Name(sourceValue.GetValue<AZStd::string>());
                uint32_t enumValue = descriptor->GetEnumValue(enumName);
                if (enumValue == MaterialPropertyDescriptor::InvalidEnumValue)
                {
                    onError(AZStd::string::format(
                                "Material property '%s': Enum value '%s' couldn't be found",
                                propertyId.GetCStr(), enumName.GetCStr())
                                .c_str());
                }
                else
                {
                    return enumValue;
                }
            }
            break;
            }

            return sourceValue;
        }

        bool MaterialTypeSourceData::BuildPropertyList(
            const AZStd::string& materialTypeSourceFilePath,
            MaterialTypeAssetCreator& materialTypeAssetCreator,
            MaterialNameContext materialNameContext,
            const MaterialTypeSourceData::PropertyGroup* propertyGroup) const
        {
            if (!MaterialPropertyId::CheckIsValidName(propertyGroup->m_name))
            {
                return false;
            }

            ExtendNameContext(materialNameContext, *propertyGroup);

            for (const AZStd::unique_ptr<PropertyDefinition>& property : propertyGroup->m_properties)
            {
                // Register the property...

                if (!MaterialPropertyId::CheckIsValidName(property->GetName()))
                {
                    return false;
                }

                Name propertyId{property->GetName()};
                materialNameContext.ContextualizeProperty(propertyId);

                auto propertyGroupIter = AZStd::find_if(propertyGroup->GetPropertyGroups().begin(), propertyGroup->GetPropertyGroups().end(),
                    [&property](const AZStd::unique_ptr<PropertyGroup>& existingPropertyGroup)
                    {
                        return existingPropertyGroup->GetName() == property->GetName();
                    });

                if (propertyGroupIter != propertyGroup->GetPropertyGroups().end())
                {
                    AZ_Error("Material source data", false, "Material property '%s' collides with a PropertyGroup with the same ID.", propertyId.GetCStr());
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
                        Name fieldName{output.m_fieldName};
                        materialNameContext.ContextualizeSrgInput(fieldName);
                        materialTypeAssetCreator.ConnectMaterialPropertyToShaderInput(fieldName);
                        break;
                    }
                    case MaterialPropertyOutputType::ShaderOption:
                    {
                        Name fieldName{output.m_fieldName};
                        materialNameContext.ContextualizeShaderOption(fieldName);

                        if (output.m_shaderIndex >= 0)
                        {
                            materialTypeAssetCreator.ConnectMaterialPropertyToShaderOption(fieldName, output.m_shaderIndex);
                        }
                        else
                        {
                            materialTypeAssetCreator.ConnectMaterialPropertyToShaderOptions(fieldName);
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
                    materialTypeAssetCreator.ReportWarning("Default value for material property '%s' is invalid.", propertyId.GetCStr());
                }
                else
                {
                    // Resolve value if needed
                    MaterialPropertyValue resolvedValue = ResolveSourceValue(
                        propertyId, property->m_value, materialTypeSourceFilePath,
                        materialTypeAssetCreator.GetMaterialPropertiesLayout(),
                        [&](const char* message){ materialTypeAssetCreator.ReportError("%s", message); });
                    materialTypeAssetCreator.SetPropertyValue(propertyId, resolvedValue);
                }
            }
            
            for (const AZStd::unique_ptr<PropertyGroup>& propertySubgroup : propertyGroup->m_propertyGroups)
            {
                bool success = BuildPropertyList(
                    materialTypeSourceFilePath,
                    materialTypeAssetCreator,
                    materialNameContext,
                    propertySubgroup.get());

                if (!success)
                {
                    return false;
                }
            }

            // We cannot create the MaterialFunctor until after all the properties are added because 
            // CreateFunctor() may need to look up properties in the MaterialPropertiesLayout
            for (auto& functorData : propertyGroup->m_materialFunctorSourceData)
            {
                MaterialFunctorSourceData::FunctorResult result = functorData->CreateFunctor(
                    MaterialFunctorSourceData::RuntimeContext(
                        materialTypeSourceFilePath,
                        materialTypeAssetCreator.GetMaterialPropertiesLayout(),
                        materialTypeAssetCreator.GetMaterialShaderResourceGroupLayout(),
                        materialTypeAssetCreator.GetShaderCollection(),
                        &materialNameContext
                    )
                );

                if (result.IsSuccess())
                {
                    Ptr<MaterialFunctor>& functor = result.GetValue();
                    if (functor != nullptr)
                    {
                        materialTypeAssetCreator.AddMaterialFunctor(functor);

                        for (AZ::Name optionName : functorData->GetActualSourceData()->GetShaderOptionDependencies())
                        {
                            materialNameContext.ContextualizeShaderOption(optionName);
                            materialTypeAssetCreator.ClaimShaderOptionOwnership(optionName);
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
            
            for (const AZStd::unique_ptr<PropertyGroup>& propertyGroup : m_propertyLayout.m_propertyGroups) 
            {
                bool success = BuildPropertyList(materialTypeSourceFilePath, materialTypeAssetCreator, MaterialNameContext{}, propertyGroup.get());

                if (!success)
                {
                    return Failure();
                }
            }

            MaterialNameContext nameContext;

            // We cannot create the MaterialFunctor until after all the properties are added because 
            // CreateFunctor() may need to look up properties in the MaterialPropertiesLayout
            for (auto& functorData : m_materialFunctorSourceData)
            {
                MaterialFunctorSourceData::FunctorResult result = functorData->CreateFunctor(
                    MaterialFunctorSourceData::RuntimeContext(
                        materialTypeSourceFilePath,
                        materialTypeAssetCreator.GetMaterialPropertiesLayout(),
                        materialTypeAssetCreator.GetMaterialShaderResourceGroupLayout(),
                        materialTypeAssetCreator.GetShaderCollection(),
                        &nameContext
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

            // Set materialtype version and add each version update object. We do this at the end so
            // that the MaterialPropertiesLayout is known in order to resolve values within the
            // version updates (e.g. in setValue update actions).
            const MaterialPropertiesLayout* materialPropertiesLayout = materialTypeAssetCreator.GetMaterialPropertiesLayout();
            auto sourceDataResolver = [&](const Name& propertyId, const MaterialPropertyValue& sourceValue)
            {
                return ResolveSourceValue(
                    propertyId, sourceValue, materialTypeSourceFilePath, materialPropertiesLayout,
                    [&](const char* message){ materialTypeAssetCreator.ReportError("%s", message); });
            };
            // Set the version and add the version updates
            materialTypeAssetCreator.SetVersion(m_version);
            for (const auto& versionUpdate : m_versionUpdates)
            {
                MaterialVersionUpdate materialVersionUpdate{versionUpdate.m_toVersion};
                for (const auto& action : versionUpdate.m_actions)
                {
                    materialVersionUpdate.AddAction(action, sourceDataResolver);
                }
                materialTypeAssetCreator.AddVersionUpdate(materialVersionUpdate);
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
