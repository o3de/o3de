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
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemSettings.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <Document/AtomToolsDocumentSystemComponent.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QMessageBox>
#include <QString>
AZ_POP_DISABLE_WARNING

namespace AtomToolsFramework
{
    AtomToolsDocumentSystemComponent::AtomToolsDocumentSystemComponent()
    {
    }

    void AtomToolsDocumentSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        AtomToolsDocumentSystemSettings::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AtomToolsDocumentSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AtomToolsDocumentSystemComponent>("AtomToolsDocumentSystemComponent", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AtomToolsDocumentSystemRequestBus>("AtomToolsDocumentSystemRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "atomtools")
                ->Event("CreateDocument", &AtomToolsDocumentSystemRequestBus::Events::CreateDocument)
                ->Event("DestroyDocument", &AtomToolsDocumentSystemRequestBus::Events::DestroyDocument)
                ->Event("OpenDocument", &AtomToolsDocumentSystemRequestBus::Events::OpenDocument)
                ->Event("CreateDocumentFromFile", &AtomToolsDocumentSystemRequestBus::Events::CreateDocumentFromFile)
                ->Event("CloseDocument", &AtomToolsDocumentSystemRequestBus::Events::CloseDocument)
                ->Event("CloseAllDocuments", &AtomToolsDocumentSystemRequestBus::Events::CloseAllDocuments)
                ->Event("CloseAllDocumentsExcept", &AtomToolsDocumentSystemRequestBus::Events::CloseAllDocumentsExcept)
                ->Event("SaveDocument", &AtomToolsDocumentSystemRequestBus::Events::SaveDocument)
                ->Event("SaveDocumentAsCopy", &AtomToolsDocumentSystemRequestBus::Events::SaveDocumentAsCopy)
                ->Event("SaveDocumentAsChild", &AtomToolsDocumentSystemRequestBus::Events::SaveDocumentAsChild)
                ->Event("SaveAllDocuments", &AtomToolsDocumentSystemRequestBus::Events::SaveAllDocuments)
                ;

            behaviorContext->EBus<AtomToolsDocumentRequestBus>("AtomToolsDocumentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "atomtools")
                ->Event("GetAbsolutePath", &AtomToolsDocumentRequestBus::Events::GetAbsolutePath)
                ->Event("GetRelativePath", &AtomToolsDocumentRequestBus::Events::GetRelativePath)
                ->Event("GetPropertyValue", &AtomToolsDocumentRequestBus::Events::GetPropertyValue)
                ->Event("SetPropertyValue", &AtomToolsDocumentRequestBus::Events::SetPropertyValue)
                ->Event("Open", &AtomToolsDocumentRequestBus::Events::Open)
                ->Event("Reopen", &AtomToolsDocumentRequestBus::Events::Reopen)
                ->Event("Close", &AtomToolsDocumentRequestBus::Events::Close)
                ->Event("Save", &AtomToolsDocumentRequestBus::Events::Save)
                ->Event("SaveAsChild", &AtomToolsDocumentRequestBus::Events::SaveAsChild)
                ->Event("SaveAsCopy", &AtomToolsDocumentRequestBus::Events::SaveAsCopy)
                ->Event("IsOpen", &AtomToolsDocumentRequestBus::Events::IsOpen)
                ->Event("IsModified", &AtomToolsDocumentRequestBus::Events::IsModified)
                ->Event("IsSavable", &AtomToolsDocumentRequestBus::Events::IsSavable)
                ->Event("CanUndo", &AtomToolsDocumentRequestBus::Events::CanUndo)
                ->Event("CanRedo", &AtomToolsDocumentRequestBus::Events::CanRedo)
                ->Event("Undo", &AtomToolsDocumentRequestBus::Events::Undo)
                ->Event("Redo", &AtomToolsDocumentRequestBus::Events::Redo)
                ->Event("BeginEdit", &AtomToolsDocumentRequestBus::Events::BeginEdit)
                ->Event("EndEdit", &AtomToolsDocumentRequestBus::Events::EndEdit)
                ;
        }
    }

    void AtomToolsDocumentSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AtomToolsDocumentSystemService"));
    }

    void AtomToolsDocumentSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AtomToolsDocumentSystemService"));
    }

    void AtomToolsDocumentSystemComponent::Init()
    {
    }

    void AtomToolsDocumentSystemComponent::Activate()
    {
        m_documentMap.clear();
        m_settings = AZ::UserSettings::CreateFind<AtomToolsDocumentSystemSettings>(AZ_CRC_CE("AtomToolsDocumentSystemSettings"), AZ::UserSettings::CT_GLOBAL);
        AtomToolsDocumentSystemRequestBus::Handler::BusConnect();
        AtomToolsDocumentNotificationBus::Handler::BusConnect();
    }

    void AtomToolsDocumentSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
        AtomToolsDocumentSystemRequestBus::Handler::BusDisconnect();
        m_documentMap.clear();
    }

    void AtomToolsDocumentSystemComponent::RegisterDocumentType(AZStd::function<AtomToolsDocument*()> documentCreator)
    {
        m_documentCreator = documentCreator;
    }

    AZ::Uuid AtomToolsDocumentSystemComponent::CreateDocument()
    {
        if (!m_documentCreator)
        {
            AZ_Error("AtomToolsDocument", false, "Failed to create new document");
            return AZ::Uuid::CreateNull();
        }

        AZStd::unique_ptr<AtomToolsDocument> document(m_documentCreator());
        if (!document)
        {
            AZ_Error("AtomToolsDocument", false, "Failed to create new document");
            return AZ::Uuid::CreateNull();
        }

        AZ::Uuid documentId = document->GetId();
        m_documentMap.emplace(documentId, document.release());
        return documentId;
    }

    bool AtomToolsDocumentSystemComponent::DestroyDocument(const AZ::Uuid& documentId)
    {
        return m_documentMap.erase(documentId) != 0;
    }

    void AtomToolsDocumentSystemComponent::OnDocumentExternallyModified(const AZ::Uuid& documentId)
    {
        m_documentIdsWithExternalChanges.insert(documentId);
        if (!AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void AtomToolsDocumentSystemComponent::OnDocumentDependencyModified(const AZ::Uuid& documentId)
    {
        m_documentIdsWithDependencyChanges.insert(documentId);
        if (!AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void AtomToolsDocumentSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        for (const AZ::Uuid& documentId : m_documentIdsWithExternalChanges)
        {
            AZStd::string documentPath;
            AtomToolsDocumentRequestBus::EventResult(documentPath, documentId, &AtomToolsDocumentRequestBus::Events::GetAbsolutePath);

            if (m_settings->m_showReloadDocumentPrompt &&
                (QMessageBox::question(QApplication::activeWindow(),
                QString("Document was externally modified"),
                QString("Would you like to reopen the document:\n%1?").arg(documentPath.c_str()),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes))
            {
                continue;
            }

            m_documentIdsWithDependencyChanges.erase(documentId);

            AtomToolsFramework::TraceRecorder traceRecorder(m_maxMessageBoxLineCount);

            bool openResult = false;
            AtomToolsDocumentRequestBus::EventResult(openResult, documentId, &AtomToolsDocumentRequestBus::Events::Open, documentPath);
            if (!openResult)
            {
                QMessageBox::critical(
                    QApplication::activeWindow(), QString("Document could not be opened"),
                    QString("Failed to open: \n%1\n\n%2").arg(documentPath.c_str()).arg(traceRecorder.GetDump().c_str()));
                AtomToolsDocumentSystemRequestBus::Broadcast(&AtomToolsDocumentSystemRequestBus::Events::CloseDocument, documentId);
            }
        }

        for (const AZ::Uuid& documentId : m_documentIdsWithDependencyChanges)
        {
            AZStd::string documentPath;
            AtomToolsDocumentRequestBus::EventResult(documentPath, documentId, &AtomToolsDocumentRequestBus::Events::GetAbsolutePath);

            if (m_settings->m_showReloadDocumentPrompt &&
                (QMessageBox::question(QApplication::activeWindow(),
                QString("Document dependencies have changed"),
                QString("Would you like to update the document with these changes:\n%1?").arg(documentPath.c_str()),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes))
            {
                continue;
            }

            AtomToolsFramework::TraceRecorder traceRecorder(m_maxMessageBoxLineCount);

            bool openResult = false;
            AtomToolsDocumentRequestBus::EventResult(openResult, documentId, &AtomToolsDocumentRequestBus::Events::Reopen);
            if (!openResult)
            {
                QMessageBox::critical(
                    QApplication::activeWindow(), QString("Document could not be opened"),
                    QString("Failed to open: \n%1\n\n%2").arg(documentPath.c_str()).arg(traceRecorder.GetDump().c_str()));
                AtomToolsDocumentSystemRequestBus::Broadcast(&AtomToolsDocumentSystemRequestBus::Events::CloseDocument, documentId);
            }
        }

        m_documentIdsWithDependencyChanges.clear();
        m_documentIdsWithExternalChanges.clear();
        AZ::TickBus::Handler::BusDisconnect();
    }

    AZ::Uuid AtomToolsDocumentSystemComponent::OpenDocument(AZStd::string_view sourcePath)
    {
        return OpenDocumentImpl(sourcePath, true);
    }

    AZ::Uuid AtomToolsDocumentSystemComponent::CreateDocumentFromFile(AZStd::string_view sourcePath, AZStd::string_view targetPath)
    {
        const AZ::Uuid documentId = OpenDocumentImpl(sourcePath, false);
        if (documentId.IsNull())
        {
            return AZ::Uuid::CreateNull();
        }

        if (!SaveDocumentAsChild(documentId, targetPath))
        {
            CloseDocument(documentId);
            return AZ::Uuid::CreateNull();
        }

        // Send document open notification after creating new one
        AtomToolsDocumentNotificationBus::Broadcast(&AtomToolsDocumentNotificationBus::Events::OnDocumentOpened, documentId);
        return documentId;
    }

    bool AtomToolsDocumentSystemComponent::CloseDocument(const AZ::Uuid& documentId)
    {
        bool isOpen = false;
        AtomToolsDocumentRequestBus::EventResult(isOpen, documentId, &AtomToolsDocumentRequestBus::Events::IsOpen);
        if (!isOpen)
        {
            // immediately destroy unopened documents
            AtomToolsDocumentSystemRequestBus::Broadcast(&AtomToolsDocumentSystemRequestBus::Events::DestroyDocument, documentId);
            return true;
        }

        AZStd::string documentPath;
        AtomToolsDocumentRequestBus::EventResult(documentPath, documentId, &AtomToolsDocumentRequestBus::Events::GetAbsolutePath);

        bool isModified = false;
        AtomToolsDocumentRequestBus::EventResult(isModified, documentId, &AtomToolsDocumentRequestBus::Events::IsModified);
        if (isModified)
        {
            auto selection = QMessageBox::question(QApplication::activeWindow(),
                QString("Document has unsaved changes"),
                QString("Do you want to save changes to\n%1?").arg(documentPath.c_str()),
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

        AtomToolsFramework::TraceRecorder traceRecorder(m_maxMessageBoxLineCount);

        bool closeResult = true;
        AtomToolsDocumentRequestBus::EventResult(closeResult, documentId, &AtomToolsDocumentRequestBus::Events::Close);
        if (!closeResult)
        {
            QMessageBox::critical(
                QApplication::activeWindow(), QString("Document could not be closed"),
                QString("Failed to close: \n%1\n\n%2").arg(documentPath.c_str()).arg(traceRecorder.GetDump().c_str()));
            return false;
        }

        AtomToolsDocumentSystemRequestBus::Broadcast(&AtomToolsDocumentSystemRequestBus::Events::DestroyDocument, documentId);
        return true;
    }

    bool AtomToolsDocumentSystemComponent::CloseAllDocuments()
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

    bool AtomToolsDocumentSystemComponent::CloseAllDocumentsExcept(const AZ::Uuid& documentId)
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

    bool AtomToolsDocumentSystemComponent::SaveDocument(const AZ::Uuid& documentId)
    {
        AZStd::string saveDocumentPath;
        AtomToolsDocumentRequestBus::EventResult(saveDocumentPath, documentId, &AtomToolsDocumentRequestBus::Events::GetAbsolutePath);

        if (saveDocumentPath.empty() || !AzFramework::StringFunc::Path::Normalize(saveDocumentPath))
        {
            return false;
        }

        const QFileInfo saveInfo(saveDocumentPath.c_str());
        if (saveInfo.exists() && !saveInfo.isWritable())
        {
            QMessageBox::critical(QApplication::activeWindow(), "Error", QString("Document could not be overwritten:\n%1").arg(saveDocumentPath.c_str()));
            return false;
        }

        AtomToolsFramework::TraceRecorder traceRecorder(m_maxMessageBoxLineCount);

        bool result = false;
        AtomToolsDocumentRequestBus::EventResult(result, documentId, &AtomToolsDocumentRequestBus::Events::Save);
        if (!result)
        {
            QMessageBox::critical(
                QApplication::activeWindow(), QString("Document could not be saved"),
                QString("Failed to save: \n%1\n\n%2").arg(saveDocumentPath.c_str()).arg(traceRecorder.GetDump().c_str()));
            return false;
        }

        return true;
    }

    bool AtomToolsDocumentSystemComponent::SaveDocumentAsCopy(const AZ::Uuid& documentId, AZStd::string_view targetPath)
    {
        AZStd::string saveDocumentPath = targetPath;
        if (saveDocumentPath.empty() || !AzFramework::StringFunc::Path::Normalize(saveDocumentPath))
        {
            return false;
        }

        const QFileInfo saveInfo(saveDocumentPath.c_str());
        if (saveInfo.exists() && !saveInfo.isWritable())
        {
            QMessageBox::critical(QApplication::activeWindow(), "Error", QString("Document could not be overwritten:\n%1").arg(saveDocumentPath.c_str()));
            return false;
        }

        AtomToolsFramework::TraceRecorder traceRecorder(m_maxMessageBoxLineCount);

        bool result = false;
        AtomToolsDocumentRequestBus::EventResult(result, documentId, &AtomToolsDocumentRequestBus::Events::SaveAsCopy, saveDocumentPath);
        if (!result)
        {
            QMessageBox::critical(
                QApplication::activeWindow(), QString("Document could not be saved"),
                QString("Failed to save: \n%1\n\n%2").arg(saveDocumentPath.c_str()).arg(traceRecorder.GetDump().c_str()));
            return false;
        }

        return true;
    }

    bool AtomToolsDocumentSystemComponent::SaveDocumentAsChild(const AZ::Uuid& documentId, AZStd::string_view targetPath)
    {
        AZStd::string saveDocumentPath = targetPath;
        if (saveDocumentPath.empty() || !AzFramework::StringFunc::Path::Normalize(saveDocumentPath))
        {
            return false;
        }

        const QFileInfo saveInfo(saveDocumentPath.c_str());
        if (saveInfo.exists() && !saveInfo.isWritable())
        {
            QMessageBox::critical(QApplication::activeWindow(), "Error", QString("Document could not be overwritten:\n%1").arg(saveDocumentPath.c_str()));
            return false;
        }

        AtomToolsFramework::TraceRecorder traceRecorder(m_maxMessageBoxLineCount);

        bool result = false;
        AtomToolsDocumentRequestBus::EventResult(result, documentId, &AtomToolsDocumentRequestBus::Events::SaveAsChild, saveDocumentPath);
        if (!result)
        {
            QMessageBox::critical(
                QApplication::activeWindow(), QString("Document could not be saved"),
                QString("Failed to save: \n%1\n\n%2").arg(saveDocumentPath.c_str()).arg(traceRecorder.GetDump().c_str()));
            return false;
        }

        return true;
    }

    bool AtomToolsDocumentSystemComponent::SaveAllDocuments()
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

    AZ::Uuid AtomToolsDocumentSystemComponent::OpenDocumentImpl(AZStd::string_view sourcePath, bool checkIfAlreadyOpen)
    {
        AZStd::string requestedPath = sourcePath;
        if (requestedPath.empty())
        {
            return AZ::Uuid::CreateNull();
        }

        if (!AzFramework::StringFunc::Path::Normalize(requestedPath))
        {
            QMessageBox::critical(QApplication::activeWindow(), "Error", QString("Document path is invalid:\n%1").arg(requestedPath.c_str()));
            return AZ::Uuid::CreateNull();
        }

        // Determine if the file is already open and select it
        if (checkIfAlreadyOpen)
        {
            for (const auto& documentPair : m_documentMap)
            {
                AZStd::string openDocumentPath;
                AtomToolsDocumentRequestBus::EventResult(openDocumentPath, documentPair.first, &AtomToolsDocumentRequestBus::Events::GetAbsolutePath);
                if (openDocumentPath == requestedPath)
                {
                    AtomToolsDocumentNotificationBus::Broadcast(&AtomToolsDocumentNotificationBus::Events::OnDocumentOpened, documentPair.first);
                    return documentPair.first;
                }
            }
        }

        AtomToolsFramework::TraceRecorder traceRecorder(m_maxMessageBoxLineCount);

        AZ::Uuid documentId = AZ::Uuid::CreateNull();
        AtomToolsDocumentSystemRequestBus::BroadcastResult(documentId, &AtomToolsDocumentSystemRequestBus::Events::CreateDocument);
        if (documentId.IsNull())
        {
            QMessageBox::critical(
                QApplication::activeWindow(), QString("Document could not be created"),
                QString("Failed to create: \n%1\n\n%2").arg(requestedPath.c_str()).arg(traceRecorder.GetDump().c_str()));
            return AZ::Uuid::CreateNull();
        }

        bool openResult = false;
        AtomToolsDocumentRequestBus::EventResult(openResult, documentId, &AtomToolsDocumentRequestBus::Events::Open, requestedPath);
        if (!openResult)
        {
            QMessageBox::critical(
                QApplication::activeWindow(), QString("Document could not be opened"),
                QString("Failed to open: \n%1\n\n%2").arg(requestedPath.c_str()).arg(traceRecorder.GetDump().c_str()));
            AtomToolsDocumentSystemRequestBus::Broadcast(&AtomToolsDocumentSystemRequestBus::Events::DestroyDocument, documentId);
            return AZ::Uuid::CreateNull();
        }
        else if (traceRecorder.GetWarningCount(true) > 0)
        {
            QMessageBox::warning(
                QApplication::activeWindow(), QString("Document opened with warnings"),
                QString("Warnings encountered: \n%1\n\n%2").arg(requestedPath.c_str()).arg(traceRecorder.GetDump().c_str()));
        }

        return documentId;
    }
}
