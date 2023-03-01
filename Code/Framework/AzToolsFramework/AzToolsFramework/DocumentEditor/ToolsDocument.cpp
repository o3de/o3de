/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/DocumentEditor/ToolsDocument.h>
#include <AzToolsFramework/DocumentEditor/ToolsDocumentNotificationBus.h>
#include <AzToolsFramework/API/Utils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

namespace AzToolsFramework
{
    void ToolsDocument::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ToolsDocument>()
                ->Version(0);
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<ToolsDocumentRequestBus>("ToolsDocumentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "tools")
                ->Event("GetAbsolutePath", &ToolsDocumentRequestBus::Events::GetAbsolutePath)
                ->Event("Open", &ToolsDocumentRequestBus::Events::Open)
                ->Event("Reopen", &ToolsDocumentRequestBus::Events::Reopen)
                ->Event("Close", &ToolsDocumentRequestBus::Events::Close)
                ->Event("Save", &ToolsDocumentRequestBus::Events::Save)
                ->Event("SaveAsChild", &ToolsDocumentRequestBus::Events::SaveAsChild)
                ->Event("SaveAsCopy", &ToolsDocumentRequestBus::Events::SaveAsCopy)
                ->Event("IsOpen", &ToolsDocumentRequestBus::Events::IsOpen)
                ->Event("IsModified", &ToolsDocumentRequestBus::Events::IsModified)
                ->Event("CanSaveAsChild", &ToolsDocumentRequestBus::Events::CanSaveAsChild)
                ->Event("CanUndo", &ToolsDocumentRequestBus::Events::CanUndo)
                ->Event("CanRedo", &ToolsDocumentRequestBus::Events::CanRedo)
                ->Event("Undo", &ToolsDocumentRequestBus::Events::Undo)
                ->Event("Redo", &ToolsDocumentRequestBus::Events::Redo)
                ->Event("BeginEdit", &ToolsDocumentRequestBus::Events::BeginEdit)
                ->Event("EndEdit", &ToolsDocumentRequestBus::Events::EndEdit)
                ;
        }
    }

    ToolsDocument::ToolsDocument(const AZ::Crc32& toolId, const DocumentTypeInfo& documentTypeInfo)
        : m_toolId(toolId)
        , m_documentTypeInfo(documentTypeInfo)
    {
        ToolsDocumentRequestBus::Handler::BusConnect(m_id);
        ToolsDocumentNotificationBus::Event(m_toolId, &ToolsDocumentNotificationBus::Events::OnDocumentCreated, m_id);
    }

    ToolsDocument::~ToolsDocument()
    {
        ToolsDocumentNotificationBus::Event(m_toolId, &ToolsDocumentNotificationBus::Events::OnDocumentDestroyed, m_id);
        ToolsDocumentRequestBus::Handler::BusDisconnect();
        AzToolsFramework::AssetSystemBus::Handler::BusDisconnect();
    }

    const DocumentTypeInfo& ToolsDocument::GetDocumentTypeInfo() const
    {
        return m_documentTypeInfo;
    }

    DocumentObjectInfoVector ToolsDocument::GetObjectInfo() const
    {
        return DocumentObjectInfoVector();
    }

    const AZ::Uuid& ToolsDocument::GetId() const
    {
        return m_id;
    }

    const AZStd::string& ToolsDocument::GetAbsolutePath() const
    {
        return m_absolutePath;
    }

    bool ToolsDocument::Open(const AZStd::string& loadPath)
    {
        Clear();

        m_absolutePath = loadPath;
        if (!ValidateDocumentPath(m_absolutePath))
        {
            AZ_Error("ToolsDocument", false, "Document path is invalid, not in a supported project or gem folder, or marked as non-editable: '%s'.", m_absolutePath.c_str());
            return OpenFailed();
        }

        if (!GetDocumentTypeInfo().IsSupportedExtensionToOpen(m_absolutePath) &&
            !GetDocumentTypeInfo().IsSupportedExtensionToCreate(m_absolutePath))
        {
            AZ_Error("ToolsDocument", false, "Document path extension is not supported: '%s'.", m_absolutePath.c_str());
            return OpenFailed();
        }

        return true;
    }

    bool ToolsDocument::Reopen()
    {
        if (!ReopenRecordState())
        {
            return false;
        }

        const auto loadPath = m_absolutePath;
        if (!Open(loadPath))
        {
            return false;
        }

        if (!ReopenRestoreState())
        {
            return false;
        }

        ToolsDocumentNotificationBus::Event(m_toolId, &ToolsDocumentNotificationBus::Events::OnDocumentModified, m_id);
        ToolsDocumentNotificationBus::Event(m_toolId, &ToolsDocumentNotificationBus::Events::OnDocumentUndoStateChanged, m_id);
        return true;
    }

    bool ToolsDocument::Save()
    {
        if (!GetDocumentTypeInfo().IsSupportedExtensionToSave(m_absolutePath))
        {
            AZ_Error("ToolsDocument", false, "Document type can not be saved: '%s'.", m_absolutePath.c_str());
            return SaveFailed();
        }

        m_savePathNormalized = m_absolutePath;
        if (!ValidateDocumentPath(m_savePathNormalized))
        {
            AZ_Error("ToolsDocument", false, "Document path is invalid, not in a supported project or gem folder, or marked as non-editable: '%s'.", m_savePathNormalized.c_str());
            return SaveFailed();
        }

        if (!GetDocumentTypeInfo().IsSupportedExtensionToSave(m_savePathNormalized))
        {
            AZ_Error("ToolsDocument", false, "Document save path extension is not supported: '%s'.", m_savePathNormalized.c_str());
            return SaveFailed();
        }

        return true;
    }

    bool ToolsDocument::SaveAsCopy(const AZStd::string& savePath)
    {
        if (!GetDocumentTypeInfo().IsSupportedExtensionToSave(savePath))
        {
            AZ_Error("ToolsDocument", false, "Document type can not be saved: '%s'.", m_absolutePath.c_str());
            return SaveFailed();
        }

        m_savePathNormalized = savePath;
        if (!ValidateDocumentPath(m_savePathNormalized))
        {
            AZ_Error("ToolsDocument", false, "Document path is invalid, not in a supported project or gem folder, or marked as non-editable: '%s'.", m_savePathNormalized.c_str());
            return SaveFailed();
        }

        if (!GetDocumentTypeInfo().IsSupportedExtensionToSave(m_savePathNormalized))
        {
            AZ_Error("ToolsDocument", false, "Document save path extension is not supported: '%s'.", m_savePathNormalized.c_str());
            return SaveFailed();
        }

        return true;
    }

    bool ToolsDocument::SaveAsChild(const AZStd::string& savePath)
    {
        if (!GetDocumentTypeInfo().IsSupportedExtensionToSave(savePath))
        {
            AZ_Error("AtomToolsDocument", false, "Document type can not be saved: '%s'.", m_absolutePath.c_str());
            return SaveFailed();
        }

        m_savePathNormalized = savePath;
        if (!ValidateDocumentPath(m_savePathNormalized))
        {
            AZ_Error("ToolsDocument", false, "Document path is invalid, not in a supported project or gem folder, or marked as non-editable: '%s'.", m_savePathNormalized.c_str());
            return SaveFailed();
        }

        if (!GetDocumentTypeInfo().IsSupportedExtensionToSave(m_savePathNormalized))
        {
            AZ_Error("ToolsDocument", false, "Document save path extension is not supported: '%s'.", m_savePathNormalized.c_str());
            return SaveFailed();
        }

        if (m_absolutePath == m_savePathNormalized || m_sourceDependencies.find(m_savePathNormalized) != m_sourceDependencies.end())
        {
            AZ_Error("ToolsDocument", false, "Document can not be saved over a dependancy: '%s'.", m_savePathNormalized.c_str());
            return SaveFailed();
        }

        return true;
    }

    bool ToolsDocument::Close()
    {
        AZ_TracePrintf("ToolsDocument", "Document closed: '%s'.\n", m_absolutePath.c_str());
        ToolsDocumentNotificationBus::Event(m_toolId, &ToolsDocumentNotificationBus::Events::OnDocumentClosed, m_id);

        // Clearing after notification so paths are still available
        Clear();
        return true;
    }

    void ToolsDocument::Clear()
    {
        AzToolsFramework::AssetSystemBus::Handler::BusDisconnect();

        m_absolutePath.clear();
        m_sourceDependencies.clear();
        m_ignoreSourceFileChangeToSelf = {};
        m_undoHistory.clear();
        m_undoHistoryIndex = {};

        ToolsDocumentNotificationBus::Event(m_toolId, &ToolsDocumentNotificationBus::Events::OnDocumentCleared, m_id);
    }

    bool ToolsDocument::IsOpen() const
    {
        return !m_id.IsNull();
    }

    bool ToolsDocument::IsModified() const
    {
        return false;
    }

    bool ToolsDocument::CanSaveAsChild() const
    {
        return GetDocumentTypeInfo().IsSupportedExtensionToSave(m_absolutePath);
    }

    bool ToolsDocument::CanUndo() const
    {
        // Undo will only be allowed if something has been recorded and we're not at the beginning of history
        return !m_undoHistory.empty() && m_undoHistoryIndex > 0;
    }

    bool ToolsDocument::CanRedo() const
    {
        // Redo will only be allowed if something has been recorded and we're not at the end of history
        return !m_undoHistory.empty() && m_undoHistoryIndex < m_undoHistory.size();
    }

    bool ToolsDocument::Undo()
    {
        if (CanUndo())
        {
            // The history index is one beyond the last executed command. Decrement the index then execute undo.
            m_undoHistory[--m_undoHistoryIndex].first();
            AZ_TracePrintf("ToolsDocument", "Document undo: '%s'.\n", m_absolutePath.c_str());
            ToolsDocumentNotificationBus::Event(m_toolId, &ToolsDocumentNotificationBus::Events::OnDocumentUndoStateChanged, m_id);
            return true;
        }
        return false;
    }

    bool ToolsDocument::Redo()
    {
        if (CanRedo())
        {
            // Execute the current redo command then move the history index to the next position.
            m_undoHistory[m_undoHistoryIndex++].second();
            AZ_TracePrintf("ToolsDocument", "Document redo: '%s'.\n", m_absolutePath.c_str());
            ToolsDocumentNotificationBus::Event(m_toolId, &ToolsDocumentNotificationBus::Events::OnDocumentUndoStateChanged, m_id);
            return true;
        }
        return false;
    }

    bool ToolsDocument::BeginEdit()
    {
        AZ_Warning("ToolsDocument", false, "%s not implemented.", __FUNCTION__);
        return false;
    }

    bool ToolsDocument::EndEdit()
    {
        AZ_Warning("ToolsDocument", false, "%s not implemented.", __FUNCTION__);
        return false;
    }

    bool ToolsDocument::OpenSucceeded()
    {
        AZ_TracePrintf("ToolsDocument", "Document opened: '%s'.\n", m_absolutePath.c_str());
        AzToolsFramework::AssetSystemBus::Handler::BusConnect();
        ToolsDocumentNotificationBus::Event(m_toolId, &ToolsDocumentNotificationBus::Events::OnDocumentOpened, m_id);
        return true;
    }

    bool ToolsDocument::OpenFailed()
    {
        AZ_TracePrintf("ToolsDocument", "Document could not opened: '%s'.\n", m_absolutePath.c_str());
        ToolsDocumentNotificationBus::Event(m_toolId, &ToolsDocumentNotificationBus::Events::OnDocumentError, m_id);
        Clear();
        return false;
    }

    bool ToolsDocument::SaveSucceeded()
    {
        m_ignoreSourceFileChangeToSelf = true;

        AZ_TracePrintf("ToolsDocument", "Document saved: '%s'.\n", m_savePathNormalized.c_str());

        // Auto add or checkout saved file
        AzToolsFramework::SourceControlCommandBus::Broadcast(
            &AzToolsFramework::SourceControlCommandBus::Events::RequestEdit, m_savePathNormalized.c_str(), true,
            [](bool, const AzToolsFramework::SourceControlFileInfo&) {});

        ToolsDocumentNotificationBus::Event(m_toolId, &ToolsDocumentNotificationBus::Events::OnDocumentSaved, m_id);
        return true;
    }

    bool ToolsDocument::SaveFailed()
    {
        AZ_TracePrintf("ToolsDocument", "Document not saved: '%s'.\n", m_savePathNormalized.c_str());
        ToolsDocumentNotificationBus::Event(m_toolId, &ToolsDocumentNotificationBus::Events::OnDocumentError, m_id);
        return false;
    }

    bool ToolsDocument::ReopenRecordState()
    {
        m_undoHistoryBeforeReopen = m_undoHistory;
        m_undoHistoryIndexBeforeReopen = m_undoHistoryIndex;
        return true;
    }

    bool ToolsDocument::ReopenRestoreState()
    {
        m_undoHistory = m_undoHistoryBeforeReopen;
        m_undoHistoryIndex = m_undoHistoryIndexBeforeReopen;
        m_undoHistoryBeforeReopen = {};
        m_undoHistoryIndexBeforeReopen = {};
        return true;
    }

    void ToolsDocument::AddUndoRedoHistory(const UndoRedoFunction& undoCommand, const UndoRedoFunction& redoCommand)
    {
        // Wipe any state beyond the current history index
        m_undoHistory.erase(m_undoHistory.begin() + m_undoHistoryIndex, m_undoHistory.end());

        // Add undo and redo operations using functions that capture state and restore it when executed
        m_undoHistory.emplace_back(undoCommand, redoCommand);

        // Assign the index to the end of history
        m_undoHistoryIndex = aznumeric_cast<int>(m_undoHistory.size());
        ToolsDocumentNotificationBus::Event(m_toolId, &ToolsDocumentNotificationBus::Events::OnDocumentUndoStateChanged, m_id);
    }

    void ToolsDocument::SourceFileChanged(AZStd::string relativePath, AZStd::string scanFolder, [[maybe_unused]] AZ::Uuid sourceUUID)
    {
#if DISABLED_FOR_NOW
        const auto sourcePath = AZ::RPI::AssetUtils::ResolvePathReference(scanFolder, relativePath);

        if (m_absolutePath == sourcePath)
        {
            // ignore notifications caused by saving the open document
            if (!m_ignoreSourceFileChangeToSelf)
            {
                AZ_TracePrintf("ToolsDocument", "Document changed externally: '%s'.\n", m_absolutePath.c_str());
                ToolsDocumentNotificationBus::Event(
                    m_toolId, &ToolsDocumentNotificationBus::Events::OnDocumentExternallyModified, m_id);
            }
            m_ignoreSourceFileChangeToSelf = false;
        }
        else if (m_sourceDependencies.find(sourcePath) != m_sourceDependencies.end())
        {
            AZ_TracePrintf("ToolsDocument", "Document dependency changed: '%s'.\n", m_absolutePath.c_str());
            ToolsDocumentNotificationBus::Event(
                m_toolId, &ToolsDocumentNotificationBus::Events::OnDocumentDependencyModified, m_id);
        }
#endif
    }

} // namespace AzToolsFramework
