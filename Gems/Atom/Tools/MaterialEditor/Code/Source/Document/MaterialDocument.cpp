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
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Reflect/Image/Image.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <AtomCore/Instance/Instance.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/Util/MaterialPropertyUtil.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <Document/MaterialDocument.h>

namespace MaterialEditor
{
    MaterialDocument::MaterialDocument()
        : AtomToolsFramework::AtomToolsDocument()
    {
        MaterialDocumentRequestBus::Handler::BusConnect(m_id);
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Broadcast(&AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentCreated, m_id);
    }

    MaterialDocument::~MaterialDocument()
    {
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Broadcast(&AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentDestroyed, m_id);
        MaterialDocumentRequestBus::Handler::BusDisconnect();
        Clear();
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

    const AZStd::any& MaterialDocument::GetPropertyValue(const AZ::Name& propertyId) const
    {
        using namespace AZ;
        using namespace RPI;

        if (!IsOpen())
        {
            AZ_Error("MaterialDocument", false, "Material document is not open.");
            return m_invalidValue;
        }

        const auto it = m_properties.find(propertyId);
        if (it == m_properties.end())
        {
            AZ_Error("MaterialDocument", false, "Material document property could not be found: '%s'.", propertyId.GetCStr());
            return m_invalidValue;
        }

        const AtomToolsFramework::DynamicProperty& property = it->second;
        return property.GetValue();
    }

    const AtomToolsFramework::DynamicProperty& MaterialDocument::GetProperty(const AZ::Name& propertyId) const
    {
        if (!IsOpen())
        {
            AZ_Error("MaterialDocument", false, "Material document is not open.");
            return m_invalidProperty;
        }

        const auto it = m_properties.find(propertyId);
        if (it == m_properties.end())
        {
            AZ_Error("MaterialDocument", false, "Material document property could not be found: '%s'.", propertyId.GetCStr());
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

    void MaterialDocument::SetPropertyValue(const AZ::Name& propertyId, const AZStd::any& value)
    {
        using namespace AZ;
        using namespace RPI;

        if (!IsOpen())
        {
            AZ_Error("MaterialDocument", false, "Material document is not open.");
            return;
        }

        const auto it = m_properties.find(propertyId);
        if (it == m_properties.end())
        {
            AZ_Error("MaterialDocument", false, "Material document property could not be found: '%s'.", propertyId.GetCStr());
            return;
        }

        // This first converts to an acceptable runtime type in case the value came from script
        const AZ::RPI::MaterialPropertyValue propertyValue =
            AtomToolsFramework::ConvertToRuntimeType(value);

        AtomToolsFramework::DynamicProperty& property = it->second;
        property.SetValue(AtomToolsFramework::ConvertToEditableType(propertyValue));

        const auto propertyIndex = m_materialInstance->FindPropertyIndex(propertyId);
        if (!propertyIndex.IsNull())
        {
            if (m_materialInstance->SetPropertyValue(propertyIndex, propertyValue))
            {
                MaterialPropertyFlags dirtyFlags = m_materialInstance->GetPropertyDirtyFlags();

                Recompile();

                EditorMaterialFunctorResult result = RunEditorMaterialFunctors(dirtyFlags);
                for (const Name& changedPropertyGroupName : result.m_updatedPropertyGroups)
                {
                    AtomToolsFramework::AtomToolsDocumentNotificationBus::Broadcast(&AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentPropertyGroupVisibilityChanged, m_id, changedPropertyGroupName, IsPropertyGroupVisible(changedPropertyGroupName));
                }
                for (const Name& changedPropertyName : result.m_updatedProperties)
                {
                    AtomToolsFramework::AtomToolsDocumentNotificationBus::Broadcast(&AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentPropertyConfigModified, m_id, GetProperty(changedPropertyName));
                }
            }
        }

        AtomToolsFramework::AtomToolsDocumentNotificationBus::Broadcast(&AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentPropertyValueModified, m_id, property);
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Broadcast(&AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentModified, m_id);
    }

    bool MaterialDocument::Open(AZStd::string_view loadPath)
    {
        if (!OpenInternal(loadPath))
        {
            Clear();
            AZ_Error("MaterialDocument", false, "Material document could not be opened: '%s'.", loadPath.data());
            return false;
        }

        AtomToolsFramework::AtomToolsDocumentNotificationBus::Broadcast(&AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentOpened, m_id);
        return true;
    }

    bool MaterialDocument::Reopen()
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
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Broadcast(&AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentOpened, m_id);
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
        sourceData.m_materialTypeVersion = m_materialAsset->GetMaterialTypeAsset()->GetVersion();
        sourceData.m_materialType = AtomToolsFramework::GetExteralReferencePath(m_absolutePath, m_materialSourceData.m_materialType);
        sourceData.m_parentMaterial = AtomToolsFramework::GetExteralReferencePath(m_absolutePath, m_materialSourceData.m_parentMaterial);

        // populate sourceData with modified or overwritten properties
        const bool savedProperties = SavePropertiesToSourceData(m_absolutePath, sourceData, [](const AtomToolsFramework::DynamicProperty& property)
        {
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

        AtomToolsFramework::AtomToolsDocumentNotificationBus::Broadcast(&AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentSaved, m_id);

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
        sourceData.m_materialTypeVersion = m_materialAsset->GetMaterialTypeAsset()->GetVersion();
        sourceData.m_materialType = AtomToolsFramework::GetExteralReferencePath(normalizedSavePath, m_materialSourceData.m_materialType);
        sourceData.m_parentMaterial = AtomToolsFramework::GetExteralReferencePath(normalizedSavePath, m_materialSourceData.m_parentMaterial);

        // populate sourceData with modified or overwritten properties
        const bool savedProperties = SavePropertiesToSourceData(normalizedSavePath, sourceData, [](const AtomToolsFramework::DynamicProperty& property)
        {
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

        AtomToolsFramework::AtomToolsDocumentNotificationBus::Broadcast(&AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentSaved, m_id);

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
        sourceData.m_materialTypeVersion = m_materialAsset->GetMaterialTypeAsset()->GetVersion();
        sourceData.m_materialType = AtomToolsFramework::GetExteralReferencePath(normalizedSavePath, m_materialSourceData.m_materialType);

        // Only assign a parent path if the source was a .material
        if (AzFramework::StringFunc::Path::IsExtension(m_relativePath.c_str(), MaterialSourceData::Extension))
        {
            sourceData.m_parentMaterial = AtomToolsFramework::GetExteralReferencePath(normalizedSavePath, m_absolutePath);
        }

        // populate sourceData with modified properties
        const bool savedProperties = SavePropertiesToSourceData(normalizedSavePath, sourceData, [](const AtomToolsFramework::DynamicProperty& property)
        {
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

        AtomToolsFramework::AtomToolsDocumentNotificationBus::Broadcast(&AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentSaved, m_id);

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

        AtomToolsFramework::AtomToolsDocumentNotificationBus::Broadcast(&AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentClosed, m_id);

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
            AtomToolsFramework::AtomToolsDocumentNotificationBus::Broadcast(&AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentUndoStateChanged, m_id);
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
            AtomToolsFramework::AtomToolsDocumentNotificationBus::Broadcast(&AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentUndoStateChanged, m_id);
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
            AtomToolsFramework::AtomToolsDocumentNotificationBus::Broadcast(&AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentUndoStateChanged, m_id);
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

    void MaterialDocument::SourceFileChanged(AZStd::string relativePath, AZStd::string scanFolder, [[maybe_unused]] AZ::Uuid sourceUUID)
    {
        auto sourcePath = AZ::RPI::AssetUtils::ResolvePathReference(scanFolder, relativePath);

        if (m_absolutePath == sourcePath)
        {
            // ignore notifications caused by saving the open document
            if (!m_saveTriggeredInternally)
            {
                AZ_TracePrintf("MaterialDocument", "Material document changed externally: '%s'.\n", m_absolutePath.c_str());
                AtomToolsFramework::AtomToolsDocumentNotificationBus::Broadcast(
                    &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentExternallyModified, m_id);
            }
            m_saveTriggeredInternally = false;
        }
        else if (m_sourceDependencies.find(sourcePath) != m_sourceDependencies.end())
        {
            AZ_TracePrintf("MaterialDocument", "Material document dependency changed: '%s'.\n", m_absolutePath.c_str());
            AtomToolsFramework::AtomToolsDocumentNotificationBus::Broadcast(
                &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentDependencyModified, m_id);
        }
    }

    bool MaterialDocument::SavePropertiesToSourceData(
        const AZStd::string& exportPath, AZ::RPI::MaterialSourceData& sourceData, PropertyFilterFunction propertyFilter) const
    {
        using namespace AZ;
        using namespace RPI;

        bool result = true;

        // populate sourceData with properties that meet the filter
        m_materialTypeSourceData.EnumerateProperties([&](const AZStd::string& groupName, const AZStd::string& propertyName, const auto& propertyDefinition) {

            const MaterialPropertyId propertyId(groupName, propertyName);

            const auto it = m_properties.find(propertyId.GetFullName());
            if (it != m_properties.end() && propertyFilter(it->second))
            {
                MaterialPropertyValue propertyValue = AtomToolsFramework::ConvertToRuntimeType(it->second.GetValue());
                if (propertyValue.IsValid())
                {
                    if (!AtomToolsFramework::ConvertToExportFormat(exportPath, propertyId.GetFullName(), propertyDefinition, propertyValue))
                    {
                        AZ_Error("MaterialDocument", false, "Material document property could not be converted: '%s' in '%s'.", propertyId.GetFullName().GetCStr(), m_absolutePath.c_str());
                        result = false;
                        return false;
                    }

                    sourceData.m_properties[groupName][propertyName].m_value = propertyValue;
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

        m_relativePath = sourceAssetInfo.m_relativePath;
        if (!AzFramework::StringFunc::Path::Normalize(m_relativePath))
        {
            AZ_Error("MaterialDocument", false, "Material document path could not be normalized: '%s'.", m_relativePath.c_str());
            return false;
        }

        // The material document and inspector are constructed from source data
        if (AzFramework::StringFunc::Path::IsExtension(m_absolutePath.c_str(), MaterialSourceData::Extension))
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
                    AssetUtils::ResolvePathReference(m_absolutePath, m_materialSourceData.m_parentMaterial);
            }

            if (!m_materialSourceData.m_materialType.empty())
            {
                m_materialSourceData.m_materialType = AssetUtils::ResolvePathReference(m_absolutePath, m_materialSourceData.m_materialType);
            }

            // Load the material type source data which provides the layout and default values of all of the properties
            auto materialTypeOutcome = MaterialUtils::LoadMaterialTypeSourceData(m_materialSourceData.m_materialType);
            if (!materialTypeOutcome.IsSuccess())
            {
                AZ_Error("MaterialDocument", false, "Material type source data could not be loaded: '%s'.", m_materialSourceData.m_materialType.c_str());
                return false;
            }
            m_materialTypeSourceData = materialTypeOutcome.GetValue();
            
            if (MaterialSourceData::ApplyVersionUpdatesResult::Failed == m_materialSourceData.ApplyVersionUpdates(m_absolutePath))
            {
                AZ_Error("MaterialDocument", false, "Material source data could not be auto updated to the latest version of the material type: '%s'.", m_materialSourceData.m_materialType.c_str());
                return false;
            }
        }
        else if (AzFramework::StringFunc::Path::IsExtension(m_absolutePath.c_str(), MaterialTypeSourceData::Extension))
        {
            // A material document can be created or loaded from material or material type source data. If we are attempting to load
            // material type source data then the material source data object can be created just by referencing the document path as the
            // material type path.
            auto materialTypeOutcome = MaterialUtils::LoadMaterialTypeSourceData(m_absolutePath);
            if (!materialTypeOutcome.IsSuccess())
            {
                AZ_Error("MaterialDocument", false, "Material type source data could not be loaded: '%s'.", m_absolutePath.c_str());
                return false;
            }
            m_materialTypeSourceData = materialTypeOutcome.GetValue();

            // We are storing absolute paths in the loaded version of the source data so that the files can be resolved at all times.
            m_materialSourceData.m_materialType = m_absolutePath;
            m_materialSourceData.m_parentMaterial.clear();
        }
        else
        {
            AZ_Error("MaterialDocument", false, "Material document extension not supported: '%s'.", m_absolutePath.c_str());
            return false;
        }
        
        const bool elevateWarnings = false;

        // In order to support automation, general usability, and 'save as' functionality, the user must not have to wait
        // for their JSON file to be cooked by the asset processor before opening or editing it.
        // We need to reduce or remove dependency on the asset processor. In order to get around the bottleneck for now,
        // we can create the asset dynamically from the source data.
        // Long term, the material document should not be concerned with assets at all. The viewport window should be the
        // only thing concerned with assets or instances.
        auto materialAssetResult =
            m_materialSourceData.CreateMaterialAssetFromSourceData(Uuid::CreateRandom(), m_absolutePath, elevateWarnings, true, &m_sourceDependencies);
        if (!materialAssetResult)
        {
            AZ_Error("MaterialDocument", false, "Material asset could not be created from source data: '%s'.", m_absolutePath.c_str());
            return false;
        }

        m_materialAsset = materialAssetResult.GetValue();
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

        AZStd::array_view<AZ::RPI::MaterialPropertyValue> parentPropertyValues = materialTypeAsset->GetDefaultPropertyValues();
        AZ::Data::Asset<MaterialAsset> parentMaterialAsset;
        if (!m_materialSourceData.m_parentMaterial.empty())
        {
            AZ::RPI::MaterialSourceData parentMaterialSourceData;
            if (!AZ::RPI::JsonUtils::LoadObjectFromFile(m_materialSourceData.m_parentMaterial, parentMaterialSourceData))
            {
                AZ_Error("MaterialDocument", false, "Material parent source data could not be loaded for: '%s'.", m_materialSourceData.m_parentMaterial.c_str());
                return false;
            }

            const auto parentMaterialAssetIdResult = AssetUtils::MakeAssetId(m_materialSourceData.m_parentMaterial, 0);
            if (!parentMaterialAssetIdResult)
            {
                AZ_Error("MaterialDocument", false, "Material parent asset ID could not be created: '%s'.", m_materialSourceData.m_parentMaterial.c_str());
                return false;
            }
            
            auto parentMaterialAssetResult = parentMaterialSourceData.CreateMaterialAssetFromSourceData(
                parentMaterialAssetIdResult.GetValue(), m_materialSourceData.m_parentMaterial, true, true);
            if (!parentMaterialAssetResult)
            {
                AZ_Error("MaterialDocument", false, "Material parent asset could not be created from source data: '%s'.", m_materialSourceData.m_parentMaterial.c_str());
                return false;
            }

            parentMaterialAsset = parentMaterialAssetResult.GetValue();
            parentPropertyValues = parentMaterialAsset->GetPropertyValues();
        }

        // Creating a material from a material asset will fail if a texture is referenced but not loaded 
        m_materialInstance = Material::Create(m_materialAsset);
        if (!m_materialInstance)
        {
            AZ_Error("MaterialDocument", false, "Material instance could not be created: '%s'.", m_absolutePath.c_str());
            return false;
        }

        // Pipeline State Object changes are always allowed in the material editor because it only runs on developer systems
        // where such changes are supported at runtime.
        m_materialInstance->SetPsoHandlingOverride(AZ::RPI::MaterialPropertyPsoHandling::Allowed);

        // Populate the property map from a combination of source data and assets
        // Assets must still be used for now because they contain the final accumulated value after all other materials
        // in the hierarchy are applied
        m_materialTypeSourceData.EnumerateProperties([this, &parentPropertyValues](const AZStd::string& groupName, const AZStd::string& propertyName, const auto& propertyDefinition) {
            AtomToolsFramework::DynamicPropertyConfig propertyConfig;

            // Assign id before conversion so it can be used in dynamic description
            propertyConfig.m_id = MaterialPropertyId(groupName, propertyName).GetCStr();

            const auto& propertyIndex = m_materialAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(propertyConfig.m_id);
            const bool propertyIndexInBounds = propertyIndex.IsValid() && propertyIndex.GetIndex() < m_materialAsset->GetPropertyValues().size();
            AZ_Warning("MaterialDocument", propertyIndexInBounds, "Failed to add material property '%s' to document '%s'.", propertyConfig.m_id.GetCStr(), m_absolutePath.c_str());

            if (propertyIndexInBounds)
            {
                AtomToolsFramework::ConvertToPropertyConfig(propertyConfig, propertyDefinition);
                propertyConfig.m_showThumbnail = true;
                propertyConfig.m_originalValue = AtomToolsFramework::ConvertToEditableType(m_materialAsset->GetPropertyValues()[propertyIndex.GetIndex()]);
                propertyConfig.m_parentValue = AtomToolsFramework::ConvertToEditableType(parentPropertyValues[propertyIndex.GetIndex()]);
                auto groupDefinition = m_materialTypeSourceData.FindGroup(groupName);
                propertyConfig.m_groupName = groupDefinition ? groupDefinition->m_displayName : groupName;
                m_properties[propertyConfig.m_id] = AtomToolsFramework::DynamicProperty(propertyConfig);
            }
            return true;
        });

        // Populate the property group visibility map
        for (MaterialTypeSourceData::GroupDefinition& group : m_materialTypeSourceData.GetGroupDefinitionsInDisplayOrder())
        {
            m_propertyGroupVisibility[AZ::Name{group.m_name}] = true;
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
        propertyConfig.m_name = "materialType";
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
        propertyConfig.m_name = "parentMaterial";
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
            propertyConfig.m_name = shaderInput;
            propertyConfig.m_displayName = shaderInput;
            propertyConfig.m_groupName = "UV Sets";
            propertyConfig.m_description = shaderInput;
            propertyConfig.m_defaultValue = uvName;
            propertyConfig.m_originalValue = uvName;
            propertyConfig.m_parentValue = uvName;
            propertyConfig.m_readOnly = true;

            m_properties[propertyConfig.m_id] = AtomToolsFramework::DynamicProperty(propertyConfig);
        }

        const MaterialFunctorSourceData::EditorContext editorContext =
            MaterialFunctorSourceData::EditorContext(m_materialSourceData.m_materialType, m_materialAsset->GetMaterialPropertiesLayout());
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
        AzToolsFramework::AssetSystemBus::Handler::BusDisconnect();

        m_materialAsset = {};
        m_materialInstance = {};
        m_absolutePath.clear();
        m_relativePath.clear();
        m_sourceDependencies.clear();
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
