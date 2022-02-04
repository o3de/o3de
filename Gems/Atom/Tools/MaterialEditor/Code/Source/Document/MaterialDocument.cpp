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
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <AtomCore/Instance/Instance.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/Util/MaterialPropertyUtil.h>
#include <Document/MaterialDocument.h>

namespace MaterialEditor
{
    MaterialDocument::MaterialDocument()
        : AtomToolsFramework::AtomToolsDocument()
    {
        MaterialDocumentRequestBus::Handler::BusConnect(m_id);
    }

    MaterialDocument::~MaterialDocument()
    {
        MaterialDocumentRequestBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
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

    void MaterialDocument::SetPropertyValue(const AZ::Name& propertyId, const AZStd::any& value)
    {
        if (!IsOpen())
        {
            AZ_Error("MaterialDocument", false, "Document is not open.");
            return;
        }

        AtomToolsFramework::DynamicProperty* foundProperty = {};
        TraverseGroups(m_groups, [&, this](auto& group) {
            for (auto& property : group->m_properties)
            {
                if (property.GetId() == propertyId)
                {
                    foundProperty = &property;

                    // This first converts to an acceptable runtime type in case the value came from script
                    const AZ::RPI::MaterialPropertyValue propertyValue = AtomToolsFramework::ConvertToRuntimeType(value);

                    property.SetValue(AtomToolsFramework::ConvertToEditableType(propertyValue));

                    const auto propertyIndex = m_materialInstance->FindPropertyIndex(propertyId);
                    if (!propertyIndex.IsNull())
                    {
                        if (m_materialInstance->SetPropertyValue(propertyIndex, propertyValue))
                        {
                            AZ::RPI::MaterialPropertyFlags dirtyFlags = m_materialInstance->GetPropertyDirtyFlags();

                            Recompile();
                            RunEditorMaterialFunctors(dirtyFlags);
                        }
                    }

                    AtomToolsFramework::AtomToolsDocumentNotificationBus::Broadcast(
                        &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentObjectInfoChanged, m_id,
                        GetObjectInfoFromDynamicPropertyGroup(group.get()), false);

                    AtomToolsFramework::AtomToolsDocumentNotificationBus::Broadcast(
                        &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentModified, m_id);
                    return false;
                }
            }
            return true;
        });

        if (!foundProperty)
        {
            AZ_Error("MaterialDocument", false, "Document property could not be found: '%s'.", propertyId.GetCStr());
        }
    }

    const AZStd::any& MaterialDocument::GetPropertyValue(const AZ::Name& propertyId) const
    {
        if (!IsOpen())
        {
            AZ_Error("MaterialDocument", false, "Document is not open.");
            return m_invalidValue;
        }

        auto property = FindProperty(propertyId);
        if (!property)
        {
            AZ_Error("MaterialDocument", false, "Document property could not be found: '%s'.", propertyId.GetCStr());
            return m_invalidValue;
        }

        return property->GetValue();
    }

    AZStd::vector<AtomToolsFramework::DocumentObjectInfo> MaterialDocument::GetObjectInfo() const
    {
        if (!IsOpen())
        {
            AZ_Error("MaterialDocument", false, "Document is not open.");
            return {};
        }

        AZStd::vector<AtomToolsFramework::DocumentObjectInfo> objects;
        objects.reserve(m_groups.size());

        AtomToolsFramework::DocumentObjectInfo objectInfo;
        for (const auto& group : m_groups)
        {
            objects.push_back(GetObjectInfoFromDynamicPropertyGroup(group.get()));
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
        sourceData.m_materialTypeVersion = m_materialAsset->GetMaterialTypeAsset()->GetVersion();
        sourceData.m_materialType = AtomToolsFramework::GetExteralReferencePath(m_absolutePath, m_materialSourceData.m_materialType);
        sourceData.m_parentMaterial = AtomToolsFramework::GetExteralReferencePath(m_absolutePath, m_materialSourceData.m_parentMaterial);
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

    bool MaterialDocument::SaveAsCopy(AZStd::string_view savePath)
    {
        if (!AtomToolsDocument::SaveAsCopy(savePath))
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

        // populate sourceData with modified or overridden properties and save object
        AZ::RPI::MaterialSourceData sourceData;
        sourceData.m_materialTypeVersion = m_materialAsset->GetMaterialTypeAsset()->GetVersion();
        sourceData.m_materialType = AtomToolsFramework::GetExteralReferencePath(m_savePathNormalized, m_materialSourceData.m_materialType);
        sourceData.m_parentMaterial = AtomToolsFramework::GetExteralReferencePath(m_savePathNormalized, m_materialSourceData.m_parentMaterial);
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

    bool MaterialDocument::SaveAsChild(AZStd::string_view savePath)
    {
        if (!AtomToolsDocument::SaveAsChild(savePath))
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

        // populate sourceData with modified or overridden properties and save object
        AZ::RPI::MaterialSourceData sourceData;
        sourceData.m_materialTypeVersion = m_materialAsset->GetMaterialTypeAsset()->GetVersion();
        sourceData.m_materialType = AtomToolsFramework::GetExteralReferencePath(m_savePathNormalized, m_materialSourceData.m_materialType);

        // Only assign a parent path if the source was a .material
        if (AzFramework::StringFunc::Path::IsExtension(m_absolutePath.c_str(), AZ::RPI::MaterialSourceData::Extension))
        {
            sourceData.m_parentMaterial = AtomToolsFramework::GetExteralReferencePath(m_savePathNormalized, m_absolutePath);
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

    bool MaterialDocument::IsOpen() const
    {
        return AtomToolsDocument::IsOpen() && m_materialAsset.IsReady() && m_materialInstance;
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

    bool MaterialDocument::IsSavable() const
    {
        return AzFramework::StringFunc::Path::IsExtension(m_absolutePath.c_str(), AZ::RPI::MaterialSourceData::Extension);
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
            const auto& propertyValueForRedo = GetPropertyValue(propertyName);
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

    void MaterialDocument::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
    {
        if (m_compilePending)
        {
            if (m_materialInstance->Compile())
            {
                m_compilePending = false;
                AZ::TickBus::Handler::BusDisconnect();
            }
        }
    }

    bool MaterialDocument::SaveSourceData(AZ::RPI::MaterialSourceData& sourceData, PropertyFilterFunction propertyFilter) const
    {
        bool addPropertiesResult = true;

        // populate sourceData with properties that meet the filter
        m_materialTypeSourceData.EnumerateProperties([&, this](const AZStd::string& propertyIdContext, const auto& propertyDefinition) {

            AZ::Name propertyId{propertyIdContext + propertyDefinition->GetName()};

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
                    
                    // TODO: Support populating the Material Editor with nested property groups, not just the top level.
                    const AZStd::string groupName = propertyId.GetStringView().substr(0, propertyId.GetStringView().size() - propertyDefinition->GetName().size() - 1);
                    sourceData.m_properties[groupName][propertyDefinition->GetName()].m_value = propertyValue;
                }
            }
            return true;
        });

        if (!addPropertiesResult)
        {
            AZ_Error("MaterialDocument", false, "Document properties could not be saved: '%s'.", m_savePathNormalized.c_str());
            return false;
        }

        if (!AZ::RPI::JsonUtils::SaveObjectToFile(m_savePathNormalized, sourceData))
        {
            AZ_Error("MaterialDocument", false, "Document could not be saved: '%s'.", m_savePathNormalized.c_str());
            return false;
        }

        return true;
    }

    bool MaterialDocument::Open(AZStd::string_view loadPath)
    {
        if (!AtomToolsDocument::Open(loadPath))
        {
            return false;
        }

        // The material document and inspector are constructed from source data
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

        AZStd::span<const AZ::RPI::MaterialPropertyValue> parentPropertyValues = materialTypeAsset->GetDefaultPropertyValues();
        AZ::Data::Asset<AZ::RPI::MaterialAsset> parentMaterialAsset;
        if (!m_materialSourceData.m_parentMaterial.empty())
        {
            AZ::RPI::MaterialSourceData parentMaterialSourceData;
            if (!AZ::RPI::JsonUtils::LoadObjectFromFile(m_materialSourceData.m_parentMaterial, parentMaterialSourceData))
            {
                AZ_Error("MaterialDocument", false, "Material parent source data could not be loaded for: '%s'.", m_materialSourceData.m_parentMaterial.c_str());
                return OpenFailed();
            }

            const auto parentMaterialAssetIdResult = AZ::RPI::AssetUtils::MakeAssetId(m_materialSourceData.m_parentMaterial, 0);
            if (!parentMaterialAssetIdResult)
            {
                AZ_Error("MaterialDocument", false, "Material parent asset ID could not be created: '%s'.", m_materialSourceData.m_parentMaterial.c_str());
                return OpenFailed();
            }
            
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

        // Creating a material from a material asset will fail if a texture is referenced but not loaded 
        m_materialInstance = AZ::RPI::Material::Create(m_materialAsset);
        if (!m_materialInstance)
        {
            AZ_Error("MaterialDocument", false, "Material instance could not be created: '%s'.", m_absolutePath.c_str());
            return OpenFailed();
        }

        // Pipeline State Object changes are always allowed in the material editor because it only runs on developer systems
        // where such changes are supported at runtime.
        m_materialInstance->SetPsoHandlingOverride(AZ::RPI::MaterialPropertyPsoHandling::Allowed);

        // Adding properties for material type and parent as part of making dynamic properties and the inspector more general purpose. This
        // allows the read only properties to appear in the inspector like any other property. This may change or be removed once support
        // for changing the material parent is implemented.
        m_groups.emplace_back(aznew AtomToolsFramework::DynamicPropertyGroup);
        m_groups.back()->m_name = "overview";
        m_groups.back()->m_displayName = "Overview";
        m_groups.back()->m_description = m_materialSourceData.m_description;

        AtomToolsFramework::DynamicPropertyConfig propertyConfig;
        propertyConfig.m_dataType = AtomToolsFramework::DynamicPropertyType::Asset;
        propertyConfig.m_id = "overview.materialType";
        propertyConfig.m_name = "materialType";
        propertyConfig.m_displayName = "Material Type";
        propertyConfig.m_groupName = "overview";
        propertyConfig.m_groupDisplayName = "Overview";
        propertyConfig.m_description = "The material type defines the layout, properties, default values, shader connections, and other "
                                       "data needed to create and edit a derived material.";
        propertyConfig.m_defaultValue = AZStd::any(materialTypeAsset);
        propertyConfig.m_originalValue = propertyConfig.m_defaultValue;
        propertyConfig.m_parentValue = propertyConfig.m_defaultValue;
        propertyConfig.m_readOnly = true;

        m_groups.back()->m_properties.push_back(AtomToolsFramework::DynamicProperty(propertyConfig));

        propertyConfig = {};
        propertyConfig.m_dataType = AtomToolsFramework::DynamicPropertyType::Asset;
        propertyConfig.m_id = "overview.parentMaterial";
        propertyConfig.m_name = "parentMaterial";
        propertyConfig.m_displayName = "Parent Material";
        propertyConfig.m_groupName = "overview";
        propertyConfig.m_groupDisplayName = "Overview";
        propertyConfig.m_description =
            "The parent material provides an initial configuration whose properties are inherited and overriden by a derived material.";
        propertyConfig.m_defaultValue = AZStd::any(parentMaterialAsset);
        propertyConfig.m_originalValue = propertyConfig.m_defaultValue;
        propertyConfig.m_parentValue = propertyConfig.m_defaultValue;
        propertyConfig.m_readOnly = true;
        propertyConfig.m_showThumbnail = true;

        m_groups.back()->m_properties.push_back(AtomToolsFramework::DynamicProperty(propertyConfig));

        m_groups.emplace_back(aznew AtomToolsFramework::DynamicPropertyGroup);
        m_groups.back()->m_name = UvGroupName;
        m_groups.back()->m_displayName = "UV Sets";
        m_groups.back()->m_description = "UV set names in this material, which can be renamed to match those in the model.";

        // Add UV name customization properties
        const AZ::RPI::MaterialUvNameMap& uvNameMap = materialTypeAsset->GetUvNameMap();
        for (const AZ::RPI::UvNamePair& uvNamePair : uvNameMap)
        {
            const AZStd::string shaderInput = uvNamePair.m_shaderInput.ToString();
            const AZStd::string uvName = uvNamePair.m_uvName.GetStringView();

            propertyConfig = {};
            propertyConfig.m_dataType = AtomToolsFramework::DynamicPropertyType::String;
            propertyConfig.m_id = AZ::RPI::MaterialPropertyId(UvGroupName, shaderInput);
            propertyConfig.m_name = shaderInput;
            propertyConfig.m_displayName = shaderInput;
            propertyConfig.m_groupName = UvGroupName;
            propertyConfig.m_groupDisplayName = "UV Sets";
            propertyConfig.m_description = shaderInput;
            propertyConfig.m_defaultValue = uvName;
            propertyConfig.m_originalValue = uvName;
            propertyConfig.m_parentValue = uvName;
            propertyConfig.m_readOnly = true;

            m_groups.back()->m_properties.push_back(AtomToolsFramework::DynamicProperty(propertyConfig));
        }

        // Populate the property map from a combination of source data and assets
        // Assets must still be used for now because they contain the final accumulated value after all other materials
        // in the hierarchy are applied
        bool enumerateResult = m_materialTypeSourceData.EnumeratePropertyGroups(
            [this, &parentPropertyValues](
                const AZStd::string& propertyIdContext, const AZ::RPI::MaterialTypeSourceData::PropertyGroup* propertyGroup)
            {
                // Add any material functors that are located inside each property group.
                if (!AddEditorMaterialFunctors(propertyGroup->GetFunctors()))
                {
                    return false;
                }

                m_groups.emplace_back(aznew AtomToolsFramework::DynamicPropertyGroup);
                m_groups.back()->m_name = propertyIdContext + propertyGroup->GetName();
                m_groups.back()->m_displayName = propertyGroup->GetDisplayName();
                m_groups.back()->m_description = propertyGroup->GetDescription();

                for (const auto& propertyDefinition : propertyGroup->GetProperties())
                {
                    // Assign id before conversion so it can be used in dynamic description
                    AtomToolsFramework::DynamicPropertyConfig propertyConfig;
                    propertyConfig.m_id = m_groups.back()->m_name + "." + propertyDefinition->GetName();

                    const auto& propertyIndex = m_materialAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(propertyConfig.m_id);
                    const bool propertyIndexInBounds =
                        propertyIndex.IsValid() && propertyIndex.GetIndex() < m_materialAsset->GetPropertyValues().size();
                    AZ_Warning(
                        "MaterialDocument", propertyIndexInBounds, "Failed to add material property '%s' to document '%s'.",
                        propertyConfig.m_id.GetCStr(), m_absolutePath.c_str());

                    if (propertyIndexInBounds)
                    {
                        AtomToolsFramework::ConvertToPropertyConfig(propertyConfig, *propertyDefinition);

                        // TODO: Support populating the Material Editor with nested property groups, not just the top level.
                        // (Does DynamicPropertyConfig really even need m_groupDisplayName?)
                        propertyConfig.m_groupName = m_groups.back()->m_name;
                        propertyConfig.m_groupDisplayName = m_groups.back()->m_displayName;
                        propertyConfig.m_showThumbnail = true;
                        propertyConfig.m_originalValue =
                            AtomToolsFramework::ConvertToEditableType(m_materialAsset->GetPropertyValues()[propertyIndex.GetIndex()]);
                        propertyConfig.m_parentValue =
                            AtomToolsFramework::ConvertToEditableType(parentPropertyValues[propertyIndex.GetIndex()]);
                        propertyConfig.m_dataChangeCallback = [documentId = m_id, propertyId = propertyConfig.m_id](const AZStd::any& value)
                        {
                            MaterialDocumentRequestBus::Event(
                                documentId, &MaterialDocumentRequestBus::Events::SetPropertyValue, propertyId, value);
                            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
                        };

                        m_groups.back()->m_properties.push_back(AtomToolsFramework::DynamicProperty(propertyConfig));
                    }
                }

                return true;
            });

        if (!enumerateResult)
        {
            return OpenFailed();
        }

        // Add material functors that are in the top-level functors list.
        if (!AddEditorMaterialFunctors(m_materialTypeSourceData.m_materialFunctorSourceData))
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

        AZ::TickBus::Handler::BusDisconnect();

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
            AZ::TickBus::Handler::BusConnect();
            m_compilePending = true;
        }
    }

    bool MaterialDocument::LoadMaterialSourceData()
    {
        // Load the material source data so that we can check properties and create a material asset from it
        if (!AZ::RPI::JsonUtils::LoadObjectFromFile(m_absolutePath, m_materialSourceData))
        {
            AZ_Error("MaterialDocument", false, "Material source data could not be loaded: '%s'.", m_absolutePath.c_str());
            return false;
        }

        // We always need the absolute path for the material type and parent material to load source data and resolving
        // relative paths when saving. This will convert and store them as absolute paths for use within the document.
        if (!m_materialSourceData.m_parentMaterial.empty())
        {
            m_materialSourceData.m_parentMaterial =
                AZ::RPI::AssetUtils::ResolvePathReference(m_absolutePath, m_materialSourceData.m_parentMaterial);
        }

        if (!m_materialSourceData.m_materialType.empty())
        {
            m_materialSourceData.m_materialType =
                AZ::RPI::AssetUtils::ResolvePathReference(m_absolutePath, m_materialSourceData.m_materialType);
        }

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
            SetPropertyValue(propertyName, propertyValue);
        }
    }

    bool MaterialDocument::AddEditorMaterialFunctors(
        const AZStd::vector<AZ::RPI::Ptr<AZ::RPI::MaterialFunctorSourceDataHolder>>& functorSourceDataHolders)
    {
        const AZ::RPI::MaterialFunctorSourceData::EditorContext editorContext = AZ::RPI::MaterialFunctorSourceData::EditorContext(
            m_materialSourceData.m_materialType, m_materialAsset->GetMaterialPropertiesLayout());

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
                AZ::RPI::MaterialFunctor::EditorContext context = AZ::RPI::MaterialFunctor::EditorContext(
                    m_materialInstance->GetPropertyValues(), m_materialInstance->GetMaterialPropertiesLayout(), propertyDynamicMetadata,
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
                AtomToolsFramework::AtomToolsDocumentNotificationBus::Broadcast(
                    &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentObjectInfoChanged, m_id,
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
