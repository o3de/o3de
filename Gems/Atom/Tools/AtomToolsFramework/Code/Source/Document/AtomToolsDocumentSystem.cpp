/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Debug/TraceRecorder.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystem.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AtomToolsFramework/Util/Util.h>
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

namespace AtomToolsFramework
{
    void DisplayErrorMessage(QWidget* parent, const QString& title, const QString& text)
    {
        AZ_Error("AtomToolsDocumentSystem", false, "%s: %s", title.toUtf8().constData(), text.toUtf8().constData());
        if (GetSettingsValue<bool>("/O3DE/AtomToolsFramework/AtomToolsDocumentSystem/DisplayErrorMessageDialogs", true))
        {
            QMessageBox::critical(parent, title, QObject::tr("%1\nThese messages can be disabled from settings.").arg(text));
        }
    }

    void DisplayWarningMessage(QWidget* parent, const QString& title, const QString& text)
    {
        AZ_Warning("AtomToolsDocumentSystem", false, "%s: %s", title.toUtf8().constData(), text.toUtf8().constData());
        if (GetSettingsValue<bool>("/O3DE/AtomToolsFramework/AtomToolsDocumentSystem/DisplayWarningMessageDialogs", true))
        {
            QMessageBox::warning(parent, title, QObject::tr("%1\nThese messages can be disabled from settings.").arg(text));
        }
    }

    void AtomToolsDocumentSystem::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AtomToolsDocumentSystem>()
                ->Version(0);

            if (auto editContext = serialize->GetEditContext())
            {
                editContext->Class<AtomToolsDocumentSystem>("AtomToolsDocumentSystem", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AtomToolsDocumentSystemRequestBus>("AtomToolsDocumentSystemRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "atomtools")
                ->Event("CreateDocumentFromTypeName", &AtomToolsDocumentSystemRequestBus::Events::CreateDocumentFromTypeName)
                ->Event("CreateDocumentFromFileType", &AtomToolsDocumentSystemRequestBus::Events::CreateDocumentFromFileType)
                ->Event("CreateDocumentFromFilePath", &AtomToolsDocumentSystemRequestBus::Events::CreateDocumentFromFilePath)
                ->Event("DestroyDocument", &AtomToolsDocumentSystemRequestBus::Events::DestroyDocument)
                ->Event("OpenDocument", &AtomToolsDocumentSystemRequestBus::Events::OpenDocument)
                ->Event("CloseDocument", &AtomToolsDocumentSystemRequestBus::Events::CloseDocument)
                ->Event("CloseAllDocuments", &AtomToolsDocumentSystemRequestBus::Events::CloseAllDocuments)
                ->Event("CloseAllDocumentsExcept", &AtomToolsDocumentSystemRequestBus::Events::CloseAllDocumentsExcept)
                ->Event("SaveDocument", &AtomToolsDocumentSystemRequestBus::Events::SaveDocument)
                ->Event("SaveDocumentAsCopy", &AtomToolsDocumentSystemRequestBus::Events::SaveDocumentAsCopy)
                ->Event("SaveDocumentAsChild", &AtomToolsDocumentSystemRequestBus::Events::SaveDocumentAsChild)
                ->Event("SaveAllDocuments", &AtomToolsDocumentSystemRequestBus::Events::SaveAllDocuments)
                ->Event("SaveAllModifiedDocuments", &AtomToolsDocumentSystemRequestBus::Events::SaveAllModifiedDocuments)
                ->Event("QueueReopenModifiedDocuments", &AtomToolsDocumentSystemRequestBus::Events::QueueReopenModifiedDocuments)
                ->Event("ReopenModifiedDocuments", &AtomToolsDocumentSystemRequestBus::Events::ReopenModifiedDocuments)
                ->Event("GetDocumentCount", &AtomToolsDocumentSystemRequestBus::Events::GetDocumentCount)
                ->Event("IsDocumentOpen", &AtomToolsDocumentSystemRequestBus::Events::IsDocumentOpen)
                ->Event("AddRecentFilePath", &AtomToolsDocumentSystemRequestBus::Events::AddRecentFilePath)
                ->Event("ClearRecentFilePaths", &AtomToolsDocumentSystemRequestBus::Events::ClearRecentFilePaths)
                ->Event("SetRecentFilePaths", &AtomToolsDocumentSystemRequestBus::Events::SetRecentFilePaths)
                ->Event("GetRecentFilePaths", &AtomToolsDocumentSystemRequestBus::Events::GetRecentFilePaths)
                ;
        }
    }

    AtomToolsDocumentSystem::AtomToolsDocumentSystem(const AZ::Crc32& toolId)
        : m_toolId(toolId)
    {
        AtomToolsDocumentSystemRequestBus::Handler::BusConnect(m_toolId);
        AtomToolsDocumentNotificationBus::Handler::BusConnect(m_toolId);
    }

    AtomToolsDocumentSystem::~AtomToolsDocumentSystem()
    {
        m_documentMap.clear();
        AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
        AtomToolsDocumentSystemRequestBus::Handler::BusDisconnect();
    }

    void AtomToolsDocumentSystem::RegisterDocumentType(const DocumentTypeInfo& documentType)
    {
        m_documentTypes.push_back(documentType);
    }

    const DocumentTypeInfoVector& AtomToolsDocumentSystem::GetRegisteredDocumentTypes() const
    {
        return m_documentTypes;
    }

    AZ::Uuid AtomToolsDocumentSystem::CreateDocumentFromType(const DocumentTypeInfo& documentType)
    {
        AZStd::unique_ptr<AtomToolsDocumentRequests> document(documentType.CreateDocument(m_toolId));
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

    AZ::Uuid AtomToolsDocumentSystem::CreateDocumentFromTypeName(const AZStd::string& documentTypeName)
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

    AZ::Uuid AtomToolsDocumentSystem::CreateDocumentFromFileType(const AZStd::string& path)
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

    AZ::Uuid AtomToolsDocumentSystem::CreateDocumentFromFilePath(const AZStd::string& sourcePath, const AZStd::string& targetPath)
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
            AtomToolsDocumentRequestBus::EventResult(openResult, documentId, &AtomToolsDocumentRequestBus::Events::Open, openPath);
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
        AtomToolsDocumentNotificationBus::Event(m_toolId, &AtomToolsDocumentNotificationBus::Events::OnDocumentOpened, documentId);

        if (traceRecorder.GetWarningCount(true) > 0)
        {
            DisplayWarningMessage(
                GetToolMainWindow(),
                QObject::tr("Document opened with warnings"),
                QObject::tr("Warnings encountered: \n%1\n\n%2").arg(openPath.c_str()).arg(traceRecorder.GetDump().c_str()));
        }

        return documentId;
    }

    bool AtomToolsDocumentSystem::DestroyDocument(const AZ::Uuid& documentId)
    {
        return m_documentMap.erase(documentId) != 0;
    }

    AZ::Uuid AtomToolsDocumentSystem::OpenDocument(const AZStd::string& sourcePath)
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
            AtomToolsDocumentRequestBus::EventResult(openDocumentPath, documentId, &AtomToolsDocumentRequestBus::Events::GetAbsolutePath);
            if (AZ::StringFunc::Equal(openDocumentPath, openPath))
            {
                AddRecentFilePath(openPath);
                AtomToolsDocumentNotificationBus::Event(m_toolId, &AtomToolsDocumentNotificationBus::Events::OnDocumentOpened, documentId);
                return documentId;
            }
        }

        return CreateDocumentFromFilePath(openPath, {});
    }

    bool AtomToolsDocumentSystem::CloseDocument(const AZ::Uuid& documentId)
    {
        AZStd::string documentPath;
        AtomToolsDocumentRequestBus::EventResult(documentPath, documentId, &AtomToolsDocumentRequestBus::Events::GetAbsolutePath);

        TraceRecorder traceRecorder(m_maxMessageBoxLineCount);

        bool closeResult = true;
        AtomToolsDocumentRequestBus::EventResult(closeResult, documentId, &AtomToolsDocumentRequestBus::Events::Close);
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

    bool AtomToolsDocumentSystem::CloseAllDocuments()
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

    bool AtomToolsDocumentSystem::CloseAllDocumentsExcept(const AZ::Uuid& documentId)
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

    bool AtomToolsDocumentSystem::SaveDocument(const AZ::Uuid& documentId)
    {
        AZStd::string savePath;
        AtomToolsDocumentRequestBus::EventResult(savePath, documentId, &AtomToolsDocumentRequestBus::Events::GetAbsolutePath);

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
        AtomToolsDocumentRequestBus::EventResult(result, documentId, &AtomToolsDocumentRequestBus::Events::Save);
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

    bool AtomToolsDocumentSystem::SaveDocumentAsCopy(const AZ::Uuid& documentId, const AZStd::string& targetPath)
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
        AtomToolsDocumentRequestBus::EventResult(result, documentId, &AtomToolsDocumentRequestBus::Events::SaveAsCopy, savePath);
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

    bool AtomToolsDocumentSystem::SaveDocumentAsChild(const AZ::Uuid& documentId, const AZStd::string& targetPath)
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
        AtomToolsDocumentRequestBus::EventResult(result, documentId, &AtomToolsDocumentRequestBus::Events::SaveAsChild, savePath);
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

    bool AtomToolsDocumentSystem::SaveAllDocuments()
    {
        bool result = true;
        for (const auto& documentPair : m_documentMap)
        {
            bool canSave = false;
            AtomToolsDocumentRequestBus::EventResult(canSave, documentPair.first, &AtomToolsDocumentRequestBus::Events::CanSave);
            if (canSave && !SaveDocument(documentPair.first))
            {
                result = false;
            }
        }

        return result;
    }

    bool AtomToolsDocumentSystem::SaveAllModifiedDocuments()
    {
        bool result = true;
        for (const auto& documentPair : m_documentMap)
        {
            bool isModified = false;
            AtomToolsDocumentRequestBus::EventResult(isModified, documentPair.first, &AtomToolsDocumentRequestBus::Events::IsModified);
            bool canSave = false;
            AtomToolsDocumentRequestBus::EventResult(canSave, documentPair.first, &AtomToolsDocumentRequestBus::Events::CanSave);
            if (isModified && canSave && !SaveDocument(documentPair.first))
            {
                result = false;
            }
        }

        m_queueSaveAllModifiedDocuments = false;
        return result;
    }

    bool AtomToolsDocumentSystem::QueueReopenModifiedDocuments()
    {
        if (!m_queueReopenModifiedDocuments)
        {
            m_queueReopenModifiedDocuments = true;
            const int interval =
                static_cast<int>(GetSettingsValue<AZ::s64>("/O3DE/AtomToolsFramework/AtomToolsDocumentSystem/ReopenInterval", 500));
            QTimer::singleShot(interval, [toolId = m_toolId]() {
                AtomToolsDocumentSystemRequestBus::Event(toolId, &AtomToolsDocumentSystemRequestBus::Events::ReopenModifiedDocuments);
            });
            return true;
        }
        return false;
    }

    bool AtomToolsDocumentSystem::ReopenModifiedDocuments()
    {
        const bool enableHotReload = GetSettingsValue<bool>("/O3DE/AtomToolsFramework/AtomToolsDocumentSystem/EnableAutomaticReload", true);
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
            GetSettingsValue<bool>("/O3DE/AtomToolsFramework/AtomToolsDocumentSystem/EnableAutomaticReloadPrompts", true);

        for (const AZ::Uuid& documentId : m_documentIdsWithExternalChanges)
        {
            m_documentIdsWithDependencyChanges.erase(documentId);

            AZStd::string documentPath;
            AtomToolsDocumentRequestBus::EventResult(documentPath, documentId, &AtomToolsDocumentRequestBus::Events::GetAbsolutePath);

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
            AtomToolsDocumentRequestBus::EventResult(openResult, documentId, &AtomToolsDocumentRequestBus::Events::Open, documentPath);
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
            AtomToolsDocumentRequestBus::EventResult(documentPath, documentId, &AtomToolsDocumentRequestBus::Events::GetAbsolutePath);

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
            AtomToolsDocumentRequestBus::EventResult(openResult, documentId, &AtomToolsDocumentRequestBus::Events::Reopen);
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

    AZ::u32 AtomToolsDocumentSystem::GetDocumentCount() const
    {
        return aznumeric_cast<AZ::u32>(m_documentMap.size());
    }

    bool AtomToolsDocumentSystem::IsDocumentOpen(const AZ::Uuid& documentId) const
    {
        bool result = false;
        AtomToolsDocumentRequestBus::EventResult(result, documentId, &AtomToolsDocumentRequestBus::Events::IsOpen);
        return result;
    }

    void AtomToolsDocumentSystem::AddRecentFilePath(const AZStd::string& absolutePath)
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

    void AtomToolsDocumentSystem::ClearRecentFilePaths()
    {
        SetRecentFilePaths(AZStd::vector<AZStd::string>());
    }

    void AtomToolsDocumentSystem::SetRecentFilePaths(const AZStd::vector<AZStd::string>& absolutePaths)
    {
        SetSettingsObject("/O3DE/AtomToolsFramework/AtomToolsDocumentSystem/RecentFilePaths", absolutePaths);
    }

    const AZStd::vector<AZStd::string> AtomToolsDocumentSystem::GetRecentFilePaths() const
    {
        return GetSettingsObject("/O3DE/AtomToolsFramework/AtomToolsDocumentSystem/RecentFilePaths", AZStd::vector<AZStd::string>());
    }

    void AtomToolsDocumentSystem::OnDocumentModified([[maybe_unused]] const AZ::Uuid& documentId)
    {
        if (GetSettingsValue<bool>("/O3DE/AtomToolsFramework/AtomToolsDocumentSystem/AutoSaveEnabled", false))
        {
            if (!m_queueSaveAllModifiedDocuments)
            {
                m_queueSaveAllModifiedDocuments = true;
                const int interval =
                    static_cast<int>(GetSettingsValue<AZ::s64>("/O3DE/AtomToolsFramework/AtomToolsDocumentSystem/AutoSaveInterval", 250));
                QTimer::singleShot(interval, [toolId = m_toolId]() {
                    AtomToolsDocumentSystemRequestBus::Event(toolId, &AtomToolsDocumentSystemRequestBus::Events::SaveAllModifiedDocuments);
                });
            }
        }
    }

    void AtomToolsDocumentSystem::OnDocumentExternallyModified(const AZ::Uuid& documentId)
    {
        m_documentIdsWithExternalChanges.insert(documentId);
        QueueReopenModifiedDocuments();
    }

    void AtomToolsDocumentSystem::OnDocumentDependencyModified(const AZ::Uuid& documentId)
    {
        m_documentIdsWithDependencyChanges.insert(documentId);
        QueueReopenModifiedDocuments();
    }
} // namespace AtomToolsFramework
