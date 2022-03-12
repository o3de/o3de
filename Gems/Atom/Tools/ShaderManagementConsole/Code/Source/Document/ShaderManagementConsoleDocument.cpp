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
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <Document/ShaderManagementConsoleDocument.h>

namespace ShaderManagementConsole
{
    ShaderManagementConsoleDocument::ShaderManagementConsoleDocument()
        : AtomToolsFramework::AtomToolsDocument()
    {
        ShaderManagementConsoleDocumentRequestBus::Handler::BusConnect(m_id);
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Broadcast(&AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentCreated, m_id);
    }

    ShaderManagementConsoleDocument::~ShaderManagementConsoleDocument()
    {
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Broadcast(&AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentDestroyed, m_id);
        ShaderManagementConsoleDocumentRequestBus::Handler::BusDisconnect();
        Clear();
    }

    size_t ShaderManagementConsoleDocument::GetShaderOptionCount() const
    {
        auto layout = m_shaderAsset->GetShaderOptionGroupLayout();
        auto& shaderOptionDescriptors = layout->GetShaderOptions();
        return shaderOptionDescriptors.size();
    }

    const AZ::RPI::ShaderOptionDescriptor& ShaderManagementConsoleDocument::GetShaderOptionDescriptor(size_t index) const
    {
        auto layout = m_shaderAsset->GetShaderOptionGroupLayout();
        auto& shaderOptionDescriptors = layout->GetShaderOptions();
        return shaderOptionDescriptors[index];
    }

    size_t ShaderManagementConsoleDocument::GetShaderVariantCount() const
    {
        return m_shaderVariantListSourceData.m_shaderVariants.size();
    }

    const AZ::RPI::ShaderVariantListSourceData::VariantInfo& ShaderManagementConsoleDocument::GetShaderVariantInfo(size_t index) const
    {
        return m_shaderVariantListSourceData.m_shaderVariants[index];
    }

    bool ShaderManagementConsoleDocument::Open(AZStd::string_view loadPath)
    {
        Clear();

        m_absolutePath = loadPath;
        if (!AzFramework::StringFunc::Path::Normalize(m_absolutePath))
        {
            AZ_Error("ShaderManagementConsoleDocument", false, "Document path could not be normalized: '%s'.", m_absolutePath.c_str());
            return false;
        }

        if (AzFramework::StringFunc::Path::IsRelative(m_absolutePath.c_str()))
        {
            AZ_Error("ShaderManagementConsoleDocument", false, "Document path must be absolute: '%s'.", m_absolutePath.c_str());
            return false;
        }

        if (AzFramework::StringFunc::Path::IsExtension(m_absolutePath.c_str(), AZ::RPI::ShaderVariantListSourceData::Extension))
        {
            // Load the shader config data and create a shader config asset from it
            if (!AZ::RPI::JsonUtils::LoadObjectFromFile(m_absolutePath, m_shaderVariantListSourceData))
            {
                AZ_Error("ShaderManagementConsoleDocument", false, "Failed loading shader variant list data: '%s.'", m_absolutePath.c_str());
                return false;
            }
        }

        bool result = false;
        AZ::Data::AssetInfo sourceAssetInfo;
        AZStd::string watchFolder;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            result, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, m_absolutePath.c_str(), sourceAssetInfo,
            watchFolder);
        if (!result)
        {
            AZ_Error("ShaderManagementConsoleDocument", false, "Could not find source data: '%s'.", m_absolutePath.c_str());
            return false;
        }

        m_relativePath = m_shaderVariantListSourceData.m_shaderFilePath;
        if (!AzFramework::StringFunc::Path::Normalize(m_relativePath))
        {
            AZ_Error("ShaderManagementConsoleDocument", false, "Shader path could not be normalized: '%s'.", m_relativePath.c_str());
            return false;
        }

        AZStd::string shaderPath = m_relativePath;
        AzFramework::StringFunc::Path::ReplaceExtension(shaderPath, AZ::RPI::ShaderAsset::Extension);

        m_shaderAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::ShaderAsset>(shaderPath.c_str());
        if (!m_shaderAsset)
        {
            AZ_Error("ShaderManagementConsoleDocument", false, "Could not load shader asset: %s.", shaderPath.c_str());
            return false;
        }

        AtomToolsFramework::AtomToolsDocumentNotificationBus::Broadcast(&AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentOpened, m_id);

        AZ_TracePrintf("ShaderManagementConsoleDocument", "Document opened: '%s'\n", m_absolutePath.c_str());
        return true;
    }

    bool ShaderManagementConsoleDocument::Close()
    {
        if (!IsOpen())
        {
            AZ_Error("ShaderManagementConsoleDocument", false, "Document is not open");
            return false;
        }

        Clear();
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Broadcast(&AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentClosed, m_id);
        AZ_TracePrintf("ShaderManagementConsoleDocument", "Document closed\n");
        return true;
    }

    bool ShaderManagementConsoleDocument::IsOpen() const
    {
        return !m_absolutePath.empty() && !m_relativePath.empty();
    }

    void ShaderManagementConsoleDocument::Clear()
    {
        m_absolutePath.clear();
        m_relativePath.clear();
        m_shaderVariantListSourceData = {};
        m_shaderAsset = {};
    }
} // namespace ShaderManagementConsole
