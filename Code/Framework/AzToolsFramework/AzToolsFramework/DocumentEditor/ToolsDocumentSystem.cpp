/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/DocumentEditor/TraceRecorder.h>
#include <AzToolsFramework/DocumentEditor/ToolsDocumentNotificationBus.h>
#include <AzToolsFramework/DocumentEditor/ToolsDocumentRequestBus.h>
#include <AzToolsFramework/DocumentEditor/ToolsDocumentSystem.h>
#include <AzToolsFramework/DocumentEditor/ToolsDocumentSystemRequestBus.h>
#include <AzToolsFramework/API/Utils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QMessageBox>
#include <QString>
#include <QTimer>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    void DisplayErrorMessage(QWidget* parent, const QString& title, const QString& text)
    {
        AZ_Error("ToolsDocumentSystem", false, "%s: %s", title.toUtf8().constData(), text.toUtf8().constData());
        if (GetSettingsValue<bool>("/O3DE/AzToolsFramework/ToolsDocumentSystem/DisplayErrorMessageDialogs", true))
        {
            QMessageBox::critical(parent, title, QObject::tr("%1\nThese messages can be disabled from settings.").arg(text));
        }
    }

    void DisplayWarningMessage(QWidget* parent, const QString& title, const QString& text)
    {
        AZ_Warning("ToolsDocumentSystem", false, "%s: %s", title.toUtf8().constData(), text.toUtf8().constData());
        if (GetSettingsValue<bool>("/O3DE/AzToolsFramework/ToolsDocumentSystem/DisplayWarningMessageDialogs", true))
        {
            QMessageBox::warning(parent, title, QObject::tr("%1\nThese messages can be disabled from settings.").arg(text));
        }
    }

    void ToolsDocumentSystem::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ToolsDocumentSystem>()
                ->Version(0);

            if (auto editContext = serialize->GetEditContext())
            {
                editContext->Class<ToolsDocumentSystem>("ToolsDocumentSystem", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<ToolsDocumentSystemRequestBus>("ToolsDocumentSystemRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "tools")
                ->Event("CreateDocumentFromTypeName", &ToolsDocumentSystemRequestBus::Events::CreateDocumentFromTypeName)
                ->Event("CreateDocumentFromFileType", &ToolsDocumentSystemRequestBus::Events::CreateDocumentFromFileType)
                ->Event("CreateDocumentFromFilePath", &ToolsDocumentSystemRequestBus::Events::CreateDocumentFromFilePath)
                ->Event("DestroyDocument", &ToolsDocumentSystemRequestBus::Events::DestroyDocument)
                ->Event("OpenDocument", &ToolsDocumentSystemRequestBus::Events::OpenDocument)
                ->Event("CloseDocument", &ToolsDocumentSystemRequestBus::Events::CloseDocument)
                ->Event("CloseAllDocuments", &ToolsDocumentSystemRequestBus::Events::CloseAllDocuments)
                ->Event("CloseAllDocumentsExcept", &ToolsDocumentSystemRequestBus::Events::CloseAllDocumentsExcept)
                ->Event("SaveDocument", &ToolsDocumentSystemRequestBus::Events::SaveDocument)
                ->Event("SaveDocumentAsCopy", &ToolsDocumentSystemRequestBus::Events::SaveDocumentAsCopy)
                ->Event("SaveDocumentAsChild", &ToolsDocumentSystemRequestBus::Events::SaveDocumentAsChild)
                ->Event("SaveAllDocuments", &ToolsDocumentSystemRequestBus::Events::SaveAllDocuments)
                ->Event("SaveAllModifiedDocuments", &ToolsDocumentSystemRequestBus::Events::SaveAllModifiedDocuments)
                ->Event("QueueReopenModifiedDocuments", &ToolsDocumentSystemRequestBus::Events::QueueReopenModifiedDocuments)
                ->Event("ReopenModifiedDocuments", &ToolsDocumentSystemRequestBus::Events::ReopenModifiedDocuments)
                ->Event("GetDocumentCount", &ToolsDocumentSystemRequestBus::Events::GetDocumentCount)
                ->Event("IsDocumentOpen", &ToolsDocumentSystemRequestBus::Events::IsDocumentOpen)
                ->Event("AddRecentFilePath", &ToolsDocumentSystemRequestBus::Events::AddRecentFilePath)
                ->Event("ClearRecentFilePaths", &ToolsDocumentSystemRequestBus::Events::ClearRecentFilePaths)
                ->Event("SetRecentFilePaths", &ToolsDocumentSystemRequestBus::Events::SetRecentFilePaths)
                ->Event("GetRecentFilePaths", &ToolsDocumentSystemRequestBus::Events::GetRecentFilePaths)
                ;
        }
    }

    ToolsDocumentSystem::ToolsDocumentSystem(const AZ::Crc32& toolId)
        : m_toolId(toolId)
    {
        ToolsDocumentSystemRequestBus::Handler::BusConnect(m_toolId);
        ToolsDocumentNotificationBus::Handler::BusConnect(m_toolId);
    }

    ToolsDocumentSystem::~ToolsDocumentSystem()
    {
        m_documentMap.clear();
        ToolsDocumentNotificationBus::Handler::BusDisconnect();
        ToolsDocumentSystemRequestBus::Handler::BusDisconnect();
    }

    void ToolsDocumentSystem::RegisterDocumentType(const DocumentTypeInfo& documentType)
    {
        m_documentTypes.push_back(documentType);
    }

    const DocumentTypeInfoVector& ToolsDocumentSystem::GetRegisteredDocumentTypes() const
    {
        return m_documentTypes;
    }

    AZ::Uuid ToolsDocumentSystem::CreateDocumentFromType(const DocumentTypeInfo& documentType)
    {
        AZStd::unique_ptr<ToolsDocumentRequests> document(documentType.CreateDocument(m_toolId));
        if (!document)
        {
            DisplayErrorMessage(
                GetToolMainWindow(),
                QObject::tr("Document could not be created"),
                QObject::tr("Could not create document using type: %1").arg(documentType.m_documentTypeName.c_str()));
            return AZ::Uuid::CreateNull();
        }

        const AZ::Uuid documentId = document->GetId();
        m_documentMap.emplace(documentId, document.release());
        documentType.CreateDocumentView(m_toolId, documentId);
        return documentId;
    }

    AZ::Uuid ToolsDocumentSystem::CreateDocumentFromTypeName(const AZStd::string& documentTypeName)
    {
        for (const auto& documentType : m_documentTypes)
        {
            if (AZ::StringFunc::Equal(documentType.m_documentTypeName, documentTypeName))
            {
                return CreateDocumentFromType(documentType);
            }
        }
        return AZ::Uuid();
    }

    AZ::Uuid ToolsDocumentSystem::CreateDocumentFromFileType(const AZStd::string& path)
    {
        for (const auto& documentType : m_documentTypes)
        {
            if (documentType.IsSupportedExtensionToCreate(path) || documentType.IsSupportedExtensionToOpen(path))
            {
                return CreateDocumentFromType(documentType);
            }
        }
        return AZ::Uuid();
    }

    AZ::Uuid ToolsDocumentSystem::CreateDocumentFromFilePath(const AZStd::string& sourcePath, const AZStd::string& targetPath)
    {
        // This function attempts to create a new document in a couple of different ways.
        // If a source path is specified then it will attempt to create a document using the source path extension and automatically open
        // the source file. This mechanism supports creating new documents from pre-existing templates or using the source document as a
        // parent. If no source file is specified, an attempt will be made to create the document using the target path extension to
        // determine the document type. If a target path is specified then the new document will also automatically be saved to that
        // location.
        TraceRecorder traceRecorder(m_maxMessageBoxLineCount);

        AZStd::string openPath = sourcePath;
        if (!openPath.empty() && !ValidateDocumentPath(openPath))
        {
            DisplayErrorMessage(
                GetToolMainWindow(),
                QObject::tr("Document could not be created"),
                QObject::tr("Document path is invalid, not in a supported project or gem folder, or marked as non-editable:\n%1").arg(openPath.c_str()));
            return AZ::Uuid::CreateNull();
        }

        AZStd::string savePath = targetPath;
        if (!savePath.empty() && !ValidateDocumentPath(savePath))
        {
            DisplayErrorMessage(
                GetToolMainWindow(),
                QObject::tr("Document could not be created"),
                QObject::tr("Document path is invalid, not in a supported project or gem folder, or marked as non-editable:\n%1").arg(savePath.c_str()));
            return AZ::Uuid::CreateNull();
        }

        const AZStd::string& createPath = !openPath.empty() ? openPath : savePath;
        const AZ::Uuid documentId = CreateDocumentFromFileType(createPath);
        if (documentId.IsNull())
        {
            DisplayErrorMessage(
                GetToolMainWindow(),
                QObject::tr("Document could not be created"),
                QObject::tr("Failed to create document from file type: \n%1\n\n%2").arg(createPath.c_str()).arg(traceRecorder.GetDump().c_str()));
            return AZ::Uuid::CreateNull();
        }

        if (!openPath.empty())
        {
            bool openResult = false;
            ToolsDocumentRequestBus::EventResult(openResult, documentId, &ToolsDocumentRequestBus::Events::Open, openPath);
            if (!openResult)
            {
                DisplayErrorMessage(
                    GetToolMainWindow(),
                    QObject::tr("Document could not be opened"),
                    QObject::tr("Failed to open: \n%1\n\n%2").arg(openPath.c_str()).arg(traceRecorder.GetDump().c_str()));
                DestroyDocument(documentId);
                return AZ::Uuid::CreateNull();
            }
        }

        if (!savePath.empty())
        {
            if (!SaveDocumentAsChild(documentId, savePath))
            {
                CloseDocument(documentId);
                return AZ::Uuid::CreateNull();
            }

            AddRecentFilePath(savePath);
        }
        else
        {
            AddRecentFilePath(openPath);
        }

        // Send document open notification after creating new one
        ToolsDocumentNotificationBus::Event(m_toolId, &ToolsDocumentNotificationBus::Events::OnDocumentOpened, documentId);

        if (traceRecorder.GetWarningCount(true) > 0)
        {
            DisplayWarningMessage(
                GetToolMainWindow(),
                QObject::tr("Document opened with warnings"),
                QObject::tr("Warnings encountered: \n%1\n\n%2").arg(openPath.c_str()).arg(traceRecorder.GetDump().c_str()));
        }

        return documentId;
    }

    bool ToolsDocumentSystem::DestroyDocument(const AZ::Uuid& documentId)
    {
        return m_documentMap.erase(documentId) != 0;
    }

    AZ::Uuid ToolsDocumentSystem::OpenDocument(const AZStd::string& sourcePath)
    {
        AZStd::string openPath = sourcePath;
        if (!ValidateDocumentPath(openPath))
        {
            DisplayErrorMessage(
                GetToolMainWindow(),
                QObject::tr("Document could not be opened"),
                QObject::tr("Document path is invalid, not in a supported project or gem folder, or marked as non-editable:\n%1").arg(openPath.c_str()));
            return AZ::Uuid::CreateNull();
        }

        // Determine if the file is already open and select it
        for (const auto& documentPair : m_documentMap)
        {
            const auto& documentId = documentPair.first;

            AZStd::string openDocumentPath;
            ToolsDocumentRequestBus::EventResult(openDocumentPath, documentId, &ToolsDocumentRequestBus::Events::GetAbsolutePath);
            if (AZ::StringFunc::Equal(openDocumentPath, openPath))
            {
                AddRecentFilePath(openPath);
                ToolsDocumentNotificationBus::Event(m_toolId, &ToolsDocumentNotificationBus::Events::OnDocumentOpened, documentId);
                return documentId;
            }
        }

        return CreateDocumentFromFilePath(openPath, {});
    }

    bool ToolsDocumentSystem::CloseDocument(const AZ::Uuid& documentId)
    {
        AZStd::string documentPath;
        ToolsDocumentRequestBus::EventResult(documentPath, documentId, &ToolsDocumentRequestBus::Events::GetAbsolutePath);

        TraceRecorder traceRecorder(m_maxMessageBoxLineCount);

        bool closeResult = true;
        ToolsDocumentRequestBus::EventResult(closeResult, documentId, &ToolsDocumentRequestBus::Events::Close);
        if (!closeResult)
        {
            DisplayErrorMessage(
                GetToolMainWindow(),
                QObject::tr("Document could not be closed"),
                QObject::tr("Failed to close: \n%1\n\n%2").arg(documentPath.c_str()).arg(traceRecorder.GetDump().c_str()));
            return false;
        }

        DestroyDocument(documentId);
        return true;
    }

    bool ToolsDocumentSystem::CloseAllDocuments()
    {
        bool result = true;
        auto documentMap = m_documentMap;
        for (const auto& documentPair : documentMap)
        {
            if (!CloseDocument(documentPair.first))
            {
                result = false;
            }
        }

        return result;
    }

    bool ToolsDocumentSystem::CloseAllDocumentsExcept(const AZ::Uuid& documentId)
    {
        bool result = true;
        auto documentMap = m_documentMap;
        for (const auto& documentPair : documentMap)
        {
            if (documentPair.first != documentId)
            {
                if (!CloseDocument(documentPair.first))
                {
                    result = false;
                }
            }
        }

        return result;
    }

    bool ToolsDocumentSystem::SaveDocument(const AZ::Uuid& documentId)
    {
        AZStd::string savePath;
        ToolsDocumentRequestBus::EventResult(savePath, documentId, &ToolsDocumentRequestBus::Events::GetAbsolutePath);

        if (!ValidateDocumentPath(savePath))
        {
            DisplayErrorMessage(
                GetToolMainWindow(),
                QObject::tr("Document could not be saved"),
                QObject::tr("Document path is invalid, not in a supported project or gem folder, or marked as non-editable:\n%1").arg(savePath.c_str()));
            return false;
        }

        const QFileInfo saveInfo(savePath.c_str());
        if (saveInfo.exists() && !saveInfo.isWritable())
        {
            DisplayErrorMessage(
                GetToolMainWindow(),
                QObject::tr("Document could not be saved"),
                QObject::tr("Document could not be overwritten:\n%1").arg(savePath.c_str()));
            return false;
        }

        TraceRecorder traceRecorder(m_maxMessageBoxLineCount);

        bool result = false;
        ToolsDocumentRequestBus::EventResult(result, documentId, &ToolsDocumentRequestBus::Events::Save);
        if (!result)
        {
            DisplayErrorMessage(
                GetToolMainWindow(),
                QObject::tr("Document could not be saved"),
                QObject::tr("Failed to save: \n%1\n\n%2").arg(savePath.c_str()).arg(traceRecorder.GetDump().c_str()));
            return false;
        }

        return true;
    }

    bool ToolsDocumentSystem::SaveDocumentAsCopy(const AZ::Uuid& documentId, const AZStd::string& targetPath)
    {
        AZStd::string savePath = targetPath;
        if (!ValidateDocumentPath(savePath))
        {
            DisplayErrorMessage(
                GetToolMainWindow(),
                QObject::tr("Document could not be saved"),
                QObject::tr("Document path is invalid, not in a supported project or gem folder, or marked as non-editable:\n%1").arg(savePath.c_str()));
            return false;
        }

        const QFileInfo saveInfo(savePath.c_str());
        if (saveInfo.exists() && !saveInfo.isWritable())
        {
            DisplayErrorMessage(
                GetToolMainWindow(),
                QObject::tr("Document could not be saved"),
                QObject::tr("Document could not be overwritten:\n%1").arg(savePath.c_str()));
            return false;
        }

        TraceRecorder traceRecorder(m_maxMessageBoxLineCount);

        bool result = false;
        ToolsDocumentRequestBus::EventResult(result, documentId, &ToolsDocumentRequestBus::Events::SaveAsCopy, savePath);
        if (!result)
        {
            DisplayErrorMessage(
                GetToolMainWindow(),
                QObject::tr("Document could not be saved"),
                QObject::tr("Failed to save: \n%1\n\n%2").arg(savePath.c_str()).arg(traceRecorder.GetDump().c_str()));
            return false;
        }

        AddRecentFilePath(savePath);
        return true;
    }

    bool ToolsDocumentSystem::SaveDocumentAsChild(const AZ::Uuid& documentId, const AZStd::string& targetPath)
    {
        AZStd::string savePath = targetPath;
        if (!ValidateDocumentPath(savePath))
        {
            DisplayErrorMessage(
                GetToolMainWindow(),
                QObject::tr("Document could not be saved"),
                QObject::tr("Document path is invalid, not in a supported project or gem folder, or marked as non-editable:\n%1").arg(savePath.c_str()));
            return false;
        }

        const QFileInfo saveInfo(savePath.c_str());
        if (saveInfo.exists() && !saveInfo.isWritable())
        {
            DisplayErrorMessage(
                GetToolMainWindow(),
                QObject::tr("Document could not be saved"),
                QObject::tr("Document could not be overwritten:\n%1").arg(savePath.c_str()));
            return false;
        }

        TraceRecorder traceRecorder(m_maxMessageBoxLineCount);

        bool result = false;
        ToolsDocumentRequestBus::EventResult(result, documentId, &ToolsDocumentRequestBus::Events::SaveAsChild, savePath);
        if (!result)
        {
            DisplayErrorMessage(
                GetToolMainWindow(),
                QObject::tr("Document could not be saved"),
                QObject::tr("Failed to save: \n%1\n\n%2").arg(savePath.c_str()).arg(traceRecorder.GetDump().c_str()));
            return false;
        }

        AddRecentFilePath(savePath);
        return true;
    }

    bool ToolsDocumentSystem::SaveAllDocuments()
    {
        bool result = true;
        for (const auto& documentPair : m_documentMap)
        {
            const auto& documentId = documentPair.first;

            AZStd::string documentPath;
            ToolsDocumentRequestBus::EventResult(documentPath, documentId, &ToolsDocumentRequestBus::Events::GetAbsolutePath);
            DocumentTypeInfo documentInfo;
            ToolsDocumentRequestBus::EventResult(documentInfo, documentId, &ToolsDocumentRequestBus::Events::GetDocumentTypeInfo);
            if (documentInfo.IsSupportedExtensionToSave(documentPath))
            {
                if (!SaveDocument(documentId))
            {
                result = false;
            }
        }
        }

        return result;
    }

    bool ToolsDocumentSystem::SaveAllModifiedDocuments()
    {
        bool result = true;
        for (const auto& documentPair : m_documentMap)
        {
            const auto& documentId = documentPair.first;

            AZStd::string documentPath;
            ToolsDocumentRequestBus::EventResult(documentPath, documentId, &ToolsDocumentRequestBus::Events::GetAbsolutePath);
            DocumentTypeInfo documentInfo;
            ToolsDocumentRequestBus::EventResult(documentInfo, documentId, &ToolsDocumentRequestBus::Events::GetDocumentTypeInfo);
            if (documentInfo.IsSupportedExtensionToSave(documentPath))
            {
            bool isModified = false;
                ToolsDocumentRequestBus::EventResult(isModified, documentId, &ToolsDocumentRequestBus::Events::IsModified);
                if (isModified)
                {
                    if (!SaveDocument(documentId))
            {
                result = false;
            }
        }
            }
        }

        m_queueSaveAllModifiedDocuments = false;
        return result;
    }

    bool ToolsDocumentSystem::QueueReopenModifiedDocuments()
    {
        if (!m_queueReopenModifiedDocuments)
        {
            m_queueReopenModifiedDocuments = true;
            const int interval =
                static_cast<int>(GetSettingsValue<AZ::s64>("/O3DE/AzToolsFramework/ToolsDocumentSystem/ReopenInterval", 500));
            QTimer::singleShot(interval, [toolId = m_toolId]() {
                ToolsDocumentSystemRequestBus::Event(toolId, &ToolsDocumentSystemRequestBus::Events::ReopenModifiedDocuments);
            });
            return true;
        }
        return false;
    }

    bool ToolsDocumentSystem::ReopenModifiedDocuments()
    {
        const bool enableHotReload = GetSettingsValue<bool>("/O3DE/AzToolsFramework/ToolsDocumentSystem/EnableAutomaticReload", true);
        if (!enableHotReload)
        {
            m_documentIdsWithDependencyChanges.clear();
            m_documentIdsWithExternalChanges.clear();
            m_queueReopenModifiedDocuments = false;
            return false;
        }

        // Postpone document reload if a modal dialog is active or the application is out of focus
        if (QApplication::activeModalWidget() || !(QApplication::applicationState() & Qt::ApplicationActive))
        {
            QueueReopenModifiedDocuments();
            return false;
        }

        const bool enableHotReloadPrompts =
            GetSettingsValue<bool>("/O3DE/AzToolsFramework/ToolsDocumentSystem/EnableAutomaticReloadPrompts", true);

        for (const AZ::Uuid& documentId : m_documentIdsWithExternalChanges)
        {
            m_documentIdsWithDependencyChanges.erase(documentId);

            AZStd::string documentPath;
            ToolsDocumentRequestBus::EventResult(documentPath, documentId, &ToolsDocumentRequestBus::Events::GetAbsolutePath);

            if (enableHotReloadPrompts &&
                (QMessageBox::question(GetToolMainWindow(),
                QObject::tr("Document was externally modified"),
                QObject::tr("Would you like to reopen the document:\n%1?").arg(documentPath.c_str()),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes))
            {
                continue;
            }

            TraceRecorder traceRecorder(m_maxMessageBoxLineCount);

            bool openResult = false;
            ToolsDocumentRequestBus::EventResult(openResult, documentId, &ToolsDocumentRequestBus::Events::Open, documentPath);
            if (!openResult)
            {
                DisplayErrorMessage(
                    GetToolMainWindow(),
                    QObject::tr("Document could not be opened"),
                    QObject::tr("Failed to open: \n%1\n\n%2").arg(documentPath.c_str()).arg(traceRecorder.GetDump().c_str()));
                CloseDocument(documentId);
            }
        }

        for (const AZ::Uuid& documentId : m_documentIdsWithDependencyChanges)
        {
            AZStd::string documentPath;
            ToolsDocumentRequestBus::EventResult(documentPath, documentId, &ToolsDocumentRequestBus::Events::GetAbsolutePath);

            if (enableHotReloadPrompts &&
                (QMessageBox::question(GetToolMainWindow(),
                QObject::tr("Document dependencies have changed"),
                QObject::tr("Would you like to update the document with these changes:\n%1?").arg(documentPath.c_str()),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes))
            {
                continue;
            }

            TraceRecorder traceRecorder(m_maxMessageBoxLineCount);

            bool openResult = false;
            ToolsDocumentRequestBus::EventResult(openResult, documentId, &ToolsDocumentRequestBus::Events::Reopen);
            if (!openResult)
            {
                DisplayErrorMessage(
                    GetToolMainWindow(),
                    QObject::tr("Document could not be opened"),
                    QObject::tr("Failed to open: \n%1\n\n%2").arg(documentPath.c_str()).arg(traceRecorder.GetDump().c_str()));
                CloseDocument(documentId);
            }
        }

        m_documentIdsWithDependencyChanges.clear();
        m_documentIdsWithExternalChanges.clear();
        m_queueReopenModifiedDocuments = false;
        return true;
    }

    AZ::u32 ToolsDocumentSystem::GetDocumentCount() const
    {
        return aznumeric_cast<AZ::u32>(m_documentMap.size());
    }

    bool ToolsDocumentSystem::IsDocumentOpen(const AZ::Uuid& documentId) const
    {
        bool result = false;
        ToolsDocumentRequestBus::EventResult(result, documentId, &ToolsDocumentRequestBus::Events::IsOpen);
        return result;
    }

    void ToolsDocumentSystem::AddRecentFilePath(const AZStd::string& absolutePath)
    {
        if (!absolutePath.empty())
        {
            // Get the list of previously stored recent file paths from the settings registry
            AZStd::vector<AZStd::string> paths = GetRecentFilePaths();

            // If the new path is already in the list then remove it Because it will be moved to the front of the list
            AZStd::erase_if(paths, [&absolutePath](const AZStd::string& currentPath) {
                return AZ::StringFunc::Equal(currentPath, absolutePath);
            });

            paths.insert(paths.begin(), absolutePath);

            constexpr const size_t recentFilePathsMax = 10;
            if (paths.size() > recentFilePathsMax)
            {
                paths.resize(recentFilePathsMax);
            }

            SetRecentFilePaths(paths);
        }
    }

    void ToolsDocumentSystem::ClearRecentFilePaths()
    {
        SetRecentFilePaths(AZStd::vector<AZStd::string>());
    }

    void ToolsDocumentSystem::SetRecentFilePaths(const AZStd::vector<AZStd::string>& absolutePaths)
    {
        SetSettingsObject("/O3DE/AzToolsFramework/ToolsDocumentSystem/RecentFilePaths", absolutePaths);
    }

    const AZStd::vector<AZStd::string> ToolsDocumentSystem::GetRecentFilePaths() const
    {
        return GetSettingsObject("/O3DE/AzToolsFramework/ToolsDocumentSystem/RecentFilePaths", AZStd::vector<AZStd::string>());
    }

    void ToolsDocumentSystem::OnDocumentModified([[maybe_unused]] const AZ::Uuid& documentId)
    {
        if (GetSettingsValue<bool>("/O3DE/AzToolsFramework/ToolsDocumentSystem/AutoSaveEnabled", false))
        {
            if (!m_queueSaveAllModifiedDocuments)
            {
                m_queueSaveAllModifiedDocuments = true;
                const int interval =
                    static_cast<int>(GetSettingsValue<AZ::s64>("/O3DE/AzToolsFramework/ToolsDocumentSystem/AutoSaveInterval", 250));
                QTimer::singleShot(interval, [toolId = m_toolId]() {
                    ToolsDocumentSystemRequestBus::Event(toolId, &ToolsDocumentSystemRequestBus::Events::SaveAllModifiedDocuments);
                });
            }
        }
    }

    void ToolsDocumentSystem::OnDocumentExternallyModified(const AZ::Uuid& documentId)
    {
        m_documentIdsWithExternalChanges.insert(documentId);
        QueueReopenModifiedDocuments();
    }

    void ToolsDocumentSystem::OnDocumentDependencyModified(const AZ::Uuid& documentId)
    {
        m_documentIdsWithDependencyChanges.insert(documentId);
        QueueReopenModifiedDocuments();
    }
} // namespace AzToolsFramework
