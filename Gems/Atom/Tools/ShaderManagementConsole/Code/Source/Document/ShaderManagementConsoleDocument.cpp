/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetDatabase/AssetDatabaseConnection.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <Document/ShaderManagementConsoleDocument.h>

namespace ShaderManagementConsole
{
    void ShaderManagementConsoleDocument::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ShaderManagementConsoleDocument, AtomToolsFramework::AtomToolsDocument>()
                ->Version(0);
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<ShaderManagementConsoleDocumentRequestBus>("ShaderManagementConsoleDocumentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "shadermanagementconsole")
                ->Event("SetShaderVariantListSourceData", &ShaderManagementConsoleDocumentRequestBus::Events::SetShaderVariantListSourceData)
                ->Event("GetShaderVariantListSourceData", &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderVariantListSourceData)
                ->Event("GetShaderOptionDescriptorCount", &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderOptionDescriptorCount)
                ->Event("GetShaderOptionDescriptor", &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderOptionDescriptor)
                ;
        }
    }

    ShaderManagementConsoleDocument::ShaderManagementConsoleDocument(
        const AZ::Crc32& toolId, const AtomToolsFramework::DocumentTypeInfo& documentTypeInfo)
        : AtomToolsFramework::AtomToolsDocument(toolId, documentTypeInfo)
    {
        ShaderManagementConsoleDocumentRequestBus::Handler::BusConnect(m_id);
    }

    ShaderManagementConsoleDocument::~ShaderManagementConsoleDocument()
    {
        ShaderManagementConsoleDocumentRequestBus::Handler::BusDisconnect();
    }

    void ShaderManagementConsoleDocument::SetShaderVariantListSourceData(
        const AZ::RPI::ShaderVariantListSourceData& shaderVariantListSourceData)
    {
        m_shaderVariantListSourceData = shaderVariantListSourceData;
        AZStd::string shaderPath = m_shaderVariantListSourceData.m_shaderFilePath;
        AzFramework::StringFunc::Path::ReplaceExtension(shaderPath, AZ::RPI::ShaderAsset::Extension);

        m_shaderAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::ShaderAsset>(shaderPath.c_str());
        AZ_Error("ShaderManagementConsoleDocument", m_shaderAsset.IsReady(), "Could not load shader asset: %s.", shaderPath.c_str());

        AtomToolsFramework::AtomToolsDocumentNotificationBus::Event(
            m_toolId, &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentObjectInfoInvalidated, m_id);
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Event(
            m_toolId, &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentModified, m_id);
        m_modified = true;
    }

    const AZ::RPI::ShaderVariantListSourceData& ShaderManagementConsoleDocument::GetShaderVariantListSourceData() const
    {
        return m_shaderVariantListSourceData;
    }

    size_t ShaderManagementConsoleDocument::GetShaderOptionDescriptorCount() const
    {
        if (IsOpen())
        {
            const auto& layout = m_shaderAsset->GetShaderOptionGroupLayout();
            const auto& shaderOptionDescriptors = layout->GetShaderOptions();
            return shaderOptionDescriptors.size();
        }
        return 0;
    }

    const AZ::RPI::ShaderOptionDescriptor& ShaderManagementConsoleDocument::GetShaderOptionDescriptor(size_t index) const
    {
        if (IsOpen())
        {
            const auto& layout = m_shaderAsset->GetShaderOptionGroupLayout();
            const auto& shaderOptionDescriptors = layout->GetShaderOptions();
            return shaderOptionDescriptors.at(index);
        }
        return m_invalidDescriptor;
    }

    AtomToolsFramework::DocumentTypeInfo ShaderManagementConsoleDocument::BuildDocumentTypeInfo()
    {
        AtomToolsFramework::DocumentTypeInfo documentType;
        documentType.m_documentTypeName = "Shader Variant List";
        documentType.m_documentFactoryCallback = [](const AZ::Crc32& toolId, const AtomToolsFramework::DocumentTypeInfo& documentTypeInfo) {
            return aznew ShaderManagementConsoleDocument(toolId, documentTypeInfo); };
        documentType.m_supportedExtensionsToCreate.push_back({ "Shader", AZ::RPI::ShaderSourceData::Extension });
        documentType.m_supportedExtensionsToCreate.push_back({ "Shader Variant List", AZ::RPI::ShaderVariantListSourceData::Extension });
        documentType.m_supportedExtensionsToOpen.push_back({ "Shader", AZ::RPI::ShaderSourceData::Extension });
        documentType.m_supportedExtensionsToOpen.push_back({ "Shader Variant List", AZ::RPI::ShaderVariantListSourceData::Extension });
        documentType.m_supportedExtensionsToSave.push_back({ "Shader Variant List", AZ::RPI::ShaderVariantListSourceData::Extension });
        documentType.m_supportedAssetTypesToCreate.insert(azrtti_typeid<AZ::RPI::ShaderAsset>());
        return documentType;
    }

    AtomToolsFramework::DocumentObjectInfoVector ShaderManagementConsoleDocument::GetObjectInfo() const
    {
        if (!IsOpen())
        {
            AZ_Error("ShaderManagementConsoleDocument", false, "Document is not open.");
            return {};
        }

        AtomToolsFramework::DocumentObjectInfoVector objects;

        AtomToolsFramework::DocumentObjectInfo objectInfo;
        objectInfo.m_visible = true;
        objectInfo.m_name = "Shader Variant List";
        objectInfo.m_displayName = "Shader Variant List";
        objectInfo.m_description = "Shader Variant List";
        objectInfo.m_objectType = azrtti_typeid<AZ::RPI::ShaderVariantListSourceData>();
        objectInfo.m_objectPtr = const_cast<AZ::RPI::ShaderVariantListSourceData*>(&m_shaderVariantListSourceData);
        objects.push_back(AZStd::move(objectInfo));

        return objects;
    }

    bool ShaderManagementConsoleDocument::Open(const AZStd::string& loadPath)
    {
        if (!AtomToolsDocument::Open(loadPath))
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

        if (AzFramework::StringFunc::Path::IsExtension(m_absolutePath.c_str(), AZ::RPI::ShaderSourceData::Extension))
        {
            return LoadShaderSourceData();
        }

        if (AzFramework::StringFunc::Path::IsExtension(m_absolutePath.c_str(), AZ::RPI::ShaderVariantListSourceData::Extension))
        {
            return LoadShaderVariantListSourceData();
        }

        AZ_Error("ShaderManagementConsoleDocument", false, "Document extension is not supported: '%s.'", m_absolutePath.c_str());
        return OpenFailed();
    }

    bool ShaderManagementConsoleDocument::Save()
    {
        if (!AtomToolsDocument::Save())
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

        return SaveSourceData();
    }

    bool ShaderManagementConsoleDocument::SaveAsCopy(const AZStd::string& savePath)
    {
        if (!AtomToolsDocument::SaveAsCopy(savePath))
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

        return SaveSourceData();
    }

    bool ShaderManagementConsoleDocument::SaveAsChild(const AZStd::string& savePath)
    {
        if (!AtomToolsDocument::SaveAsChild(savePath))
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

        return SaveSourceData();
    }

    bool ShaderManagementConsoleDocument::IsOpen() const
    {
        return AtomToolsDocument::IsOpen() && m_shaderAsset.IsReady();
    }

    bool ShaderManagementConsoleDocument::IsModified() const
    {
        return m_modified;
    }

    bool ShaderManagementConsoleDocument::BeginEdit()
    {
        // Save the current properties as a momento for undo before any changes are applied
        m_shaderVariantListSourceDataBeforeEdit = m_shaderVariantListSourceData;
        return true;
    }

    bool ShaderManagementConsoleDocument::EndEdit()
    {
        bool modified = false;

        // Lazy evaluation, comparing the current and previous shader variant list source data state to determine if we need to record undo/redo history.
        // TODO Refine this so that only the deltas are stored.
        const auto& undoState = m_shaderVariantListSourceDataBeforeEdit;
        const auto& redoState = m_shaderVariantListSourceData;
        if (undoState.m_shaderFilePath != redoState.m_shaderFilePath ||
            undoState.m_shaderVariants.size() != redoState.m_shaderVariants.size())
        {
            modified = true;
        }
        else
        {
            for (size_t i = 0; i < redoState.m_shaderVariants.size(); ++i)
            {
                if (undoState.m_shaderVariants[i].m_stableId != redoState.m_shaderVariants[i].m_stableId ||
                    undoState.m_shaderVariants[i].m_options != redoState.m_shaderVariants[i].m_options)
                {
                    modified = true;
                    break;
                }
            }
        }

        if (modified)
        {
            AddUndoRedoHistory(
                [this, undoState]() { SetShaderVariantListSourceData(undoState); },
                [this, redoState]() { SetShaderVariantListSourceData(redoState); });
        }

        m_shaderVariantListSourceDataBeforeEdit = {};
        return true;
    }

    void ShaderManagementConsoleDocument::Clear()
    {
        AtomToolsFramework::AtomToolsDocument::Clear();

        m_shaderVariantListSourceData = {};
        m_shaderVariantListSourceDataBeforeEdit = {};
        m_shaderAsset = {};
        m_modified = {};
    }

    bool ShaderManagementConsoleDocument::SaveSourceData()
    {
        if (!AZ::RPI::JsonUtils::SaveObjectToFile(m_savePathNormalized, m_shaderVariantListSourceData))
        {
            AZ_Error("ShaderManagementConsoleDocument", false, "Document could not be saved: '%s'.", m_savePathNormalized.c_str());
            return SaveFailed();
        }

        m_absolutePath = m_savePathNormalized;
        m_modified = {};
        return SaveSucceeded();
    }

    bool ShaderManagementConsoleDocument::LoadShaderSourceData()
    {
        // Get info such as relative path of the file and asset id
        bool result = false;
        AZ::Data::AssetInfo shaderAssetInfo;
        AZStd::string watchFolder;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            result, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, m_absolutePath.c_str(), shaderAssetInfo,
            watchFolder);

        if (!result)
        {
            AZ_Error("ShaderManagementConsoleDocument", false, "Failed to get the asset info for the file: %s.", m_absolutePath.c_str());
            return OpenFailed();
        }

        // retrieves a list of all material source files that use the shader. Note that materials inherit from materialtype files, which
        // are actual files that refer to shader files.
        const auto& materialAssetIds = FindMaterialAssetsUsingShader(shaderAssetInfo.m_relativePath);

        // This loop collects all uniquely-identified shader items used by the materials based on its shader variant id.
        AZStd::set<AZ::RPI::ShaderVariantId> shaderVariantIds;
        AZStd::vector<AZ::RPI::ShaderOptionGroup> shaderVariantListShaderOptionGroups;
        for (const auto& materialAssetId : materialAssetIds)
        {
            const auto& materialInstanceShaderItems = GetMaterialInstanceShaderItems(materialAssetId);
            for (const auto& shaderItem : materialInstanceShaderItems)
            {
                const auto& shaderAssetId = shaderItem.GetShaderAsset().GetId();
                if (shaderAssetInfo.m_assetId == shaderAssetId)
                {
                    const auto& shaderVariantId = shaderItem.GetShaderVariantId();
                    if (!shaderVariantId.IsEmpty() && shaderVariantIds.insert(shaderVariantId).second)
                    {
                        shaderVariantListShaderOptionGroups.push_back(shaderItem.GetShaderOptionGroup());
                    }
                }
            }
        }

        // Generate the shader variant list data by collecting shader option name-value pairs.s
        AZ::RPI::ShaderVariantListSourceData shaderVariantListSourceData;
        shaderVariantListSourceData.m_shaderFilePath = shaderAssetInfo.m_relativePath;
        int stableId = 1;
        for (const auto& shaderOptionGroup : shaderVariantListShaderOptionGroups)
        {
            AZ::RPI::ShaderVariantListSourceData::VariantInfo variantInfo;
            variantInfo.m_stableId = stableId;

            const auto& shaderOptionDescriptors = shaderOptionGroup.GetShaderOptionDescriptors();
            for (const auto& shaderOptionDescriptor : shaderOptionDescriptors)
            {
                const auto& optionName = shaderOptionDescriptor.GetName();
                const auto& optionValue = shaderOptionGroup.GetValue(optionName);
                if (!optionValue.IsValid())
                {
                    continue;
                }

                const auto& valueName = shaderOptionDescriptor.GetValueName(optionValue);
                variantInfo.m_options[optionName.GetStringView()] = valueName.GetStringView();
            }

            if (!variantInfo.m_options.empty())
            {
                shaderVariantListSourceData.m_shaderVariants.push_back(variantInfo);
                stableId++;
            }
        }

        // Even though the data originated from a different file source it has now been transformed into a shader variant list so update the
        // extension to match. This will allow the document to be resaved immediately without doing a save as child or derived type when
        // loaded from a shader source file.
        AzFramework::StringFunc::Path::ReplaceExtension(m_absolutePath, AZ::RPI::ShaderVariantListSourceData::Extension);

        SetShaderVariantListSourceData(shaderVariantListSourceData);
        m_modified = {};

        return IsOpen() ? OpenSucceeded() : OpenFailed();
    }

    bool ShaderManagementConsoleDocument::LoadShaderVariantListSourceData()
    {
        // Load previously generated shader variant list source data
        AZ::RPI::ShaderVariantListSourceData shaderVariantListSourceData;
        if (!AZ::RPI::JsonUtils::LoadObjectFromFile(m_absolutePath, shaderVariantListSourceData))
        {
            AZ_Error("ShaderManagementConsoleDocument", false, "Failed loading shader variant list data: '%s.'", m_absolutePath.c_str());
            return OpenFailed();
        }

        SetShaderVariantListSourceData(shaderVariantListSourceData);
        m_modified = {};

        return IsOpen() ? OpenSucceeded() : OpenFailed();
    }

    AZStd::vector<AZ::Data::AssetId> ShaderManagementConsoleDocument::FindMaterialAssetsUsingShader(const AZStd::string& shaderFilePath)
    {
        AzToolsFramework::AssetDatabase::AssetDatabaseConnection assetDatabaseConnection;
        assetDatabaseConnection.OpenDatabase();

        // Find all material types that reference shaderFilePath
        AZStd::list<AZStd::string> materialTypeSources;

        assetDatabaseConnection.QuerySourceDependencyByDependsOnSource(
            shaderFilePath.c_str(), nullptr, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_Any,
            [&](AzToolsFramework::AssetDatabase::SourceFileDependencyEntry& sourceFileDependencyEntry)
            {
                if (AzFramework::StringFunc::Path::IsExtension(
                        sourceFileDependencyEntry.m_source.c_str(), AZ::RPI::MaterialTypeSourceData::Extension))
                {
                    materialTypeSources.push_back(sourceFileDependencyEntry.m_source);
                }
                return true;
            });

        // Find all materials that reference any of the material types using this shader
        AZStd::string watchFolder;
        AZ::Data::AssetInfo materialTypeSourceAssetInfo;
        AZStd::list<AzToolsFramework::AssetDatabase::ProductDatabaseEntry> productDependencies;
        for (const auto& materialTypeSource : materialTypeSources)
        {
            bool result = false;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                result, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, materialTypeSource.c_str(),
                materialTypeSourceAssetInfo, watchFolder);
            if (result)
            {
                assetDatabaseConnection.QueryDirectReverseProductDependenciesBySourceGuidSubId(
                    materialTypeSourceAssetInfo.m_assetId.m_guid, materialTypeSourceAssetInfo.m_assetId.m_subId,
                    [&](AzToolsFramework::AssetDatabase::ProductDatabaseEntry& entry)
                    {
                        if (AzFramework::StringFunc::Path::IsExtension(entry.m_productName.c_str(), AZ::RPI::MaterialAsset::Extension))
                        {
                            productDependencies.push_back(entry);
                        }
                        return true;
                    });
            }
        }

        AZStd::vector<AZ::Data::AssetId> results;
        results.reserve(productDependencies.size());
        for (const auto& product : productDependencies)
        {
            assetDatabaseConnection.QueryCombinedByProductID(
                product.m_productID,
                [&](AzToolsFramework::AssetDatabase::CombinedDatabaseEntry& combined)
                {
                    results.push_back({ combined.m_sourceGuid, combined.m_subID });
                    return false;
                },
                nullptr);
        }

        return results;
    }

    AZStd::vector<AZ::RPI::ShaderCollection::Item> ShaderManagementConsoleDocument::GetMaterialInstanceShaderItems(
        const AZ::Data::AssetId& materialAssetId)
    {
        auto materialAsset =
            AZ::RPI::AssetUtils::LoadAssetById<AZ::RPI::MaterialAsset>(materialAssetId, AZ::RPI::AssetUtils::TraceLevel::Error);
        if (!materialAsset.IsReady())
        {
            AZ_Error(
                "ShaderManagementConsoleDocument", false, "Failed to load material asset from asset id: %s",
                materialAssetId.ToFixedString().c_str());
            return AZStd::vector<AZ::RPI::ShaderCollection::Item>();
        }

        auto materialInstance = AZ::RPI::Material::Create(materialAsset);
        if (!materialInstance)
        {
            AZ_Error(
                "ShaderManagementConsoleDocument", false, "Failed to create material instance from asset: %s",
                materialAsset.ToString<AZStd::string>().c_str());
            return AZStd::vector<AZ::RPI::ShaderCollection::Item>();
        }

        return AZStd::vector<AZ::RPI::ShaderCollection::Item>(
            materialInstance->GetShaderCollection().begin(), materialInstance->GetShaderCollection().end());
    }
} // namespace ShaderManagementConsole
