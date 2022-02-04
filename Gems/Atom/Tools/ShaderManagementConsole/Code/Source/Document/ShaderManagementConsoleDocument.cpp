/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <Document/ShaderManagementConsoleDocument.h>

namespace ShaderManagementConsole
{
    ShaderManagementConsoleDocument::ShaderManagementConsoleDocument()
        : AtomToolsFramework::AtomToolsDocument()
    {
        ShaderManagementConsoleDocumentRequestBus::Handler::BusConnect(m_id);
    }

    ShaderManagementConsoleDocument::~ShaderManagementConsoleDocument()
    {
        ShaderManagementConsoleDocumentRequestBus::Handler::BusDisconnect();
    }

    void ShaderManagementConsoleDocument::SetShaderVariantListSourceData(const AZ::RPI::ShaderVariantListSourceData& sourceData)
    {
        m_shaderVariantListSourceData = sourceData;
        AZStd::string shaderPath = m_shaderVariantListSourceData.m_shaderFilePath;
        AzFramework::StringFunc::Path::ReplaceExtension(shaderPath, AZ::RPI::ShaderAsset::Extension);

        m_shaderAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::ShaderAsset>(shaderPath.c_str());
        if (!m_shaderAsset)
        {
            AZ_Error("ShaderManagementConsoleDocument", false, "Could not load shader asset: %s.", shaderPath.c_str());
        }

        AtomToolsFramework::AtomToolsDocumentNotificationBus::Broadcast(
            &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentModified, m_id);
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

        if (!AzFramework::StringFunc::Path::IsExtension(m_absolutePath.c_str(), AZ::RPI::ShaderVariantListSourceData::Extension))
        {
            AZ_Error("ShaderManagementConsoleDocument", false, "Document extension is not supported: '%s.'", m_absolutePath.c_str());
            return OpenFailed();
        }

        // Load the shader config data and create a shader config asset from it
        AZ::RPI::ShaderVariantListSourceData sourceData;
        if (!AZ::RPI::JsonUtils::LoadObjectFromFile(m_absolutePath, sourceData))
        {
            AZ_Error("ShaderManagementConsoleDocument", false, "Failed loading shader variant list data: '%s.'", m_absolutePath.c_str());
            return OpenFailed();
        }

        SetShaderVariantListSourceData(sourceData);
        return IsOpen() ? OpenSucceeded() : OpenFailed();
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
        return true;
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
} // namespace ShaderManagementConsole
