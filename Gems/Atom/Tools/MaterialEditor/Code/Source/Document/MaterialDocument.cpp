/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyId.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceData.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Reflect/Image/Image.h>
#include <AtomCore/Instance/Instance.h>
#include <Document/MaterialDocument.h>
#include <Atom/Document/MaterialDocumentNotificationBus.h>
#include <AtomToolsFramework/Util/MaterialPropertyUtil.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

namespace MaterialEditor
{
    MaterialDocument::MaterialDocument()
    {
        MaterialDocumentRequestBus::Handler::BusConnect(m_id);
        MaterialDocumentNotificationBus::Broadcast(&MaterialDocumentNotificationBus::Events::OnDocumentCreated, m_id);
    }

    MaterialDocument::~MaterialDocument()
    {
        MaterialDocumentNotificationBus::Broadcast(&MaterialDocumentNotificationBus::Events::OnDocumentDestroyed, m_id);
        MaterialDocumentRequestBus::Handler::BusDisconnect();
        Clear();
    }

    const AZ::Uuid& MaterialDocument::GetId() const
    {
        return m_id;
    }

    AZStd::string_view MaterialDocument::GetAbsolutePath() const
    {
        return m_absolutePath;
    }

    AZStd::string_view MaterialDocument::GetRelativePath() const
    {
        return m_relativePath;
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

    const AZStd::any& MaterialDocument::GetPropertyValue(const AZ::Name& propertyFullName) const
    {
        using namespace AZ;
        using namespace RPI;

        if (!IsOpen())
        {
            AZ_Error("MaterialDocument", false, "Material document is not open.");
            return m_invalidValue;
        }

        const auto it = m_properties.find(propertyFullName);
        if (it == m_properties.end())
        {
            AZ_Error("MaterialDocument", false, "Material document property could not be found: '%s'.", propertyFullName.GetCStr());
            return m_invalidValue;
        }

        const AtomToolsFramework::DynamicProperty& property = it->second;
        return property.GetValue();
    }

    const AtomToolsFramework::DynamicProperty& MaterialDocument::GetProperty(const AZ::Name& propertyFullName) const
    {
        if (!IsOpen())
        {
            AZ_Error("MaterialDocument", false, "Material document is not open.");
            return m_invalidProperty;
        }

        const auto it = m_properties.find(propertyFullName);
        if (it == m_properties.end())
        {
            AZ_Error("MaterialDocument", false, "Material document property could not be found: '%s'.", propertyFullName.GetCStr());
            return m_invalidProperty;
        }

        const AtomToolsFramework::DynamicProperty& property = it->second;
        return property;
    }
    
    bool MaterialDocument::IsPropertyGroupVisible(const AZ::Name& propertyGroupFullName) const
    {
        if (!IsOpen())
        {
            AZ_Error("MaterialDocument", false, "Material document is not open.");
            return false;
        }

        const auto it = m_propertyGroupVisibility.find(propertyGroupFullName);
        if (it == m_propertyGroupVisibility.end())
        {
            AZ_Error("MaterialDocument", false, "Material document property group could not be found: '%s'.", propertyGroupFullName.GetCStr());
            return false;
        }

        return it->second;
    }

    void MaterialDocument::SetPropertyValue(const AZ::Name& propertyFullName, const AZStd::any& value)
    {
        using namespace AZ;
        using namespace RPI;

        if (!IsOpen())
        {
            AZ_Error("MaterialDocument", false, "Material document is not open.");
            return;
        }

        const auto it = m_properties.find(propertyFullName);
        if (it == m_properties.end())
        {
            AZ_Error("MaterialDocument", false, "Material document property could not be found: '%s'.", propertyFullName.GetCStr());
            return;
        }

        // This first converts to an acceptable runtime type in case the value came from script
        const AZ::RPI::MaterialPropertyValue propertyValue =
            AtomToolsFramework::ConvertToRuntimeType(value);

        AtomToolsFramework::DynamicProperty& property = it->second;
        property.SetValue(AtomToolsFramework::ConvertToEditableType(propertyValue));

        const AZ::RPI::MaterialPropertyId propertyId = AZ::RPI::MaterialPropertyId::Parse(propertyFullName.GetStringView());
        const auto propertyIndex = m_materialInstance->FindPropertyIndex(propertyFullName);
        if (!propertyIndex.IsNull())
        {
            if (m_materialInstance->SetPropertyValue(propertyIndex, propertyValue))
            {
                MaterialPropertyFlags dirtyFlags = m_materialInstance->GetPropertyDirtyFlags();

                Recompile();

                EditorMaterialFunctorResult result = RunEditorMaterialFunctors(dirtyFlags);
                for (const Name& changedPropertyGroupName : result.m_updatedPropertyGroups)
                {
                    MaterialDocumentNotificationBus::Broadcast(&MaterialDocumentNotificationBus::Events::OnDocumentPropertyGroupVisibilityChanged, m_id, changedPropertyGroupName, IsPropertyGroupVisible(changedPropertyGroupName));
                }
                for (const Name& changedPropertyName : result.m_updatedProperties)
                {
                    MaterialDocumentNotificationBus::Broadcast(&MaterialDocumentNotificationBus::Events::OnDocumentPropertyConfigModified, m_id, GetProperty(changedPropertyName));
                }
            }
        }

        MaterialDocumentNotificationBus::Broadcast(&MaterialDocumentNotificationBus::Events::OnDocumentPropertyValueModified, m_id, property);
        MaterialDocumentNotificationBus::Broadcast(&MaterialDocumentNotificationBus::Events::OnDocumentModified, m_id);
    }

    bool MaterialDocument::Open(AZStd::string_view loadPath)
    {
        if (!OpenInternal(loadPath))
        {
            Clear();
            AZ_Error("MaterialDocument", false, "Material document could not be opened: '%s'.", loadPath.data());
            return false;
        }

        MaterialDocumentNotificationBus::Broadcast(&MaterialDocumentNotificationBus::Events::OnDocumentOpened, m_id);
        return true;
    }

    bool MaterialDocument::Rebuild()
    {
        // Store history and property changes that should be reapplied after reload
        auto undoHistoryToRestore = m_undoHistory;
        auto undoHistoryIndexToRestore = m_undoHistoryIndex;
        PropertyValueMap propertyValuesToRestore;
        for (const auto& propertyPair : m_properties)
        {
            const AtomToolsFramework::DynamicProperty& property = propertyPair.second;
            if (!AtomToolsFramework::ArePropertyValuesEqual(property.GetValue(), property.GetConfig().m_parentValue))
            {
                propertyValuesToRestore[property.GetId()] = property.GetValue();
            }
        }

        // Reopen the same document
        const AZStd::string loadPath = m_absolutePath;
        if (!OpenInternal(loadPath))
        {
            Clear();
            return false;
        }

        RestorePropertyValues(propertyValuesToRestore);
        AZStd::swap(undoHistoryToRestore, m_undoHistory);
        AZStd::swap(undoHistoryIndexToRestore, m_undoHistoryIndex);
        MaterialDocumentNotificationBus::Broadcast(&MaterialDocumentNotificationBus::Events::OnDocumentOpened, m_id);
        return true;
    }

    bool MaterialDocument::Save()
    {
        using namespace AZ;
        using namespace RPI;

        if (!IsOpen())
        {
            AZ_Error("MaterialDocument", false, "Material document is not open to be saved: '%s'.", m_absolutePath.c_str());
            return false;
        }

        if (!IsSavable())
        {
            AZ_Error("MaterialDocument", false, "Material types can only be saved as a child: '%s'.", m_absolutePath.c_str());
            return false;
        }

        // create source data from properties
        MaterialSourceData sourceData;
        sourceData.m_propertyLayoutVersion = m_materialTypeSourceData.m_propertyLayout.m_version;
        sourceData.m_materialType = m_materialSourceData.m_materialType;
        sourceData.m_parentMaterial = m_materialSourceData.m_parentMaterial;

        // Force save data to store forward slashes
        AzFramework::StringFunc::Replace(sourceData.m_materialType, "\\", "/");
        AzFramework::StringFunc::Replace(sourceData.m_parentMaterial, "\\", "/");

        // populate sourceData with modified or overwritten properties
        const bool savedProperties = SavePropertiesToSourceData(sourceData, [](const AtomToolsFramework::DynamicProperty& property) {
            return !AtomToolsFramework::ArePropertyValuesEqual(property.GetValue(), property.GetConfig().m_parentValue);
        });

        if (!savedProperties)
        {
            return false;
        }

        // write sourceData to .material file
        if (!AZ::RPI::JsonUtils::SaveObjectToFile(m_absolutePath, sourceData))
        {
            AZ_Error("MaterialDocument", false, "Material document could not be saved: '%s'.", m_absolutePath.c_str());
            return false;
        }

        // after saving, reset to a clean state
        for (auto& propertyPair : m_properties)
        {
            AtomToolsFramework::DynamicProperty& property = propertyPair.second;
            auto propertyConfig = property.GetConfig();
            propertyConfig.m_originalValue = property.GetValue();
            property.SetConfig(propertyConfig);
        }

        // Auto add or checkout saved file
        AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::RequestEdit,
            m_absolutePath.c_str(), true, [](bool, const AzToolsFramework::SourceControlFileInfo&) {});

        AZ_TracePrintf("MaterialDocument", "Material document saved: '%s'.\n", m_absolutePath.data());

        MaterialDocumentNotificationBus::Broadcast(&MaterialDocumentNotificationBus::Events::OnDocumentSaved, m_id);

        m_saveTriggeredInternally = true;
        return true;
    }

    bool MaterialDocument::SaveAsCopy(AZStd::string_view savePath)
    {
        using namespace AZ;
        using namespace RPI;

        if (!IsOpen())
        {
            AZ_Error("MaterialDocument", false, "Material document is not open to be saved: '%s'.", m_absolutePath.c_str());
            return false;
        }

        if (!IsSavable())
        {
            AZ_Error("MaterialDocument", false, "Material types can only be saved as a child: '%s'.", m_absolutePath.c_str());
            return false;
        }

        AZStd::string normalizedSavePath = savePath;
        if (!AzFramework::StringFunc::Path::Normalize(normalizedSavePath))
        {
            AZ_Error("MaterialDocument", false, "Material document save path could not be normalized: '%s'.", normalizedSavePath.c_str());
            return false;
        }

        // create source data from properties
        MaterialSourceData sourceData;
        sourceData.m_propertyLayoutVersion = m_materialTypeSourceData.m_propertyLayout.m_version;
        sourceData.m_materialType = m_materialSourceData.m_materialType;
        sourceData.m_parentMaterial = m_materialSourceData.m_parentMaterial;

        // Force save data to store forward slashes
        AzFramework::StringFunc::Replace(sourceData.m_materialType, "\\", "/");
        AzFramework::StringFunc::Replace(sourceData.m_parentMaterial, "\\", "/");

        // populate sourceData with modified or overwritten properties
        const bool savedProperties = SavePropertiesToSourceData(sourceData, [](const AtomToolsFramework::DynamicProperty& property) {
            return !AtomToolsFramework::ArePropertyValuesEqual(property.GetValue(), property.GetConfig().m_parentValue);
        });

        if (!savedProperties)
        {
            return false;
        }

        // write sourceData to .material file
        if (!AZ::RPI::JsonUtils::SaveObjectToFile(normalizedSavePath, sourceData))
        {
            AZ_Error("MaterialDocument", false, "Material document could not be saved: '%s'.", normalizedSavePath.c_str());
            return false;
        }

        // Auto add or checkout saved file
        AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::RequestEdit,
            normalizedSavePath.c_str(), true, [](bool, const AzToolsFramework::SourceControlFileInfo&) {});

        AZ_TracePrintf("MaterialDocument", "Material document saved: '%s'.\n", normalizedSavePath.c_str());

        MaterialDocumentNotificationBus::Broadcast(&MaterialDocumentNotificationBus::Events::OnDocumentSaved, m_id);

        // If the document is saved to a new file we need to reopen the new document to update assets, paths, property deltas.
        if (!Open(normalizedSavePath))
        {
            return false;
        }

        // Setting flag after reopening becausse it's cleared on open
        m_saveTriggeredInternally = true;
        return true;
    }

    bool MaterialDocument::SaveAsChild(AZStd::string_view savePath)
    {
        using namespace AZ;
        using namespace RPI;

        if (!IsOpen())
        {
            AZ_Error("MaterialDocument", false, "Material document is not open to be saved: '%s'.", m_absolutePath.c_str());
            return false;
        }

        AZStd::string normalizedSavePath = savePath;
        if (!AzFramework::StringFunc::Path::Normalize(normalizedSavePath))
        {
            AZ_Error("MaterialDocument", false, "Material document save path could not be normalized: '%s'.", normalizedSavePath.c_str());
            return false;
        }

        if (m_absolutePath == normalizedSavePath)
        {
            // ToDo: this should scan the entire hierarchy so we don't overwrite parent's parent, for example
            AZ_Error("MaterialDocument", false, "Can't overwrite parent material with a child that depends on it.");
            return false;
        }

        // create source data from properties
        MaterialSourceData sourceData;
        sourceData.m_propertyLayoutVersion = m_materialTypeSourceData.m_propertyLayout.m_version;
        sourceData.m_materialType = m_materialSourceData.m_materialType;

        // Only assign a parent path if the source was a .material
        if (AzFramework::StringFunc::Path::IsExtension(m_relativePath.c_str(), MaterialSourceData::Extension))
        {
            sourceData.m_parentMaterial = m_relativePath;
        }

        // Force save data to store forward slashes
        AzFramework::StringFunc::Replace(sourceData.m_materialType, "\\", "/");
        AzFramework::StringFunc::Replace(sourceData.m_parentMaterial, "\\", "/");

        // populate sourceData with modified properties
        const bool savedProperties = SavePropertiesToSourceData(sourceData, [](const AtomToolsFramework::DynamicProperty& property) {
            return !AtomToolsFramework::ArePropertyValuesEqual(property.GetValue(), property.GetConfig().m_originalValue);
        });

        if (!savedProperties)
        {
            return false;
        }

        // write sourceData to .material file
        if (!AZ::RPI::JsonUtils::SaveObjectToFile(normalizedSavePath, sourceData))
        {
            AZ_Error("MaterialDocument", false, "Material document could not be saved: '%s'.", normalizedSavePath.c_str());
            return false;
        }

        // Auto add or checkout saved file
        AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::RequestEdit,
            normalizedSavePath.c_str(), true, [](bool, const AzToolsFramework::SourceControlFileInfo&) {});

        AZ_TracePrintf("MaterialDocument", "Material document saved: '%s'.\n", normalizedSavePath.c_str());

        MaterialDocumentNotificationBus::Broadcast(&MaterialDocumentNotificationBus::Events::OnDocumentSaved, m_id);

        // If the document is saved to a new file we need to reopen the new document to update assets, paths, property deltas.
        if (!Open(normalizedSavePath))
        {
            return false;
        }

        // Setting flag after reopening becausse it's cleared on open
        m_saveTriggeredInternally = true;
        return true;
    }

    bool MaterialDocument::Close()
    {
        using namespace AZ;
        using namespace RPI;

        if (!IsOpen())
        {
            AZ_Error("MaterialDocument", false, "Material document is not open.");
            return false;
        }

        AZ_TracePrintf("MaterialDocument", "Material document closed: '%s'.\n", m_absolutePath.c_str());

        MaterialDocumentNotificationBus::Broadcast(&MaterialDocumentNotificationBus::Events::OnDocumentClosed, m_id);

        // Clearing after notification so paths are still available
        Clear();
        return true;
    }

    bool MaterialDocument::IsOpen() const
    {
        return !m_absolutePath.empty() && !m_relativePath.empty() && m_materialAsset.IsReady() && m_materialInstance;
    }

    bool MaterialDocument::IsModified() const
    {
        return AZStd::any_of(m_properties.begin(), m_properties.end(),
            [](const auto& propertyPair)
        {
            const AtomToolsFramework::DynamicProperty& property = propertyPair.second;
            return !AtomToolsFramework::ArePropertyValuesEqual(property.GetValue(), property.GetConfig().m_originalValue);
        });
    }

    bool MaterialDocument::IsSavable() const
    {
        return AzFramework::StringFunc::Path::IsExtension(m_absolutePath.c_str(), AZ::RPI::MaterialSourceData::Extension);
    }

    bool MaterialDocument::CanUndo() const
    {
        // Undo will only be allowed if something has been recorded and we're not at the beginning of history
        return IsOpen() && !m_undoHistory.empty() && m_undoHistoryIndex > 0;
    }

    bool MaterialDocument::CanRedo() const
    {
        // Redo will only be allowed if something has been recorded and we're not at the end of history
        return IsOpen() && !m_undoHistory.empty() && m_undoHistoryIndex < m_undoHistory.size();
    }

    bool MaterialDocument::Undo()
    {
        if (CanUndo())
        {
            // The history index is one beyond the last executed command. Decrement the index then execute undo.
            m_undoHistory[--m_undoHistoryIndex].first();
            AZ_TracePrintf("MaterialDocument", "Material document undo: '%s'.\n", m_absolutePath.c_str());
            MaterialDocumentNotificationBus::Broadcast(&MaterialDocumentNotificationBus::Events::OnDocumentUndoStateChanged, m_id);
            return true;
        }
        return false;
    }

    bool MaterialDocument::Redo()
    {
        if (CanRedo())
        {
            // Execute the current redo command then move the history index to the next position.
            m_undoHistory[m_undoHistoryIndex++].second();
            AZ_TracePrintf("MaterialDocument", "Material document redo: '%s'.\n", m_absolutePath.c_str());
            MaterialDocumentNotificationBus::Broadcast(&MaterialDocumentNotificationBus::Events::OnDocumentUndoStateChanged, m_id);
            return true;
        }
        return false;
    }

    bool MaterialDocument::BeginEdit()
    {
        // Save the current properties as a momento for undo before any changes are applied
        m_propertyValuesBeforeEdit.clear();
        for (const auto& propertyPair : m_properties)
        {
            const AtomToolsFramework::DynamicProperty& property = propertyPair.second;
            m_propertyValuesBeforeEdit[property.GetId()] = property.GetValue();
        }
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
            // Wipe any state beyond the current history index
            m_undoHistory.erase(m_undoHistory.begin() + m_undoHistoryIndex, m_undoHistory.end());

            // Add undo and redo operations using lambdas that will capture property state and restore it when executed
            m_undoHistory.emplace_back(
                [this, propertyValuesForUndo]() { RestorePropertyValues(propertyValuesForUndo); },
                [this, propertyValuesForRedo]() { RestorePropertyValues(propertyValuesForRedo); });

            // Assign the index to the end of history
            m_undoHistoryIndex = aznumeric_cast<int>(m_undoHistory.size());
            MaterialDocumentNotificationBus::Broadcast(&MaterialDocumentNotificationBus::Events::OnDocumentUndoStateChanged, m_id);
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

    void MaterialDocument::SourceFileChanged(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid sourceUUID)
    {
        if (m_sourceAssetId.m_guid == sourceUUID)
        {
            // ignore notifications caused by saving the open document
            if (!m_saveTriggeredInternally)
            {
                AZ_TracePrintf("MaterialDocument", "Material document changed externally: '%s'.\n", m_absolutePath.c_str());
                MaterialDocumentNotificationBus::Broadcast(&MaterialDocumentNotificationBus::Events::OnDocumentExternallyModified, m_id);
            }
            m_saveTriggeredInternally = false;
        }
    }

    void MaterialDocument::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (m_dependentAssetIds.find(asset->GetId()) != m_dependentAssetIds.end())
        {
            AZ_TracePrintf("MaterialDocument", "Material document dependency changed: '%s'.\n", m_absolutePath.c_str());
            MaterialDocumentNotificationBus::Broadcast(&MaterialDocumentNotificationBus::Events::OnDocumentDependencyModified, m_id);
        }
    }

    bool MaterialDocument::SavePropertiesToSourceData(AZ::RPI::MaterialSourceData& sourceData, PropertyFilterFunction propertyFilter) const
    {
        using namespace AZ;
        using namespace RPI;

        bool result = true;

        // populate sourceData with properties that meet the filter
        m_materialTypeSourceData.EnumerateProperties([this, &sourceData, &propertyFilter, &result](const AZStd::string& groupNameId, const AZStd::string& propertyNameId, const auto& propertyDefinition) {
            const MaterialPropertyId propertyId(groupNameId, propertyNameId);

            const auto it = m_properties.find(propertyId.GetFullName());
            if (it != m_properties.end() && propertyFilter(it->second))
            {
                MaterialPropertyValue propertyValue = AtomToolsFramework::ConvertToRuntimeType(it->second.GetValue());
                if (propertyValue.IsValid())
                {
                    if (!m_materialTypeSourceData.ConvertPropertyValueToSourceDataFormat(propertyDefinition, propertyValue))
                    {
                        AZ_Error("MaterialDocument", false, "Material document property could not be converted: '%s' in '%s'.", propertyId.GetFullName().GetCStr(), m_absolutePath.c_str());
                        result = false;
                        return false;
                    }

                    sourceData.m_properties[groupNameId][propertyNameId].m_value = propertyValue;
                }
            }
            return true;
        });

        return result;
    }

    bool MaterialDocument::OpenInternal(AZStd::string_view loadPath)
    {
        using namespace AZ;
        using namespace RPI;

        Clear();

        m_absolutePath = loadPath;
        if (!AzFramework::StringFunc::Path::Normalize(m_absolutePath))
        {
            AZ_Error("MaterialDocument", false, "Material document path could not be normalized: '%s'.", m_absolutePath.c_str());
            return false;
        }

        if (AzFramework::StringFunc::Path::IsRelative(m_absolutePath.c_str()))
        {
            AZ_Error("MaterialDocument", false, "Material document path must be absolute: '%s'.", m_absolutePath.c_str());
            return false;
        }

        bool result = false;
        Data::AssetInfo sourceAssetInfo;
        AZStd::string watchFolder;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(result, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath,
            m_absolutePath.c_str(), sourceAssetInfo, watchFolder);
        if (!result)
        {
            AZ_Error("MaterialDocument", false, "Could not find source material: '%s'.", m_absolutePath.c_str());
            return false;
        }

        m_sourceAssetId = sourceAssetInfo.m_assetId;
        m_relativePath = sourceAssetInfo.m_relativePath;
        if (!AzFramework::StringFunc::Path::Normalize(m_relativePath))
        {
            AZ_Error("MaterialDocument", false, "Material document path could not be normalized: '%s'.", m_relativePath.c_str());
            return false;
        }

        AZStd::string materialTypeSourceFilePath;

        // The material document and inspector are constructed from source data
        if (AzFramework::StringFunc::Path::IsExtension(m_absolutePath.c_str(), MaterialSourceData::Extension))
        {
            // Load the material source data so that we can check properties and create a material asset from it
            if (!AZ::RPI::JsonUtils::LoadObjectFromFile(m_absolutePath, m_materialSourceData))
            {
                AZ_Error("MaterialDocument", false, "Material source data could not be loaded: '%s'.", m_absolutePath.c_str());
                return false;
            }

            // We must also always load the material type data for a complete, ordered set of the
            // groups and properties that will be needed for comparison and building the inspector
            materialTypeSourceFilePath = AssetUtils::ResolvePathReference(m_absolutePath, m_materialSourceData.m_materialType);
            auto materialTypeOutcome = MaterialUtils::LoadMaterialTypeSourceData(materialTypeSourceFilePath);
            if (!materialTypeOutcome.IsSuccess())
            {
                AZ_Error("MaterialDocument", false, "Material type source data could not be loaded: '%s'.", materialTypeSourceFilePath.c_str());
                return false;
            }
            m_materialTypeSourceData = materialTypeOutcome.GetValue();
        }
        else if (AzFramework::StringFunc::Path::IsExtension(m_absolutePath.c_str(), MaterialTypeSourceData::Extension))
        {
            materialTypeSourceFilePath = m_absolutePath;

            // Load the material type source data, which will be used for enumerating properties and building material source data
            auto materialTypeOutcome = MaterialUtils::LoadMaterialTypeSourceData(materialTypeSourceFilePath);
            if (!materialTypeOutcome.IsSuccess())
            {
                AZ_Error("MaterialDocument", false, "Material type source data could not be loaded: '%s'.", m_absolutePath.c_str());
                return false;
            }
            m_materialTypeSourceData = materialTypeOutcome.GetValue();

            // The document represents a material, not a material type.
            // If the input data is a material type file we have to generate the material source data by referencing it.
            m_materialSourceData.m_materialType = m_relativePath;
            m_materialSourceData.m_parentMaterial.clear();
        }
        else
        {
            AZ_Error("MaterialDocument", false, "Material document extension not supported: '%s'.", m_absolutePath.c_str());
            return false;
        }

        // In order to support automation, general usability, and 'save as' functionality, the user must not have to wait
        // for their JSON file to be cooked by the asset processor before opening or editing it.
        // We need to reduce or remove dependency on the asset processor. In order to get around the bottleneck for now,
        // we can create the asset dynamically from the source data.
        // Long term, the material document should not be concerned with assets at all. The viewport window should be the
        // only thing concerned with assets or instances.
        auto createResult = m_materialSourceData.CreateMaterialAsset(Uuid::CreateRandom(), m_absolutePath, true);
        if (!createResult)
        {
            AZ_Error("MaterialDocument", false, "Material asset could not be created from source data: '%s'.", m_absolutePath.c_str());
            return false;
        }

        m_materialAsset = createResult.GetValue();
        if (!m_materialAsset.IsReady())
        {
            AZ_Error("MaterialDocument", false, "Material asset is not ready: '%s'.", m_absolutePath.c_str());
            return false;
        }

        const auto& materialTypeAsset = m_materialAsset->GetMaterialTypeAsset();
        if (!materialTypeAsset.IsReady())
        {
            AZ_Error("MaterialDocument", false, "Material type asset is not ready: '%s'.", m_absolutePath.c_str());
            return false;
        }

        // track material type asset to notify when dependencies change
        m_dependentAssetIds.insert(materialTypeAsset->GetId());
        AZ::Data::AssetBus::MultiHandler::BusConnect(materialTypeAsset->GetId());

        AZStd::array_view<AZ::RPI::MaterialPropertyValue> parentPropertyValues = materialTypeAsset->GetDefaultPropertyValues();
        AZ::Data::Asset<MaterialAsset> parentMaterialAsset;
        if (!m_materialSourceData.m_parentMaterial.empty())
        {
            // There is a parent for this material
            auto parentMaterialResult = AssetUtils::LoadAsset<MaterialAsset>(m_absolutePath, m_materialSourceData.m_parentMaterial);
            if (!parentMaterialResult)
            {
                AZ_Error("MaterialDocument", false, "Parent material asset could not be loaded: '%s'.", m_materialSourceData.m_parentMaterial.c_str());
                return false;
            }

            parentMaterialAsset = parentMaterialResult.GetValue();
            parentPropertyValues = parentMaterialAsset->GetPropertyValues();

            // track parent material asset to notify when dependencies change
            m_dependentAssetIds.insert(parentMaterialAsset->GetId());
            AZ::Data::AssetBus::MultiHandler::BusConnect(parentMaterialAsset->GetId());
        }

        // Creating a material from a material asset will fail if a texture is referenced but not loaded 
        m_materialInstance = Material::Create(m_materialAsset);
        if (!m_materialInstance)
        {
            AZ_Error("MaterialDocument", false, "Material instance could not be created: '%s'.", m_absolutePath.c_str());
            return false;
        }

        // Populate the property map from a combination of source data and assets
        // Assets must still be used for now because they contain the final accumulated value after all other materials
        // in the hierarchy are applied
        m_materialTypeSourceData.EnumerateProperties([this, &parentPropertyValues](const AZStd::string& groupNameId, const AZStd::string& propertyNameId, const auto& propertyDefinition) {
            AtomToolsFramework::DynamicPropertyConfig propertyConfig;

            // Assign id before conversion so it can be used in dynamic description
            propertyConfig.m_id = MaterialPropertyId(groupNameId, propertyNameId).GetCStr();

            const auto& propertyIndex = m_materialAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(propertyConfig.m_id);
            const bool propertyIndexInBounds = propertyIndex.IsValid() && propertyIndex.GetIndex() < m_materialAsset->GetPropertyValues().size();
            AZ_Warning("MaterialDocument", propertyIndexInBounds, "Failed to add material property '%s' to document '%s'.", propertyConfig.m_id.GetCStr(), m_absolutePath.c_str());

            if (propertyIndexInBounds)
            {
                AtomToolsFramework::ConvertToPropertyConfig(propertyConfig, propertyDefinition);
                propertyConfig.m_showThumbnail = true;
                propertyConfig.m_originalValue = AtomToolsFramework::ConvertToEditableType(m_materialAsset->GetPropertyValues()[propertyIndex.GetIndex()]);
                propertyConfig.m_parentValue = AtomToolsFramework::ConvertToEditableType(parentPropertyValues[propertyIndex.GetIndex()]);
                auto groupDefinition = m_materialTypeSourceData.FindGroup(groupNameId);
                propertyConfig.m_groupName = groupDefinition ? groupDefinition->m_displayName : groupNameId;
                m_properties[propertyConfig.m_id] = AtomToolsFramework::DynamicProperty(propertyConfig);
            }
            return true;
        });

        // Populate the property group visibility map
        for (MaterialTypeSourceData::GroupDefinition& group : m_materialTypeSourceData.GetGroupDefinitionsInDisplayOrder())
        {
            m_propertyGroupVisibility[AZ::Name{group.m_nameId}] = true;
        }

        // Adding properties for material type and parent as part of making dynamic
        // properties and the inspector more general purpose.
        // This allows the read only properties to appear in the inspector like any
        // other property.
        // This may change or be removed once support for changing the material parent
        // is implemented.
        AtomToolsFramework::DynamicPropertyConfig propertyConfig;
        propertyConfig.m_dataType = AtomToolsFramework::DynamicPropertyType::Asset;
        propertyConfig.m_id = "overview.materialType";
        propertyConfig.m_nameId = "materialType";
        propertyConfig.m_displayName = "Material Type";
        propertyConfig.m_groupName = "Overview";
        propertyConfig.m_description = "The material type defines the layout, properties, default values, shader connections, and other "
                                       "data needed to create and edit a derived material.";
        propertyConfig.m_defaultValue = AZStd::any(materialTypeAsset);
        propertyConfig.m_originalValue = propertyConfig.m_defaultValue;
        propertyConfig.m_parentValue = propertyConfig.m_defaultValue;
        propertyConfig.m_readOnly = true;

        m_properties[propertyConfig.m_id] = AtomToolsFramework::DynamicProperty(propertyConfig);

        propertyConfig = {};
        propertyConfig.m_dataType = AtomToolsFramework::DynamicPropertyType::Asset;
        propertyConfig.m_id = "overview.parentMaterial";
        propertyConfig.m_nameId = "parentMaterial";
        propertyConfig.m_displayName = "Parent Material";
        propertyConfig.m_groupName = "Overview";
        propertyConfig.m_description =
            "The parent material provides an initial configuration whose properties are inherited and overriden by a derived material.";
        propertyConfig.m_defaultValue = AZStd::any(parentMaterialAsset);
        propertyConfig.m_originalValue = propertyConfig.m_defaultValue;
        propertyConfig.m_parentValue = propertyConfig.m_defaultValue;
        propertyConfig.m_readOnly = true;
        propertyConfig.m_showThumbnail = true;

        m_properties[propertyConfig.m_id] = AtomToolsFramework::DynamicProperty(propertyConfig);

        //Add UV name customization properties
        const RPI::MaterialUvNameMap& uvNameMap = materialTypeAsset->GetUvNameMap();
        for (const RPI::UvNamePair& uvNamePair : uvNameMap)
        {
            const AZStd::string shaderInput = uvNamePair.m_shaderInput.ToString();
            const AZStd::string uvName = uvNamePair.m_uvName.GetStringView();

            propertyConfig = {};
            propertyConfig.m_dataType = AtomToolsFramework::DynamicPropertyType::String;
            propertyConfig.m_id = MaterialPropertyId(UvGroupName, shaderInput).GetCStr();
            propertyConfig.m_nameId = shaderInput;
            propertyConfig.m_displayName = shaderInput;
            propertyConfig.m_groupName = "UV Sets";
            propertyConfig.m_description = shaderInput;
            propertyConfig.m_defaultValue = uvName;
            propertyConfig.m_originalValue = uvName;
            propertyConfig.m_parentValue = uvName;
            propertyConfig.m_readOnly = true;

            m_properties[propertyConfig.m_id] = AtomToolsFramework::DynamicProperty(propertyConfig);
        }

        const MaterialFunctorSourceData::EditorContext editorContext = MaterialFunctorSourceData::EditorContext(materialTypeSourceFilePath, m_materialAsset->GetMaterialPropertiesLayout());
        for (Ptr<MaterialFunctorSourceDataHolder> functorData : m_materialTypeSourceData.m_materialFunctorSourceData)
        {
            MaterialFunctorSourceData::FunctorResult result2 = functorData->CreateFunctor(editorContext);

            if (result2.IsSuccess())
            {
                Ptr<MaterialFunctor>& functor = result2.GetValue();
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

        AZ::RPI::MaterialPropertyFlags dirtyFlags;
        dirtyFlags.set(); // Mark all properties as dirty since we just loaded the material and need to initialize property visibility
        RunEditorMaterialFunctors(dirtyFlags);

        // Connecting to bus to monitor external changes
        AzToolsFramework::AssetSystemBus::Handler::BusConnect();

        AZ_TracePrintf("MaterialDocument", "Material document opened: '%s'.\n", m_absolutePath.c_str());
        return true;
    }

    void MaterialDocument::Recompile()
    {
        if (!m_compilePending)
        {
            AZ::TickBus::Handler::BusConnect();
            m_compilePending = true;
        }
    }

    void MaterialDocument::Clear()
    {
        AZ::TickBus::Handler::BusDisconnect();
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
        AzToolsFramework::AssetSystemBus::Handler::BusDisconnect();

        m_materialAsset = {};
        m_materialInstance = {};
        m_absolutePath.clear();
        m_relativePath.clear();
        m_sourceAssetId = {};
        m_dependentAssetIds.clear();
        m_saveTriggeredInternally = {};
        m_compilePending = {};
        m_properties.clear();
        m_editorFunctors.clear();
        m_materialTypeSourceData = AZ::RPI::MaterialTypeSourceData();
        m_materialSourceData = AZ::RPI::MaterialSourceData();
        m_propertyValuesBeforeEdit.clear();
        m_undoHistory.clear();
        m_undoHistoryIndex = {};
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

    MaterialDocument::EditorMaterialFunctorResult MaterialDocument::RunEditorMaterialFunctors(AZ::RPI::MaterialPropertyFlags dirtyFlags)
    {
        EditorMaterialFunctorResult result;

        AZStd::unordered_map<AZ::Name, AZ::RPI::MaterialPropertyDynamicMetadata> propertyDynamicMetadata;
        AZStd::unordered_map<AZ::Name, AZ::RPI::MaterialPropertyGroupDynamicMetadata> propertyGroupDynamicMetadata;
        for (auto& propertyPair : m_properties)
        {
            AtomToolsFramework::DynamicProperty& property = propertyPair.second;
            AtomToolsFramework::ConvertToPropertyMetaData(propertyDynamicMetadata[property.GetId()], property.GetConfig());
        }
        for (auto& groupPair : m_propertyGroupVisibility)
        {
            AZ::RPI::MaterialPropertyGroupDynamicMetadata& metadata = propertyGroupDynamicMetadata[AZ::Name{groupPair.first}];

            bool visible = groupPair.second;
            metadata.m_visibility = visible ?
                AZ::RPI::MaterialPropertyGroupVisibility::Enabled : AZ::RPI::MaterialPropertyGroupVisibility::Hidden;
        }
        
        for (AZ::RPI::Ptr<AZ::RPI::MaterialFunctor>& functor : m_editorFunctors)
        {
            const AZ::RPI::MaterialPropertyFlags& materialPropertyDependencies = functor->GetMaterialPropertyDependencies();
            // None also covers case that the client code doesn't register material properties to dependencies,
            // which will later get caught in Process() when trying to access a property.
            if (materialPropertyDependencies.none() || functor->NeedsProcess(dirtyFlags))
            {
                AZ::RPI::MaterialFunctor::EditorContext context = AZ::RPI::MaterialFunctor::EditorContext(
                    m_materialInstance->GetPropertyValues(),
                    m_materialInstance->GetMaterialPropertiesLayout(),
                    propertyDynamicMetadata,
                    propertyGroupDynamicMetadata,
                    result.m_updatedProperties,
                    result.m_updatedPropertyGroups,
                    &materialPropertyDependencies
                );
                functor->Process(context);
            }
        }

        for (auto& propertyPair : m_properties)
        {
            AtomToolsFramework::DynamicProperty& property = propertyPair.second;
            AtomToolsFramework::DynamicPropertyConfig propertyConfig = property.GetConfig();
            AtomToolsFramework::ConvertToPropertyConfig(propertyConfig, propertyDynamicMetadata[property.GetId()]);
            property.SetConfig(propertyConfig);
        }

        for (auto& updatedPropertyGroup : result.m_updatedPropertyGroups)
        {
            bool visible = propertyGroupDynamicMetadata[updatedPropertyGroup].m_visibility == AZ::RPI::MaterialPropertyGroupVisibility::Enabled;
            m_propertyGroupVisibility[updatedPropertyGroup] = visible;
        }

        return result;
    }

} // namespace MaterialEditor
