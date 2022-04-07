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
                ->Event("GetDocumentCount", &AtomToolsDocumentSystemRequestBus::Events::GetDocumentCount)
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
            AZ_Error("AtomToolsDocument", false, "Failed to create new document.");
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
            if (documentType.IsSupportedExtensionToCreate(path))
            {
                return CreateDocumentFromType(documentType);
            }
        }
        return AZ::Uuid();
    }

    AZ::Uuid AtomToolsDocumentSystem::CreateDocumentFromFilePath(const AZStd::string& sourcePath, const AZStd::string& targetPath)
    {
        TraceRecorder traceRecorder(m_maxMessageBoxLineCount);

        AZStd::string openPath = sourcePath;
        if (!ValidateDocumentPath(openPath))
        {
            QMessageBox::critical(
                GetToolMainWindow(),
                QObject::tr("Document could not be created"),
                QObject::tr("Document path must be valid and in a recognized source asset folder:\n%1").arg(openPath.c_str()));
            return AZ::Uuid::CreateNull();
        }

        AZStd::string savePath = targetPath;
        if (!savePath.empty() && !ValidateDocumentPath(savePath))
        {
            QMessageBox::critical(
                GetToolMainWindow(),
                QObject::tr("Document could not be created"),
                QObject::tr("Document path must be valid and in a recognized source asset folder:\n%1").arg(savePath.c_str()));
            return AZ::Uuid::CreateNull();
        }

        AZ::Uuid documentId = CreateDocumentFromFileType(openPath);
        if (documentId.IsNull())
        {
            QMessageBox::critical(
                GetToolMainWindow(),
                QObject::tr("Document could not be created"),
                QObject::tr("Failed to create: \n%1\n\n%2").arg(openPath.c_str()).arg(traceRecorder.GetDump().c_str()));
            return AZ::Uuid::CreateNull();
        }

        bool openResult = false;
        AtomToolsDocumentRequestBus::EventResult(openResult, documentId, &AtomToolsDocumentRequestBus::Events::Open, openPath);
        if (!openResult)
        {
            QMessageBox::critical(
                GetToolMainWindow(),
                QObject::tr("Document could not be opened"),
                QObject::tr("Failed to open: \n%1\n\n%2").arg(openPath.c_str()).arg(traceRecorder.GetDump().c_str()));
            DestroyDocument(documentId);
            return AZ::Uuid::CreateNull();
        }

        if (!savePath.empty())
        {
            if (!SaveDocumentAsChild(documentId, savePath))
            {
                CloseDocument(documentId);
                return AZ::Uuid::CreateNull();
            }

            // Send document open notification after creating new one
            AtomToolsDocumentNotificationBus::Event(m_toolId, &AtomToolsDocumentNotificationBus::Events::OnDocumentOpened, documentId);
        }

        if (traceRecorder.GetWarningCount(true) > 0)
        {
            QMessageBox::warning(
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
            QMessageBox::critical(
                GetToolMainWindow(),
                QObject::tr("Document could not be opened"),
                QObject::tr("Document path must be valid and in a recognized source asset folder:\n%1").arg(openPath.c_str()));
            return AZ::Uuid::CreateNull();
        }

        // Determine if the file is already open and select it
        for (const auto& documentPair : m_documentMap)
        {
            AZStd::string openDocumentPath;
            AtomToolsDocumentRequestBus::EventResult(
                openDocumentPath, documentPair.first, &AtomToolsDocumentRequestBus::Events::GetAbsolutePath);
            if (AZ::StringFunc::Equal(openDocumentPath, openPath))
            {
                AtomToolsDocumentNotificationBus::Event(
                    m_toolId, &AtomToolsDocumentNotificationBus::Events::OnDocumentOpened, documentPair.first);
                return documentPair.first;
            }
        }

        return CreateDocumentFromFilePath(openPath, {});
    }

    bool AtomToolsDocumentSystem::CloseDocument(const AZ::Uuid& documentId)
    {
        bool isOpen = false;
        AtomToolsDocumentRequestBus::EventResult(isOpen, documentId, &AtomToolsDocumentRequestBus::Events::IsOpen);
        if (!isOpen)
        {
            // immediately destroy unopened documents
            DestroyDocument(documentId);
            return true;
        }

        AZStd::string documentPath;
        AtomToolsDocumentRequestBus::EventResult(documentPath, documentId, &AtomToolsDocumentRequestBus::Events::GetAbsolutePath);

        bool isModified = false;
        AtomToolsDocumentRequestBus::EventResult(isModified, documentId, &AtomToolsDocumentRequestBus::Events::IsModified);
        if (isModified)
        {
            auto selection = QMessageBox::question(
                GetToolMainWindow(),
                QObject::tr("Document has unsaved changes"),
                QObject::tr("Do you want to save changes to\n%1?").arg(documentPath.c_str()),
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            if (selection == QMessageBox::Cancel)
            {
                AZ_TracePrintf("AtomToolsDocument", "Close document canceled: %s", documentPath.c_str());
                return false;
            }
            if (selection == QMessageBox::Yes)
            {
                if (!SaveDocument(documentId))
                {
                    AZ_Error("AtomToolsDocument", false, "Close document failed because document was not saved: %s", documentPath.c_str());
                    return false;
                }
            }
        }

        TraceRecorder traceRecorder(m_maxMessageBoxLineCount);

        bool closeResult = true;
        AtomToolsDocumentRequestBus::EventResult(closeResult, documentId, &AtomToolsDocumentRequestBus::Events::Close);
        if (!closeResult)
        {
            QMessageBox::critical(
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
            QMessageBox::critical(
                GetToolMainWindow(),
                QObject::tr("Document could not be saved"),
                QObject::tr("Document path must be valid and in a recognized source asset folder:\n%1").arg(savePath.c_str()));
            return false;
        }

        const QFileInfo saveInfo(savePath.c_str());
        if (saveInfo.exists() && !saveInfo.isWritable())
        {
            QMessageBox::critical(
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
            QMessageBox::critical(
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
            QMessageBox::critical(
                GetToolMainWindow(),
                QObject::tr("Document could not be saved"),
                QObject::tr("Document path must be valid and in a recognized source asset folder:\n%1").arg(savePath.c_str()));
            return false;
        }

        const QFileInfo saveInfo(savePath.c_str());
        if (saveInfo.exists() && !saveInfo.isWritable())
        {
            QMessageBox::critical(
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
            QMessageBox::critical(
                GetToolMainWindow(),
                QObject::tr("Document could not be saved"),
                QObject::tr("Failed to save: \n%1\n\n%2").arg(savePath.c_str()).arg(traceRecorder.GetDump().c_str()));
            return false;
        }

        return true;
    }

    bool AtomToolsDocumentSystem::SaveDocumentAsChild(const AZ::Uuid& documentId, const AZStd::string& targetPath)
    {
        AZStd::string savePath = targetPath;
        if (!ValidateDocumentPath(savePath))
        {
            QMessageBox::critical(
                GetToolMainWindow(),
                QObject::tr("Document could not be saved"),
                QObject::tr("Document path must be valid and in a recognized source asset folder:\n%1").arg(savePath.c_str()));
            return false;
        }

        const QFileInfo saveInfo(savePath.c_str());
        if (saveInfo.exists() && !saveInfo.isWritable())
        {
            QMessageBox::critical(
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
            QMessageBox::critical(
                GetToolMainWindow(),
                QObject::tr("Document could not be saved"),
                QObject::tr("Failed to save: \n%1\n\n%2").arg(savePath.c_str()).arg(traceRecorder.GetDump().c_str()));
            return false;
        }

        return true;
    }

    bool AtomToolsDocumentSystem::SaveAllDocuments()
    {
        bool result = true;
        for (const auto& documentPair : m_documentMap)
        {
            if (!SaveDocument(documentPair.first))
            {
                result = false;
            }
        }

        return result;
    }

    AZ::u32 AtomToolsDocumentSystem::GetDocumentCount() const
    {
        return aznumeric_cast<AZ::u32>(m_documentMap.size());
    }


    void AtomToolsDocumentSystem::OnDocumentExternallyModified(const AZ::Uuid& documentId)
    {
        m_documentIdsWithExternalChanges.insert(documentId);
        QueueReopenDocuments();
    }

    void AtomToolsDocumentSystem::OnDocumentDependencyModified(const AZ::Uuid& documentId)
    {
        m_documentIdsWithDependencyChanges.insert(documentId);
        QueueReopenDocuments();
    }

    void AtomToolsDocumentSystem::QueueReopenDocuments()
    {
        if (!m_queueReopenDocuments)
        {
            m_queueReopenDocuments = true;
            QTimer::singleShot(500, [this] { ReopenDocuments(); });
        }
    }

    void AtomToolsDocumentSystem::ReopenDocuments()
    {
        m_queueReopenDocuments = false;

        const bool enableHotReload = GetSettingsValue<bool>("/O3DE/AtomToolsFramework/AtomToolsDocumentSystem/EnableHotReload", true);
        if (!enableHotReload)
        {
            m_documentIdsWithDependencyChanges.clear();
            m_documentIdsWithExternalChanges.clear();
            return;
        }

        // Postpone document reload if a modal dialog is active or the application is out of focus
        if (QApplication::activeModalWidget() || !(QApplication::applicationState() & Qt::ApplicationActive))
        {
            QueueReopenDocuments();
            return;
        }

        const bool enableHotReloadPrompts =
            GetSettingsValue<bool>("/O3DE/AtomToolsFramework/AtomToolsDocumentSystem/EnableHotReloadPrompts", true);

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
                QMessageBox::critical(
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
                QMessageBox::critical(
                    GetToolMainWindow(),
                    QObject::tr("Document could not be opened"),
                    QObject::tr("Failed to open: \n%1\n\n%2").arg(documentPath.c_str()).arg(traceRecorder.GetDump().c_str()));
                CloseDocument(documentId);
            }
        }

        m_documentIdsWithDependencyChanges.clear();
        m_documentIdsWithExternalChanges.clear();
    }
} // namespace AtomToolsFramework
