/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyId.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/MaterialNameContext.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <AtomCore/Instance/Instance.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/Util/MaterialPropertyUtil.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Document/MaterialDocument.h>

namespace MaterialEditor
{
    void MaterialDocument::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MaterialDocument, AtomToolsFramework::AtomToolsDocument>()
                ->Version(0);
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<MaterialDocumentRequestBus>("MaterialDocumentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "materialeditor")
                ->Event("SetPropertyValue", &MaterialDocumentRequestBus::Events::SetPropertyValue)
                ->Event("GetPropertyValue", &MaterialDocumentRequestBus::Events::GetPropertyValue);
        }
    }

    MaterialDocument::MaterialDocument(const AZ::Crc32& toolId, const AtomToolsFramework::DocumentTypeInfo& documentTypeInfo)
        : AtomToolsFramework::AtomToolsDocument(toolId, documentTypeInfo)
    {
        MaterialDocumentRequestBus::Handler::BusConnect(m_id);
    }

    MaterialDocument::~MaterialDocument()
    {
        MaterialDocumentRequestBus::Handler::BusDisconnect();
        AZ::SystemTickBus::Handler::BusDisconnect();
    }

    AZ::Data::Asset<AZ::RPI::MaterialAsset> MaterialDocument::GetAsset() const
    {
        return m_materialAsset;
    }

    AZ::Data::Instance<AZ::RPI::Material> MaterialDocument::GetInstance() const
    {
        return m_materialInstance;
    }

    const AZ::RPI::MaterialSourceData* MaterialDocument::GetMaterialSourceData() const
    {
        return &m_materialSourceData;
    }

    const AZ::RPI::MaterialTypeSourceData* MaterialDocument::GetMaterialTypeSourceData() const
    {
        return &m_materialTypeSourceData;
    }

    void MaterialDocument::SetPropertyValue(const AZStd::string& propertyId, const AZStd::any& value)
    {
        const AZ::Name propertyName(propertyId);

        AtomToolsFramework::DynamicProperty* foundProperty = {};
        TraverseGroups(m_groups, [&, this](auto& group) {
            for (auto& property : group->m_properties)
            {
                if (property.GetId() == propertyName)
                {
                    foundProperty = &property;

                    if (m_materialInstance)
                    {
                        // This first converts to an acceptable runtime type in case the value came from script
                        const AZ::RPI::MaterialPropertyValue propertyValue = AtomToolsFramework::ConvertToRuntimeType(value);

                        property.SetValue(AtomToolsFramework::ConvertToEditableType(propertyValue));

                        const auto propertyIndex = m_materialInstance->FindPropertyIndex(propertyName);
                        if (!propertyIndex.IsNull())
                        {
                            if (m_materialInstance->SetPropertyValue(propertyIndex, propertyValue))
                            {
                                AZ::RPI::MaterialPropertyFlags dirtyFlags = m_materialInstance->GetPropertyDirtyFlags();

                                Recompile();
                                RunEditorMaterialFunctors(dirtyFlags);
                            }
                        }
                    }

                    AtomToolsFramework::AtomToolsDocumentNotificationBus::Event(
                        m_toolId, &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentObjectInfoChanged, m_id,
                        GetObjectInfoFromDynamicPropertyGroup(group.get()), false);

                    AtomToolsFramework::AtomToolsDocumentNotificationBus::Event(
                        m_toolId, &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentModified, m_id);
                    return false;
                }
            }
            return true;
        });

        if (!foundProperty)
        {
            AZ_Error("MaterialDocument", false, "Document property could not be found: '%s'.", propertyId.c_str());
        }
    }

    const AZStd::any& MaterialDocument::GetPropertyValue(const AZStd::string& propertyId) const
    {
        auto property = FindProperty(AZ::Name(propertyId));
        if (!property)
        {
            AZ_Error("MaterialDocument", false, "Document property could not be found: '%s'.", propertyId.c_str());
            return m_invalidValue;
        }

        return property->GetValue();
    }

    AtomToolsFramework::DocumentTypeInfo MaterialDocument::BuildDocumentTypeInfo()
    {
        AtomToolsFramework::DocumentTypeInfo documentType;
        documentType.m_documentTypeName = "Material";
        documentType.m_documentFactoryCallback = [](const AZ::Crc32& toolId, const AtomToolsFramework::DocumentTypeInfo& documentTypeInfo) {
            return aznew MaterialDocument(toolId, documentTypeInfo); };
        documentType.m_supportedExtensionsToCreate.push_back({ "Material Type", AZ::RPI::MaterialTypeSourceData::Extension });
        documentType.m_supportedExtensionsToCreate.push_back({ "Material", AZ::RPI::MaterialSourceData::Extension });
        documentType.m_supportedExtensionsToOpen.push_back({ "Material Type", AZ::RPI::MaterialTypeSourceData::Extension });
        documentType.m_supportedExtensionsToOpen.push_back({ "Material", AZ::RPI::MaterialSourceData::Extension });
        documentType.m_supportedExtensionsToSave.push_back({ "Material", AZ::RPI::MaterialSourceData::Extension });
        documentType.m_defaultDocumentTemplate =
            AtomToolsFramework::GetPathWithoutAlias(AtomToolsFramework::GetSettingsValue<AZStd::string>(
                "/O3DE/Atom/MaterialEditor/DefaultMaterialType",
                "@gemroot:Atom_Feature_Common@/Assets/Materials/Types/StandardPBR.materialtype"));
        return documentType;
    }

    AtomToolsFramework::DocumentObjectInfoVector MaterialDocument::GetObjectInfo() const
    {
        AtomToolsFramework::DocumentObjectInfoVector objects = AtomToolsDocument::GetObjectInfo();
        objects.reserve(objects.size() + m_groups.size());

        for (const auto& group : m_groups)
        {
            objects.push_back(AZStd::move(GetObjectInfoFromDynamicPropertyGroup(group.get())));
        }

        return objects;
    }

    bool MaterialDocument::Save()
    {
        if (!AtomToolsDocument::Save())
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

        // populate sourceData with modified or overridden properties and save object
        AZ::RPI::MaterialSourceData sourceData;
        if (m_materialAsset.IsReady() &&
            m_materialAsset->GetMaterialTypeAsset().IsReady())
        {
            sourceData.m_materialTypeVersion = m_materialAsset->GetMaterialTypeAsset()->GetVersion();
        }
        sourceData.m_materialType = AtomToolsFramework::GetPathToExteralReference(m_absolutePath, m_materialSourceData.m_materialType);
        sourceData.m_parentMaterial = AtomToolsFramework::GetPathToExteralReference(m_absolutePath, m_materialSourceData.m_parentMaterial);
        auto propertyFilter = [](const AtomToolsFramework::DynamicProperty& property) {
            return !AtomToolsFramework::ArePropertyValuesEqual(property.GetValue(), property.GetConfig().m_parentValue);
        };

        if (!SaveSourceData(sourceData, propertyFilter))
        {
            return SaveFailed();
        }

        // after saving, reset to a clean state
        TraverseGroups(m_groups, [&](auto& group) {
            for (auto& property : group->m_properties)
            {
                auto propertyConfig = property.GetConfig();
                propertyConfig.m_originalValue = property.GetValue();
                property.SetConfig(propertyConfig);
            }
            return true;
        });
        return SaveSucceeded();
    }

    bool MaterialDocument::SaveAsCopy(const AZStd::string& savePath)
    {
        if (!AtomToolsDocument::SaveAsCopy(savePath))
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

        // populate sourceData with modified or overridden properties and save object
        AZ::RPI::MaterialSourceData sourceData;
        if (m_materialAsset.IsReady() &&
            m_materialAsset->GetMaterialTypeAsset().IsReady())
        {
            sourceData.m_materialTypeVersion = m_materialAsset->GetMaterialTypeAsset()->GetVersion();
        }
        sourceData.m_materialType = AtomToolsFramework::GetPathToExteralReference(m_savePathNormalized, m_materialSourceData.m_materialType);
        sourceData.m_parentMaterial = AtomToolsFramework::GetPathToExteralReference(m_savePathNormalized, m_materialSourceData.m_parentMaterial);
        auto propertyFilter = [](const AtomToolsFramework::DynamicProperty& property) {
            return !AtomToolsFramework::ArePropertyValuesEqual(property.GetValue(), property.GetConfig().m_parentValue);
        };

        if (!SaveSourceData(sourceData, propertyFilter))
        {
            return SaveFailed();
        }

        // If the document is saved to a new file we need to reopen the new document to update assets, paths, property deltas.
        if (!Open(m_savePathNormalized))
        {
            return SaveFailed();
        }

        return SaveSucceeded();
    }

    bool MaterialDocument::SaveAsChild(const AZStd::string& savePath)
    {
        if (!AtomToolsDocument::SaveAsChild(savePath))
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

        // populate sourceData with modified or overridden properties and save object
        AZ::RPI::MaterialSourceData sourceData;
        if (m_materialAsset.IsReady() &&
            m_materialAsset->GetMaterialTypeAsset().IsReady())
        {
            sourceData.m_materialTypeVersion = m_materialAsset->GetMaterialTypeAsset()->GetVersion();
        }
        sourceData.m_materialType = AtomToolsFramework::GetPathToExteralReference(m_savePathNormalized, m_materialSourceData.m_materialType);

        // Only assign a parent path if the source was a .material
        if (AzFramework::StringFunc::Path::IsExtension(m_absolutePath.c_str(), AZ::RPI::MaterialSourceData::Extension))
        {
            sourceData.m_parentMaterial = AtomToolsFramework::GetPathToExteralReference(m_savePathNormalized, m_absolutePath);
        }

        auto propertyFilter = [](const AtomToolsFramework::DynamicProperty& property) {
            return !AtomToolsFramework::ArePropertyValuesEqual(property.GetValue(), property.GetConfig().m_originalValue);
        };

        if (!SaveSourceData(sourceData, propertyFilter))
        {
            return SaveFailed();
        }

        // If the document is saved to a new file we need to reopen the new document to update assets, paths, property deltas.
        if (!Open(m_savePathNormalized))
        {
            return SaveFailed();
        }

        return SaveSucceeded();
    }

    bool MaterialDocument::IsModified() const
    {
        bool result = false;
        TraverseGroups(m_groups, [&](auto& group) {
            for (auto& property : group->m_properties)
            {
                if (!AtomToolsFramework::ArePropertyValuesEqual(property.GetValue(), property.GetConfig().m_originalValue))
                {
                    result = true;
                    return false;
                }
            }
            return true;
        });
        return result;
    }

    bool MaterialDocument::CanSaveAsChild() const
    {
        return true;
    }

    bool MaterialDocument::BeginEdit()
    {
        // Save the current properties as a momento for undo before any changes are applied
        m_propertyValuesBeforeEdit.clear();
        TraverseGroups(m_groups, [this](auto& group) {
            for (auto& property : group->m_properties)
            {
                m_propertyValuesBeforeEdit[property.GetId()] = property.GetValue();
            }
            return true;
        });
        return true;
    }

    bool MaterialDocument::EndEdit()
    {
        PropertyValueMap propertyValuesForUndo;
        PropertyValueMap propertyValuesForRedo;

        // After editing has completed, check to see if properties have changed so the deltas can be recorded in the history
        for (const auto& propertyBeforeEditPair : m_propertyValuesBeforeEdit)
        {
            const auto& propertyName = propertyBeforeEditPair.first;
            const auto& propertyValueForUndo = propertyBeforeEditPair.second;
            const auto& propertyValueForRedo = GetPropertyValue(propertyName.GetStringView());
            if (!AtomToolsFramework::ArePropertyValuesEqual(propertyValueForUndo, propertyValueForRedo))
            {
                propertyValuesForUndo[propertyName] = propertyValueForUndo;
                propertyValuesForRedo[propertyName] = propertyValueForRedo;
            }
        }

        if (!propertyValuesForUndo.empty() && !propertyValuesForRedo.empty())
        {
            AddUndoRedoHistory(
                [this, propertyValuesForUndo]() { RestorePropertyValues(propertyValuesForUndo); },
                [this, propertyValuesForRedo]() { RestorePropertyValues(propertyValuesForRedo); });
        }

        m_propertyValuesBeforeEdit.clear();
        return true;
    }

    void MaterialDocument::OnSystemTick()
    {
        if (m_compilePending)
        {
            if (m_materialInstance && m_materialInstance->Compile())
            {
                m_compilePending = false;
                AZ::SystemTickBus::Handler::BusDisconnect();
            }
        }
    }

    bool MaterialDocument::SaveSourceData(AZ::RPI::MaterialSourceData& sourceData, PropertyFilterFunction propertyFilter) const
    {
        bool addPropertiesResult = true;

        // populate sourceData with properties that meet the filter
        m_materialTypeSourceData.EnumerateProperties([&](const auto& propertyDefinition, const AZ::RPI::MaterialNameContext& nameContext) {

            AZ::Name propertyId{propertyDefinition->GetName()};
            nameContext.ContextualizeProperty(propertyId);

            const auto property = FindProperty(propertyId);
            if (property && propertyFilter(*property))
            {
                AZ::RPI::MaterialPropertyValue propertyValue = AtomToolsFramework::ConvertToRuntimeType(property->GetValue());
                if (propertyValue.IsValid())
                {
                    if (!AtomToolsFramework::ConvertToExportFormat(m_savePathNormalized, propertyId, *propertyDefinition, propertyValue))
                    {
                        AZ_Error("MaterialDocument", false, "Document property could not be converted: '%s' in '%s'.", propertyId.GetCStr(), m_absolutePath.c_str());
                        addPropertiesResult = false;
                        return false;
                    }
                    
                    sourceData.SetPropertyValue(propertyId, propertyValue);
                }
            }
            return true;
        });

        if (!addPropertiesResult)
        {
            AZ_Error("MaterialDocument", false, "Document properties could not be saved: '%s'.", m_savePathNormalized.c_str());
            return false;
        }

        // Copy the description property to the outgoing source data
        if (const AZStd::any descriptionProperty = GetPropertyValue("overview.materialDescription");
            descriptionProperty.is<AZStd::string>())
        {
            sourceData.m_description = AZStd::any_cast<AZStd::string>(descriptionProperty);
        }

        if (!AZ::RPI::JsonUtils::SaveObjectToFile(m_savePathNormalized, sourceData))
        {
            AZ_Error("MaterialDocument", false, "Document could not be saved: '%s'.", m_savePathNormalized.c_str());
            return false;
        }

        return true;
    }

    bool MaterialDocument::Open(const AZStd::string& loadPath)
    {
        if (!AtomToolsDocument::Open(loadPath))
        {
            return false;
        }

        // The material document can load both material source data and material type source data files. Saving material type documents is
        // not supported but they can be used to save a child or create a new material from the material type. This could also be extended
        // to load material product assets, like the material instance editor on the material component. Those would also not be savable
        // but could be used to create material source file, like the material component UI.
        if (AzFramework::StringFunc::Path::IsExtension(m_absolutePath.c_str(), AZ::RPI::MaterialSourceData::Extension))
        {
            if (!LoadMaterialSourceData())
            {
                return OpenFailed();
            }
        }
        else if (AzFramework::StringFunc::Path::IsExtension(m_absolutePath.c_str(), AZ::RPI::MaterialTypeSourceData::Extension))
        {
            if (!LoadMaterialTypeSourceData())
            {
                return OpenFailed();
            }
        }
        else
        {
            AZ_Error("MaterialDocument", false, "Document extension not supported: '%s'.", m_absolutePath.c_str());
            return OpenFailed();
        }
        
        const bool elevateWarnings = false;

        // In order to support automation, general usability, and 'save as' functionality, the user must not have to wait
        // for their JSON file to be cooked by the asset processor before opening or editing it.
        // We need to reduce or remove dependency on the asset processor. In order to get around the bottleneck for now,
        // we can create the asset dynamically from the source data.
        // Long term, the material document should not be concerned with assets at all. The viewport window should be the
        // only thing concerned with assets or instances.
        auto materialAssetResult = m_materialSourceData.CreateMaterialAssetFromSourceData(
            AZ::Uuid::CreateRandom(), m_absolutePath, elevateWarnings, &m_sourceDependencies);
        if (!materialAssetResult)
        {
            AZ_Error("MaterialDocument", false, "Material asset could not be created from source data: '%s'.", m_absolutePath.c_str());
            return OpenFailed();
        }

        m_materialAsset = materialAssetResult.GetValue();
        if (!m_materialAsset.IsReady())
        {
            AZ_Error("MaterialDocument", false, "Material asset is not ready: '%s'.", m_absolutePath.c_str());
            return OpenFailed();
        }

        const auto& materialTypeAsset = m_materialAsset->GetMaterialTypeAsset();
        if (!materialTypeAsset.IsReady())
        {
            AZ_Error("MaterialDocument", false, "Material type asset is not ready: '%s'.", m_absolutePath.c_str());
            return OpenFailed();
        }

        // The parent material asset is only needed to retrieve property values for comparison.
        AZStd::span<const AZ::RPI::MaterialPropertyValue> parentPropertyValues = materialTypeAsset->GetDefaultPropertyValues();
        AZ::Data::Asset<AZ::RPI::MaterialAsset> parentMaterialAsset;
        if (!m_materialSourceData.m_parentMaterial.empty())
        {
            AZ::RPI::MaterialSourceData parentMaterialSourceData;
            auto loadResult = AZ::RPI::MaterialUtils::LoadMaterialSourceData(m_materialSourceData.m_parentMaterial);
            if (!loadResult)
            {
                AZ_Error("MaterialDocument", false, "Material parent source data could not be loaded for: '%s'.", m_materialSourceData.m_parentMaterial.c_str());
                return OpenFailed();
            }

            parentMaterialSourceData = loadResult.TakeValue();

            const auto parentMaterialAssetIdResult = AZ::RPI::AssetUtils::MakeAssetId(m_materialSourceData.m_parentMaterial, 0);
            if (!parentMaterialAssetIdResult)
            {
                AZ_Error("MaterialDocument", false, "Material parent asset ID could not be created: '%s'.", m_materialSourceData.m_parentMaterial.c_str());
                return OpenFailed();
            }

            // In order to avoid reliance on the asset processor, the material asset is generated in memory, directly from source files.
            auto parentMaterialAssetResult = parentMaterialSourceData.CreateMaterialAssetFromSourceData(
                parentMaterialAssetIdResult.GetValue(), m_materialSourceData.m_parentMaterial, true);
            if (!parentMaterialAssetResult)
            {
                AZ_Error("MaterialDocument", false, "Material parent asset could not be created from source data: '%s'.", m_materialSourceData.m_parentMaterial.c_str());
                return OpenFailed();
            }

            parentMaterialAsset = parentMaterialAssetResult.GetValue();
            parentPropertyValues = parentMaterialAsset->GetPropertyValues();
        }

        // A material instance needs to be created from the loaded asset to execute functors and be able to modify properties in real time
        // on the object in the viewport. Now that there is much better support for hot reloading, and material assets cook fairly
        // quickly, this direct connection to the viewport instance may not be required. It will still be required for functors. The
        // instance will fail to create a new document will not open if the material asset has bad texture or material type references.
        m_materialInstance = AZ::RPI::Material::Create(m_materialAsset);
        if (!m_materialInstance)
        {
            AZ_Error("MaterialDocument", false, "Material instance could not be created: '%s'.", m_absolutePath.c_str());
            return OpenFailed();
        }

        // Pipeline State Object changes are always allowed in the material editor because it only runs on developer systems where such
        // changes are supported at runtime.
        m_materialInstance->SetPsoHandlingOverride(AZ::RPI::MaterialPropertyPsoHandling::Allowed);

        // Inserting hardcoded properties to display material type, parent material, description, UV set names, and other information at the
        // top of the inspector. Dynamic properties were originally created to generically adapt and edit JSON and other non-standard
        // reflected data using the RPE. Most of these hardcoded properties are readonly. As that changes, it may be cleaner to add
        // explicit functions and reflection for things that are more complicated to edit like parent material and material type.
        auto createHeadingPropertyConfig = [](const AZStd::string& group, const AZStd::string& name, const AZStd::string& description,
            const AZStd::any& value, bool readOnly)
        {
            AtomToolsFramework::DynamicPropertyConfig propertyConfig;
            propertyConfig.m_name = name;
            propertyConfig.m_displayName = AtomToolsFramework::GetDisplayNameFromText(propertyConfig.m_name);
            propertyConfig.m_groupName = group;
            propertyConfig.m_groupDisplayName = AtomToolsFramework::GetDisplayNameFromText(propertyConfig.m_groupName);
            propertyConfig.m_id = propertyConfig.m_groupName + "." + name;
            propertyConfig.m_description = description;
            propertyConfig.m_parentValue = propertyConfig.m_originalValue = propertyConfig.m_defaultValue = value;
            propertyConfig.m_readOnly = readOnly;
            propertyConfig.m_showThumbnail = true;
            return propertyConfig;
        };

        m_groups.emplace_back(aznew AtomToolsFramework::DynamicPropertyGroup);
        m_groups.back()->m_name = "overview";
        m_groups.back()->m_displayName = "Overview";
        m_groups.back()->m_description = "Overview of the current material and its dependencies";

        m_groups.back()->m_properties.emplace_back(createHeadingPropertyConfig(
            "overview",
            "materialType",
            AZStd::string::format(
                "The material type defines the layout, properties, default values, shader connections, and other data needed to create and "
                "edit a material.\n\nDescription of %s:\n%s",
                AtomToolsFramework::GetDisplayNameFromPath(m_materialSourceData.m_materialType).c_str(),
                m_materialTypeSourceData.m_description.c_str()),
            AZStd::any(materialTypeAsset),
            true));

        m_groups.back()->m_properties.emplace_back(createHeadingPropertyConfig(
            "overview",
            "parentMaterial",
            "The parent material provides an initial configuration whose properties are inherited and overriden by a derived material.",
            AZStd::any(parentMaterialAsset),
            true));

        m_groups.back()->m_properties.emplace_back(createHeadingPropertyConfig(
            "overview",
            "materialDescription",
            "Description of the selected material.",
            AZStd::any(m_materialSourceData.m_description),
            false));

        // Inserting a hard coded property group to display UV channels specified in the material type.
        m_groups.emplace_back(aznew AtomToolsFramework::DynamicPropertyGroup);
        m_groups.back()->m_name = UvGroupName;
        m_groups.back()->m_displayName = "UV Sets";
        m_groups.back()->m_description = "UV set names in this material, which can be renamed to match those in the model.";

        const AZ::RPI::MaterialUvNameMap& uvNameMap = materialTypeAsset->GetUvNameMap();
        for (const AZ::RPI::UvNamePair& uvNamePair : uvNameMap)
        {
            const AZStd::string shaderInput = uvNamePair.m_shaderInput.ToString();
            const AZStd::string uvName = uvNamePair.m_uvName.GetStringView();
            m_groups.back()->m_properties.emplace_back(createHeadingPropertyConfig(UvGroupName, shaderInput, shaderInput, AZStd::any(uvName), true));
        }

        // Populate the property map from a combination of source data and assets
        // Assets must still be used for now because they contain the final accumulated value after all other materials
        // in the hierarchy are applied
        bool enumerateResult = m_materialTypeSourceData.EnumeratePropertyGroups(
            [this, &parentPropertyValues](const AZ::RPI::MaterialTypeSourceData::PropertyGroupStack& propertyGroupStack)
            {
                using namespace AZ::RPI;

                const MaterialTypeSourceData::PropertyGroup* propertyGroup = propertyGroupStack.back();
                
                MaterialNameContext groupNameContext = MaterialTypeSourceData::MakeMaterialNameContext(propertyGroupStack);

                if (!AddEditorMaterialFunctors(propertyGroup->GetFunctors(), groupNameContext))
                {
                    return false;
                }

                // Build a container of all of the group and display names accumulated while enumerating the group hierarchy. These will be
                // joined together for assembling full property IDs and group display names.
                AZStd::vector<AZStd::string> groupNameVector;
                groupNameVector.reserve(propertyGroupStack.size());

                AZStd::vector<AZStd::string> groupDisplayNameVector;
                groupDisplayNameVector.reserve(propertyGroupStack.size());

                for (auto& propertyGroupStackItem : propertyGroupStack)
                {
                    groupNameVector.push_back(propertyGroupStackItem->GetName());
                    groupDisplayNameVector.push_back(propertyGroupStackItem->GetDisplayName());
                }

                // Create a dynamic property group that will be managed by the document and used to display the properties in the inspector.
                AZStd::shared_ptr<AtomToolsFramework::DynamicPropertyGroup> dynamicPropertyGroup;
                dynamicPropertyGroup.reset(aznew AtomToolsFramework::DynamicPropertyGroup);

                // Copy details about this property group from the material type property group definition. Recombine the group name and
                // display name vectors so that the complete hierarchy will be displayed in the UI and available for creating property IDs.
                AzFramework::StringFunc::Join(dynamicPropertyGroup->m_name, groupNameVector.begin(), groupNameVector.end(), ".");
                AzFramework::StringFunc::Join(dynamicPropertyGroup->m_displayName, groupDisplayNameVector.begin(), groupDisplayNameVector.end(), " | ");

                if (dynamicPropertyGroup->m_displayName.empty())
                {
                    dynamicPropertyGroup->m_displayName =
                        !propertyGroup->GetDisplayName().empty() ? propertyGroup->GetDisplayName() : propertyGroup->GetName();
                }

                dynamicPropertyGroup->m_description = propertyGroup->GetDescription();
                if (dynamicPropertyGroup->m_description.empty())
                {
                    dynamicPropertyGroup->m_description = dynamicPropertyGroup->m_displayName;
                }

                // All of the material type properties must be adapted for display in the ui. This is done by converting them into a dynamic
                // property class that can be used to display and edit multiple types.
                for (const auto& propertyDefinition : propertyGroup->GetProperties())
                {
                    AtomToolsFramework::DynamicPropertyConfig propertyConfig;

                    // The property ID must be set up before calling the function to convert the rest of the material type property
                    // definition into the dynamic property config. The dynamic property config will set up a description that includes the
                    // ID.
                    propertyConfig.m_id = propertyDefinition->GetName();
                    groupNameContext.ContextualizeProperty(propertyConfig.m_id);

                    // A valid property index is required to look up property values in the material type and material asset property vectors.
                    const auto& propertyIndex = m_materialAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(propertyConfig.m_id);
                    const bool propertyIndexInBounds =
                        propertyIndex.IsValid() && propertyIndex.GetIndex() < m_materialAsset->GetPropertyValues().size();

                    AZ_Warning(
                        "MaterialDocument",
                        propertyIndexInBounds,
                        "Failed to add material property '%s' to document '%s'.",
                        propertyConfig.m_id.GetCStr(),
                        m_absolutePath.c_str());

                    if (propertyIndexInBounds)
                    {
                        // Utility function converts most attributes from the property definition into a dynamic property config.
                        AtomToolsFramework::ConvertToPropertyConfig(propertyConfig, *propertyDefinition);

                        // The utility function assigns a description from the property definition along with its name and display name.
                        // This will be displayed as the tooltip when dragging over the property in the inspector UI. The description is
                        // extended here so that the tooltip will display an image and additional information about the indicator that
                        // appears when properties are modified. The tooltip will automatically interpret the embedded HTML and display the
                        // image and formatting.
                        propertyConfig.m_description +=
                            "\n\n<img src=\':/Icons/changed_property.svg\'> An indicator icon will be shown to the left of properties with "
                            "overridden values that are different from the parent material, or material type if there is no parent.\n";

                        // The dynamic property uses the group name and display name to forward as attributes to the RPE and property asset
                        // control. The control will then use the attributes to display a context sensitive title when opening the asset
                        // picker for textures and other assets. Rather than using strings, this data could also be specified using
                        // AZStd::function.
                        propertyConfig.m_groupName = dynamicPropertyGroup->m_name;
                        propertyConfig.m_groupDisplayName = dynamicPropertyGroup->m_displayName;

                        // Enabling thumbnails will display a preview image next to an asset property in the RPE, if one is available.
                        propertyConfig.m_showThumbnail = true;

                        // Multiple values are recorded for the property, including the original value, default value, and parent value.
                        // These values are compared against each other to determine if an indicator needs to be displayed in the property
                        // inspector as well as which values get saved with the material.
                        propertyConfig.m_originalValue =
                            AtomToolsFramework::ConvertToEditableType(m_materialAsset->GetPropertyValues()[propertyIndex.GetIndex()]);
                        propertyConfig.m_parentValue =
                            AtomToolsFramework::ConvertToEditableType(parentPropertyValues[propertyIndex.GetIndex()]);

                        // The data change callback is invoked whenever the properties are modified in the inspector. The changes will be
                        // stored in the dynamic property automatically but need to be processed and applied to the material instance that's
                        // displayed in the viewport. This is also necessary to update and rerun functors.
                        propertyConfig.m_dataChangeCallback = [documentId = m_id, propertyId = propertyConfig.m_id](const AZStd::any& value)
                        {
                            MaterialDocumentRequestBus::Event(
                                documentId, &MaterialDocumentRequestBus::Events::SetPropertyValue, propertyId.GetStringView(), value);
                            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
                        };

                        dynamicPropertyGroup->m_properties.push_back(AtomToolsFramework::DynamicProperty(propertyConfig));
                    }
                }

                // The group will not be added if no properties were added to it.
                if (!dynamicPropertyGroup->m_properties.empty())
                {
                    m_groups.push_back(dynamicPropertyGroup);
                }
                return true;
            });

        if (!enumerateResult)
        {
            return OpenFailed();
        }

        // Add material functors that are in the top-level functors list.
        AZ::RPI::MaterialNameContext materialNameContext; // There is no name context for top-level functors, only functors inside PropertyGroups
        if (!AddEditorMaterialFunctors(m_materialTypeSourceData.m_materialFunctorSourceData, materialNameContext))
        {
            return OpenFailed();
        }

        AZ::RPI::MaterialPropertyFlags dirtyFlags;
        dirtyFlags.set(); // Mark all properties as dirty since we just loaded the material and need to initialize property visibility
        RunEditorMaterialFunctors(dirtyFlags);

        return OpenSucceeded();
    }

    void MaterialDocument::Clear()
    {
        AtomToolsFramework::AtomToolsDocument::Clear();

        AZ::SystemTickBus::Handler::BusDisconnect();

        m_materialAsset = {};
        m_materialInstance = {};
        m_compilePending = {};
        m_groups.clear();
        m_editorFunctors.clear();
        m_materialTypeSourceData = AZ::RPI::MaterialTypeSourceData();
        m_materialSourceData = AZ::RPI::MaterialSourceData();
        m_propertyValuesBeforeEdit.clear();
    }

    bool MaterialDocument::ReopenRecordState()
    {
        m_propertyValuesBeforeReopen.clear();
        TraverseGroups(m_groups, [this](auto& group) {
            for (auto& property : group->m_properties)
            {
                if (!AtomToolsFramework::ArePropertyValuesEqual(property.GetValue(), property.GetConfig().m_parentValue))
                {
                    m_propertyValuesBeforeReopen[property.GetId()] = property.GetValue();
                }
            }
            return true;
        });
        return AtomToolsDocument::ReopenRecordState();
    }

    bool MaterialDocument::ReopenRestoreState()
    {
        RestorePropertyValues(m_propertyValuesBeforeReopen);
        m_propertyValuesBeforeReopen.clear();
        return AtomToolsDocument::ReopenRestoreState();
    }

    void MaterialDocument::Recompile()
    {
        if (!m_compilePending)
        {
            AZ::SystemTickBus::Handler::BusConnect();
            m_compilePending = true;
        }
    }

    bool MaterialDocument::LoadMaterialSourceData()
    {
        auto loadResult = AZ::RPI::MaterialUtils::LoadMaterialSourceData(m_absolutePath);
        if (!loadResult)
        {
            AZ_Error("MaterialDocument", false, "Material source data could not be loaded: '%s'.", m_absolutePath.c_str());
            return false;
        }

        m_materialSourceData = loadResult.TakeValue();

        // We always need the absolute path for the material type and parent material to load source data and resolving
        // relative paths when saving. This will convert and store them as absolute paths for use within the document.
        m_materialSourceData.m_parentMaterial =
            AZ::RPI::AssetUtils::ResolvePathReference(m_absolutePath, m_materialSourceData.m_parentMaterial);
        m_materialSourceData.m_materialType =
            AZ::RPI::AssetUtils::ResolvePathReference(m_absolutePath, m_materialSourceData.m_materialType);
        // If the material was previously saved with a reference to a material pipeline generated material type in the intermediate asset
        // folder, attempt to redirect to the original source material type.
        m_materialSourceData.m_materialType =
            AZ::RPI::MaterialUtils::PredictOriginalMaterialTypeSourcePath(m_materialSourceData.m_materialType);

        // Load the material type source data which provides the layout and default values of all of the properties
        auto materialTypeOutcome = AZ::RPI::MaterialUtils::LoadMaterialTypeSourceData(m_materialSourceData.m_materialType);
        if (!materialTypeOutcome.IsSuccess())
        {
            AZ_Error("MaterialDocument", false, "Material type source data could not be loaded: '%s'.", m_materialSourceData.m_materialType.c_str());
            return false;
        }

        m_materialTypeSourceData = materialTypeOutcome.TakeValue();
        return true;
    }

    bool MaterialDocument::LoadMaterialTypeSourceData()
    {
        // A material document can be created or loaded from material or material type source data. If we are attempting to load
        // material type source data then the material source data object can be created just by referencing the document path as the
        // material type path.
        auto materialTypeOutcome = AZ::RPI::MaterialUtils::LoadMaterialTypeSourceData(m_absolutePath);
        if (!materialTypeOutcome.IsSuccess())
        {
            AZ_Error("MaterialDocument", false, "Material type source data could not be loaded: '%s'.", m_absolutePath.c_str());
            return false;
        }

        m_materialTypeSourceData = materialTypeOutcome.TakeValue();

        // We are storing absolute paths in the loaded version of the source data so that the files can be resolved at all times.
        m_materialSourceData.m_materialType = m_absolutePath;
        m_materialSourceData.m_parentMaterial.clear();
        return true;
    }

    void MaterialDocument::RestorePropertyValues(const PropertyValueMap& propertyValues)
    {
        for (const auto& propertyValuePair : propertyValues)
        {
            const auto& propertyName = propertyValuePair.first;
            const auto& propertyValue = propertyValuePair.second;
            SetPropertyValue(propertyName.GetStringView(), propertyValue);
        }
    }

    bool MaterialDocument::AddEditorMaterialFunctors(
        const AZStd::vector<AZ::RPI::Ptr<AZ::RPI::MaterialFunctorSourceDataHolder>>& functorSourceDataHolders,
        const AZ::RPI::MaterialNameContext& nameContext)
    {
        const AZ::RPI::MaterialFunctorSourceData::EditorContext editorContext = AZ::RPI::MaterialFunctorSourceData::EditorContext(
            m_materialSourceData.m_materialType, m_materialAsset->GetMaterialPropertiesLayout(), &nameContext);

        for (AZ::RPI::Ptr<AZ::RPI::MaterialFunctorSourceDataHolder> functorData : functorSourceDataHolders)
        {
            AZ::RPI::MaterialFunctorSourceData::FunctorResult result = functorData->CreateFunctor(editorContext);

            if (result.IsSuccess())
            {
                AZ::RPI::Ptr<AZ::RPI::MaterialFunctor>& functor = result.GetValue();
                if (functor != nullptr)
                {
                    m_editorFunctors.push_back(functor);
                }
            }
            else
            {
                AZ_Error("MaterialDocument", false, "Material functors were not created: '%s'.", m_absolutePath.c_str());
                return false;
            }
        }

        return true;
    }

    void MaterialDocument::RunEditorMaterialFunctors(AZ::RPI::MaterialPropertyFlags dirtyFlags)
    {
        if (!m_materialInstance)
        {
            return;
        }

        AZStd::unordered_map<AZ::Name, AZ::RPI::MaterialPropertyDynamicMetadata> propertyDynamicMetadata;
        AZStd::unordered_map<AZ::Name, AZ::RPI::MaterialPropertyGroupDynamicMetadata> propertyGroupDynamicMetadata;

        TraverseGroups(m_groups, [&](auto& group) {
            AZ::RPI::MaterialPropertyGroupDynamicMetadata& metadata = propertyGroupDynamicMetadata[AZ::Name{ group->m_name }];
            metadata.m_visibility = group->m_visible ? AZ::RPI::MaterialPropertyGroupVisibility::Enabled : AZ::RPI::MaterialPropertyGroupVisibility::Hidden;

            for (auto& property : group->m_properties)
            {
                AtomToolsFramework::ConvertToPropertyMetaData(propertyDynamicMetadata[property.GetId()], property.GetConfig());
            }
            return true;
        });

        AZStd::unordered_set<AZ::Name> updatedProperties;
        AZStd::unordered_set<AZ::Name> updatedPropertyGroups;

        for (AZ::RPI::Ptr<AZ::RPI::MaterialFunctor>& functor : m_editorFunctors)
        {
            const AZ::RPI::MaterialPropertyFlags& materialPropertyDependencies = functor->GetMaterialPropertyDependencies();

            // None also covers case that the client code doesn't register material properties to dependencies,
            // which will later get caught in Process() when trying to access a property.
            if (materialPropertyDependencies.none() || functor->NeedsProcess(dirtyFlags))
            {
                AZ::RPI::MaterialFunctorAPI::EditorContext context = AZ::RPI::MaterialFunctorAPI::EditorContext(
                    m_materialInstance->GetPropertyCollection(), propertyDynamicMetadata,
                    propertyGroupDynamicMetadata, updatedProperties, updatedPropertyGroups,
                    &materialPropertyDependencies);
                functor->Process(context);
            }
        }

        TraverseGroups(m_groups, [&](auto& group) {
            bool groupChange = false;
            bool groupRebuilt = false;
            if (updatedPropertyGroups.find(AZ::Name(group->m_name)) != updatedPropertyGroups.end())
            {
                AZ::RPI::MaterialPropertyGroupDynamicMetadata& metadata = propertyGroupDynamicMetadata[AZ::Name{ group->m_name }];
                group->m_visible = metadata.m_visibility != AZ::RPI::MaterialPropertyGroupVisibility::Hidden;
                groupChange = true;
            }

            for (auto& property : group->m_properties)
            {
                if (updatedProperties.find(AZ::Name(property.GetId())) != updatedProperties.end())
                {
                    const bool visibleBefore = property.GetConfig().m_visible;
                    AtomToolsFramework::DynamicPropertyConfig propertyConfig = property.GetConfig();
                    AtomToolsFramework::ConvertToPropertyConfig(propertyConfig, propertyDynamicMetadata[property.GetId()]);
                    property.SetConfig(propertyConfig);
                    groupChange = true;
                    groupRebuilt |= visibleBefore != property.GetConfig().m_visible;
                }
            }

            if (groupChange || groupRebuilt)
            {
                AtomToolsFramework::AtomToolsDocumentNotificationBus::Event(
                    m_toolId, &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentObjectInfoChanged, m_id,
                    GetObjectInfoFromDynamicPropertyGroup(group.get()), groupRebuilt);
            }
            return true;
        });
    }

    AtomToolsFramework::DocumentObjectInfo MaterialDocument::GetObjectInfoFromDynamicPropertyGroup(
        const AtomToolsFramework::DynamicPropertyGroup* group) const
    {
        AtomToolsFramework::DocumentObjectInfo objectInfo;
        objectInfo.m_visible = group->m_visible;
        objectInfo.m_name = group->m_name;
        objectInfo.m_displayName = group->m_displayName;
        objectInfo.m_description = group->m_description;
        objectInfo.m_objectType = azrtti_typeid<AtomToolsFramework::DynamicPropertyGroup>();
        objectInfo.m_objectPtr = const_cast<AtomToolsFramework::DynamicPropertyGroup*>(group);

        if (group->m_name == "overview")
        {
            // Properties in the overview category don't require special comparison or indicator icons. However, the blank icon is still
            // needed to keep everything aligned.
            objectInfo.m_nodeIndicatorFunction = []([[maybe_unused]] const AzToolsFramework::InstanceDataNode* node)
            {
                return ":/Icons/blank.png";
            };
        }
        else
        {
            objectInfo.m_nodeIndicatorFunction = [](const AzToolsFramework::InstanceDataNode* node)
            {
                const auto property = AtomToolsFramework::FindAncestorInstanceDataNodeByType<AtomToolsFramework::DynamicProperty>(node);
                return property && !AtomToolsFramework::ArePropertyValuesEqual(property->GetValue(), property->GetConfig().m_parentValue)
                    ? ":/Icons/changed_property.svg"
                    : ":/Icons/blank.png";
            };
        }

        return objectInfo;
    }

    bool MaterialDocument::TraverseGroups(
        AZStd::vector<AZStd::shared_ptr<AtomToolsFramework::DynamicPropertyGroup>>& groups,
        AZStd::function<bool(AZStd::shared_ptr<AtomToolsFramework::DynamicPropertyGroup>&)> callback)
    {
        if (!callback)
        {
            return false;
        }

        for (auto& group : groups)
        {
            if (!callback(group) || !TraverseGroups(group->m_groups, callback))
            {
                return false;
            }
        }

        return true;
    }

    bool MaterialDocument::TraverseGroups(
        const AZStd::vector<AZStd::shared_ptr<AtomToolsFramework::DynamicPropertyGroup>>& groups,
        AZStd::function<bool(const AZStd::shared_ptr<AtomToolsFramework::DynamicPropertyGroup>&)> callback) const
    {
        if (!callback)
        {
            return false;
        }

        for (auto& group : groups)
        {
            if (!callback(group) || !TraverseGroups(group->m_groups, callback))
            {
                return false;
            }
        }

        return true;
    }

    AtomToolsFramework::DynamicProperty* MaterialDocument::FindProperty(const AZ::Name& propertyId)
    {
        AtomToolsFramework::DynamicProperty* result = nullptr; 
        TraverseGroups(m_groups, [&](auto& group) {
            for (auto& property : group->m_properties)
            {
                if (property.GetId() == propertyId)
                {
                    result = &property;
                    return false;
                }
            }
            return true;
        });
        return result;
    }

    const AtomToolsFramework::DynamicProperty* MaterialDocument::FindProperty(const AZ::Name& propertyId) const
    {
        AtomToolsFramework::DynamicProperty* result = nullptr; 
        TraverseGroups(m_groups, [&](auto& group) {
            for (auto& property : group->m_properties)
            {
                if (property.GetId() == propertyId)
                {
                    result = &property;
                    return false;
                }
            }
            return true;
        });
        return result;
    }
} // namespace MaterialEditor
