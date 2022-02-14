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
#include <AzCore/Utils/Utils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <Document/ShaderManagementConsoleDocument.h>

namespace ShaderManagementConsole
{
    ShaderManagementConsoleDocument::ShaderManagementConsoleDocument(const AZ::Crc32& toolId)
        : AtomToolsFramework::AtomToolsDocument(toolId)
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
            m_toolId, &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentModified, m_id);
    }

    const AZ::RPI::ShaderVariantListSourceData& ShaderManagementConsoleDocument::GetShaderVariantListSourceData() const
    {
        return m_shaderVariantListSourceData;
    }

    size_t ShaderManagementConsoleDocument::GetShaderVariantCount() const
    {
        return m_shaderVariantListSourceData.m_shaderVariants.size();
    }

    const AZ::RPI::ShaderVariantListSourceData::VariantInfo& ShaderManagementConsoleDocument::GetShaderVariantInfo(size_t index) const
    {
        return m_shaderVariantListSourceData.m_shaderVariants[index];
    }

    size_t ShaderManagementConsoleDocument::GetShaderOptionCount() const
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

    AZStd::vector<AtomToolsFramework::DocumentObjectInfo> ShaderManagementConsoleDocument::GetObjectInfo() const
    {
        if (!IsOpen())
        {
            AZ_Error("ShaderManagementConsoleDocument", false, "Document is not open.");
            return {};
        }

        AZStd::vector<AtomToolsFramework::DocumentObjectInfo> objects;

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

    bool ShaderManagementConsoleDocument::Open(AZStd::string_view loadPath)
    {
        if (!AtomToolsDocument::Open(loadPath))
        {
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

    bool ShaderManagementConsoleDocument::SaveAsCopy(AZStd::string_view savePath)
    {
        if (!AtomToolsDocument::SaveAsCopy(savePath))
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

        return SaveSourceData();
    }

    bool ShaderManagementConsoleDocument::SaveAsChild(AZStd::string_view savePath)
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
        return false;
    }

    bool ShaderManagementConsoleDocument::IsSavable() const
    {
        return IsOpen() &&
            AzFramework::StringFunc::Path::IsExtension(m_absolutePath.c_str(), AZ::RPI::ShaderVariantListSourceData::Extension);
    }

    void ShaderManagementConsoleDocument::Clear()
    {
        AtomToolsFramework::AtomToolsDocument::Clear();

        m_shaderVariantListSourceData = {};
        m_shaderAsset = {};
    }

    bool ShaderManagementConsoleDocument::SaveSourceData()
    {
        if (!AZ::RPI::JsonUtils::SaveObjectToFile(m_savePathNormalized, m_shaderVariantListSourceData))
        {
            AZ_Error("ShaderManagementConsoleDocument", false, "Document could not be saved: '%s'.", m_savePathNormalized.c_str());
            return SaveFailed();
        }

        m_absolutePath = m_savePathNormalized;
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

        SetShaderVariantListSourceData(shaderVariantListSourceData);
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
        return IsOpen() ? OpenSucceeded() : OpenFailed();
    }

    AZStd::vector<AZ::Data::AssetId> ShaderManagementConsoleDocument::FindMaterialAssetsUsingShader(const AZStd::string& shaderFilePath)
    {
        AzToolsFramework::AssetDatabase::AssetDatabaseConnection assetDatabaseConnection;
        assetDatabaseConnection.OpenDatabase();

        // Find all material types that reference shaderFilePath
        AZStd::vector<AZStd::string> materialTypeSources;

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
        AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer productDependencies;
        for (const auto& materialTypeSource : materialTypeSources)
        {
            bool result = false;
            AZ::Data::AssetInfo materialTypeSourceAssetInfo;
            AZStd::string watchFolder;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                result, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, materialTypeSource.c_str(),
                materialTypeSourceAssetInfo, watchFolder);

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

        AZStd::vector<AZ::Data::AssetId> results;
        results.reserve(productDependencies.size());
        for (auto product : productDependencies)
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
                materialAssetId.ToString<AZStd::string>().c_str());
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
