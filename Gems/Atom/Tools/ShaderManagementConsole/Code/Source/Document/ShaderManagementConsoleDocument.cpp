/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

#include <Document/ShaderManagementConsoleDocument.h>
#include <Atom/Document/ShaderManagementConsoleDocumentNotificationBus.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

namespace ShaderManagementConsole
{
    ShaderManagementConsoleDocument::ShaderManagementConsoleDocument()
    {
        ShaderManagementConsoleDocumentRequestBus::Handler::BusConnect(m_id);
        ShaderManagementConsoleDocumentNotificationBus::Broadcast(&ShaderManagementConsoleDocumentNotificationBus::Events::OnDocumentCreated, m_id);
    }

    ShaderManagementConsoleDocument::~ShaderManagementConsoleDocument()
    {
        ShaderManagementConsoleDocumentNotificationBus::Broadcast(&ShaderManagementConsoleDocumentNotificationBus::Events::OnDocumentDestroyed, m_id);
        ShaderManagementConsoleDocumentRequestBus::Handler::BusDisconnect();
    }

    const AZ::Uuid& ShaderManagementConsoleDocument::GetId() const
    {
        return m_id;
    }

    AZStd::string_view ShaderManagementConsoleDocument::GetAbsolutePath() const
    {
        return m_absolutePath;
    }

    AZStd::string_view ShaderManagementConsoleDocument::GetRelativePath() const
    {
        return m_relativePath;
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

    ShaderManagementConsoleDocumentResult ShaderManagementConsoleDocument::Open(AZStd::string_view loadPath)
    {
        Clear();

        m_absolutePath = loadPath;
        if (!AzFramework::StringFunc::Path::Normalize(m_absolutePath))
        {
            return AZ::Failure(AZStd::string::format("Document path could not be normalized: '%s'.", m_absolutePath.c_str()));
        }

        if (AzFramework::StringFunc::Path::IsRelative(m_absolutePath.c_str()))
        {
            return AZ::Failure(AZStd::string::format("Document path must be absolute: '%s'.", m_absolutePath.c_str()));
        }

        if (AzFramework::StringFunc::Path::IsExtension(m_absolutePath.c_str(), AZ::RPI::ShaderVariantListSourceData::Extension))
        {
            // Load the shader config data and create a shader config asset from it
            if (!AZ::RPI::JsonUtils::LoadObjectFromFile(m_absolutePath, m_shaderVariantListSourceData))
            {
                return AZ::Failure(AZStd::string::format("Failed loading shader variant list data: '%s.'", m_absolutePath.c_str()));
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
            return AZ::Failure(AZStd::string::format("Could not find source data: '%s'.", m_absolutePath.c_str()));
        }

        m_relativePath = m_shaderVariantListSourceData.m_shaderFilePath;
        if (!AzFramework::StringFunc::Path::Normalize(m_relativePath))
        {
            return AZ::Failure(AZStd::string::format("Shader path could not be normalized: '%s'.", m_relativePath.c_str()));
        }

        AZStd::string shaderPath = m_relativePath;
        AzFramework::StringFunc::Path::ReplaceExtension(shaderPath, AZ::RPI::ShaderAsset::Extension);

        m_shaderAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::ShaderAsset>(shaderPath.c_str());
        if (!m_shaderAsset)
        {
            return AZ::Failure(AZStd::string::format("Could not load shader asset: %s.", shaderPath.c_str()));
        }

        ShaderManagementConsoleDocumentNotificationBus::Broadcast(&ShaderManagementConsoleDocumentNotificationBus::Events::OnDocumentOpened, m_id);

        return AZ::Success(AZStd::string::format("Document loaded: '%s'", m_absolutePath.c_str()));
    }

    ShaderManagementConsoleDocumentResult ShaderManagementConsoleDocument::Save()
    {
        if (!IsOpen())
        {
            return AZ::Failure(AZStd::string::format("Document is not open to be saved: '%s'.", m_absolutePath.c_str()));
        }

        if (!IsSavable())
        {
            return AZ::Failure(AZStd::string::format("Document can not be saved: '%s'.", m_absolutePath.c_str()));
        }

        return AZ::Failure(AZStd::string::format("%s is not implemented!", __FUNCTION__));

        // Auto add or checkout saved file
        //AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::RequestEdit,
        //    m_absolutePath.c_str(), true,
        //    [](bool, const AzToolsFramework::SourceControlFileInfo&) {});

        //ShaderManagementConsoleDocumentNotificationBus::Broadcast(&ShaderManagementConsoleDocumentNotificationBus::Events::OnDocumentSaved, m_id);

        //return AZ::Success(AZStd::string::format("Document saved: %s", m_absolutePath.data()));
    }

    ShaderManagementConsoleDocumentResult ShaderManagementConsoleDocument::SaveAsCopy(AZStd::string_view savePath)
    {
        if (!IsOpen())
        {
            return AZ::Failure(AZStd::string::format("Document is not open to be saved: '%s'.", m_absolutePath.c_str()));
        }

        if (!IsSavable())
        {
            return AZ::Failure(AZStd::string::format("Document can not be saved: '%s'.", m_absolutePath.c_str()));
        }

        AZStd::string normalizedSavePath = savePath;
        if (!AzFramework::StringFunc::Path::Normalize(normalizedSavePath))
        {
            return AZ::Failure(AZStd::string::format("Document save path could not be normalized: '%s'.", normalizedSavePath.c_str()));
        }

        return AZ::Failure(AZStd::string::format("%s is not implemented!", __FUNCTION__));

        // Auto add or checkout saved file
        //AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::RequestEdit,
        //    normalizedSavePath.c_str(), true,
        //    [](bool, const AzToolsFramework::SourceControlFileInfo&) {});

        //ShaderManagementConsoleDocumentNotificationBus::Broadcast(&ShaderManagementConsoleDocumentNotificationBus::Events::OnDocumentSaved, m_id);

        //return AZ::Success(AZStd::string::format("Document saved: %s", normalizedSavePath.c_str()));
    }

    ShaderManagementConsoleDocumentResult ShaderManagementConsoleDocument::Close()
    {
        if (!IsOpen())
        {
            return AZ::Failure(AZStd::string("Document is not open"));
        }

        Clear();
        ShaderManagementConsoleDocumentNotificationBus::Broadcast(&ShaderManagementConsoleDocumentNotificationBus::Events::OnDocumentClosed, m_id);
        return AZ::Success(AZStd::string("Document was closed"));
    }

    bool ShaderManagementConsoleDocument::IsOpen() const
    {
        return !m_absolutePath.empty() && !m_relativePath.empty();
    }

    bool ShaderManagementConsoleDocument::IsModified() const
    {
        return false;
    }

    bool ShaderManagementConsoleDocument::IsSavable() const
    {
        return true;
    }

    bool ShaderManagementConsoleDocument::CanUndo() const
    {
        // Undo will only be allowed if something has been recorded and we're not at the beginning of history
        return IsOpen() && !m_undoHistory.empty() && m_undoHistoryIndex > 0;
    }

    bool ShaderManagementConsoleDocument::CanRedo() const
    {
        // Redo will only be allowed if something has been recorded and we're not at the end of history
        return IsOpen() && !m_undoHistory.empty() && m_undoHistoryIndex < m_undoHistory.size();
    }

    bool ShaderManagementConsoleDocument::Undo()
    {
        if (CanUndo())
        {
            // The history index is one beyond the last executed command. Decrement the index then execute undo.
            m_undoHistory[--m_undoHistoryIndex].first();
            ShaderManagementConsoleDocumentNotificationBus::Broadcast(&ShaderManagementConsoleDocumentNotificationBus::Events::OnDocumentUndoStateChanged, m_id);
            return true;
        }
        return false;
    }

    bool ShaderManagementConsoleDocument::Redo()
    {
        if (CanRedo())
        {
            // Execute the current redo command then move the history index to the next position.
            m_undoHistory[m_undoHistoryIndex++].second();
            ShaderManagementConsoleDocumentNotificationBus::Broadcast(&ShaderManagementConsoleDocumentNotificationBus::Events::OnDocumentUndoStateChanged, m_id);
            return true;
        }
        return false;
    }

    bool ShaderManagementConsoleDocument::BeginEdit()
    {
        return true;
    }

    bool ShaderManagementConsoleDocument::EndEdit()
    {
        // Wipe any state beyond the current history index
        m_undoHistory.erase(m_undoHistory.begin() + m_undoHistoryIndex, m_undoHistory.end());

        // Add undo and redo operations using lambdas that will capture property state and restore it when executed
        m_undoHistory.emplace_back(
            [this]() { /**/ },
            [this]() { /**/ });

        // Assign the index to the end of history
        m_undoHistoryIndex = aznumeric_cast<int>(m_undoHistory.size());
        ShaderManagementConsoleDocumentNotificationBus::Broadcast(&ShaderManagementConsoleDocumentNotificationBus::Events::OnDocumentUndoStateChanged, m_id);

        return true;
    }

    void ShaderManagementConsoleDocument::Clear()
    {
        m_absolutePath.clear();
        m_relativePath.clear();
    }
} // namespace ShaderManagementConsole
