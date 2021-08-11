/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Document/ShaderManagementConsoleDocumentSystemComponent.h>

#include <AtomToolsFramework/Debug/TraceRecorder.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AtomToolsFramework/Util/Util.h>

#include <Atom/Document/ShaderManagementConsoleDocumentSystemRequestBus.h>
#include <Atom/Document/ShaderManagementConsoleDocumentRequestBus.h>
#include <Atom/Document/ShaderManagementConsoleDocumentNotificationBus.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QStyle>
#include <QMessageBox>
#include <QFileDialog>
AZ_POP_DISABLE_WARNING

namespace ShaderManagementConsole
{
    ShaderManagementConsoleDocumentSystemComponent::ShaderManagementConsoleDocumentSystemComponent()
    {
    }

    void ShaderManagementConsoleDocumentSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ShaderManagementConsoleDocumentSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<ShaderManagementConsoleDocumentSystemComponent>("ShaderManagementConsoleDocumentSystemComponent", "Manages documents")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<ShaderManagementConsoleDocumentSystemRequestBus>("ShaderManagementConsoleDocumentSystemRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "shadermanagementconsole")
                ->Event("CreateDocument", &ShaderManagementConsoleDocumentSystemRequestBus::Events::CreateDocument)
                ->Event("DestroyDocument", &ShaderManagementConsoleDocumentSystemRequestBus::Events::DestroyDocument)
                ->Event("OpenDocument", &ShaderManagementConsoleDocumentSystemRequestBus::Events::OpenDocument)
                ->Event("CloseDocument", &ShaderManagementConsoleDocumentSystemRequestBus::Events::CloseDocument)
                ->Event("CloseAllDocuments", &ShaderManagementConsoleDocumentSystemRequestBus::Events::CloseAllDocuments)
                ->Event("CloseAllDocumentsExcept", &ShaderManagementConsoleDocumentSystemRequestBus::Events::CloseAllDocumentsExcept)
                ->Event("SaveDocument", &ShaderManagementConsoleDocumentSystemRequestBus::Events::SaveDocument)
                ->Event("SaveDocumentAsCopy", &ShaderManagementConsoleDocumentSystemRequestBus::Events::SaveDocumentAsCopy)
                ->Event("SaveAllDocuments", &ShaderManagementConsoleDocumentSystemRequestBus::Events::SaveAllDocuments)
                ;

            behaviorContext->EBus<ShaderManagementConsoleDocumentRequestBus>("ShaderManagementConsoleDocumentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "shadermanagementconsole")
                ->Event("GetAbsolutePath", &ShaderManagementConsoleDocumentRequestBus::Events::GetAbsolutePath)
                ->Event("GetRelativePath", &ShaderManagementConsoleDocumentRequestBus::Events::GetRelativePath)
                ->Event("GetShaderOptionCount", &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderOptionCount)
                ->Event("GetShaderOptionDescriptor", &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderOptionDescriptor)
                ->Event("GetShaderVariantCount", &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderVariantCount)
                ->Event("GetShaderVariantInfo", &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderVariantInfo)
                ->Event("Open", &ShaderManagementConsoleDocumentRequestBus::Events::Open)
                ->Event("Close", &ShaderManagementConsoleDocumentRequestBus::Events::Close)
                ->Event("Save", &ShaderManagementConsoleDocumentRequestBus::Events::Save)
                ->Event("SaveAsCopy", &ShaderManagementConsoleDocumentRequestBus::Events::SaveAsCopy)
                ->Event("IsOpen", &ShaderManagementConsoleDocumentRequestBus::Events::IsOpen)
                ->Event("IsModified", &ShaderManagementConsoleDocumentRequestBus::Events::IsModified)
                ->Event("IsSavable", &ShaderManagementConsoleDocumentRequestBus::Events::IsSavable)
                ->Event("CanUndo", &ShaderManagementConsoleDocumentRequestBus::Events::CanUndo)
                ->Event("CanRedo", &ShaderManagementConsoleDocumentRequestBus::Events::CanRedo)
                ->Event("Undo", &ShaderManagementConsoleDocumentRequestBus::Events::Undo)
                ->Event("Redo", &ShaderManagementConsoleDocumentRequestBus::Events::Redo)
                ->Event("BeginEdit", &ShaderManagementConsoleDocumentRequestBus::Events::BeginEdit)
                ->Event("EndEdit", &ShaderManagementConsoleDocumentRequestBus::Events::EndEdit)
                ;
        }
    }

    void ShaderManagementConsoleDocumentSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("AssetProcessorToolsConnection", 0x734669bc));
        required.push_back(AZ_CRC("AssetDatabaseService", 0x3abf5601));
        required.push_back(AZ_CRC("PropertyManagerService", 0x63a3d7ad));
        required.push_back(AZ_CRC("RPISystem", 0xf2add773));
    }

    void ShaderManagementConsoleDocumentSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ShaderManagementConsoleDocumentSystemService"));
    }

    void ShaderManagementConsoleDocumentSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("ShaderManagementConsoleDocumentSystemService"));
    }

    void ShaderManagementConsoleDocumentSystemComponent::Init()
    {
    }

    void ShaderManagementConsoleDocumentSystemComponent::Activate()
    {
        m_documentMap.clear();
        ShaderManagementConsoleDocumentSystemRequestBus::Handler::BusConnect();
    }

    void ShaderManagementConsoleDocumentSystemComponent::Deactivate()
    {
        ShaderManagementConsoleDocumentSystemRequestBus::Handler::BusDisconnect();
        m_documentMap.clear();
    }

    AZ::Uuid ShaderManagementConsoleDocumentSystemComponent::CreateDocument()
    {
        auto document = AZStd::make_unique<ShaderManagementConsoleDocument>();
        if (!document)
        {
            AZ_Error("ShaderManagementConsoleDocument", false, "Failed to create new document");
            return AZ::Uuid::CreateNull();
        }

        AZ::Uuid documentId = document->GetId();
        m_documentMap.emplace(documentId, document.release());
        return documentId;
    }

    bool ShaderManagementConsoleDocumentSystemComponent::DestroyDocument(const AZ::Uuid& documentId)
    {
        return m_documentMap.erase(documentId) != 0;
    }

    AZ::Uuid ShaderManagementConsoleDocumentSystemComponent::OpenDocument(AZStd::string_view sourcePath)
    {
        return OpenDocumentImpl(sourcePath, true);
    }

    bool ShaderManagementConsoleDocumentSystemComponent::CloseDocument(const AZ::Uuid& documentId)
    {
        bool isOpen = false;
        ShaderManagementConsoleDocumentRequestBus::EventResult(isOpen, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::IsOpen);
        if (!isOpen)
        {
            // immediately destroy unopened documents
            ShaderManagementConsoleDocumentSystemRequestBus::Broadcast(&ShaderManagementConsoleDocumentSystemRequestBus::Events::DestroyDocument, documentId);
            return true;
        }

        AZStd::string documentPath;
        ShaderManagementConsoleDocumentRequestBus::EventResult(documentPath, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetAbsolutePath);

        bool isModified = false;
        ShaderManagementConsoleDocumentRequestBus::EventResult(isModified, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::IsModified);
        if (isModified)
        {
            auto selection = QMessageBox::question(QApplication::activeWindow(),
                QString("Document has unsaved changes"),
                QString("Do you want to save changes to\n%1?").arg(documentPath.c_str()),
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            if (selection == QMessageBox::Cancel)
            {
                AZ_TracePrintf("ShaderManagementConsoleDocument", "Close document canceled: %s", documentPath.c_str());
                return false;
            }
            if (selection == QMessageBox::Yes)
            {
                if (!SaveDocument(documentId))
                {
                    AZ_Error("ShaderManagementConsoleDocument", false, "Close document failed because document was not saved: %s", documentPath.c_str());
                    return false;
                }
            }
        }

        AtomToolsFramework::TraceRecorder traceRecorder(m_maxMessageBoxLineCount);

        bool closeResult = true;
        ShaderManagementConsoleDocumentRequestBus::EventResult(closeResult, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::Close);
        if (!closeResult)
        {
            QMessageBox::critical(
                QApplication::activeWindow(), QString("Document could not be closed"),
                QString("Failed to close: \n%1\n\n%2").arg(documentPath.c_str()).arg(traceRecorder.GetDump().c_str()));
            return false;
        }

        ShaderManagementConsoleDocumentSystemRequestBus::Broadcast(&ShaderManagementConsoleDocumentSystemRequestBus::Events::DestroyDocument, documentId);
        return true;
    }

    bool ShaderManagementConsoleDocumentSystemComponent::CloseAllDocuments()
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

    bool ShaderManagementConsoleDocumentSystemComponent::CloseAllDocumentsExcept(const AZ::Uuid& documentId)
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

    bool ShaderManagementConsoleDocumentSystemComponent::SaveDocument(const AZ::Uuid& documentId)
    {
        AZStd::string saveDocumentPath;
        ShaderManagementConsoleDocumentRequestBus::EventResult(saveDocumentPath, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetAbsolutePath);

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
        ShaderManagementConsoleDocumentRequestBus::EventResult(result, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::Save);
        if (!result)
        {
            QMessageBox::critical(
                QApplication::activeWindow(), QString("Document could not be saved"),
                QString("Failed to save: \n%1\n\n%2").arg(saveDocumentPath.c_str()).arg(traceRecorder.GetDump().c_str()));
            return false;
        }

        return true;
    }

    bool ShaderManagementConsoleDocumentSystemComponent::SaveDocumentAsCopy(const AZ::Uuid& documentId, AZStd::string_view targetPath)
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
        ShaderManagementConsoleDocumentRequestBus::EventResult(result, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::SaveAsCopy, saveDocumentPath);
        if (!result)
        {
            QMessageBox::critical(
                QApplication::activeWindow(), QString("Document could not be saved"),
                QString("Failed to save: \n%1\n\n%2").arg(saveDocumentPath.c_str()).arg(traceRecorder.GetDump().c_str()));
            return false;
        }

        return true;
    }

    bool ShaderManagementConsoleDocumentSystemComponent::SaveAllDocuments()
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

    AZ::Uuid ShaderManagementConsoleDocumentSystemComponent::OpenDocumentImpl(AZStd::string_view sourcePath, bool checkIfAlreadyOpen)
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
                ShaderManagementConsoleDocumentRequestBus::EventResult(openDocumentPath, documentPair.first, &ShaderManagementConsoleDocumentRequestBus::Events::GetAbsolutePath);
                if (openDocumentPath == requestedPath)
                {
                    ShaderManagementConsoleDocumentNotificationBus::Broadcast(&ShaderManagementConsoleDocumentNotificationBus::Events::OnDocumentOpened, documentPair.first);
                    return documentPair.first;
                }
            }
        }

        AtomToolsFramework::TraceRecorder traceRecorder(m_maxMessageBoxLineCount);

        AZ::Uuid documentId = AZ::Uuid::CreateNull();
        ShaderManagementConsoleDocumentSystemRequestBus::BroadcastResult(documentId, &ShaderManagementConsoleDocumentSystemRequestBus::Events::CreateDocument);
        if (documentId.IsNull())
        {
            QMessageBox::critical(
                QApplication::activeWindow(), QString("Document could not be created"),
                QString("Failed to create: \n%1\n\n%2").arg(requestedPath.c_str()).arg(traceRecorder.GetDump().c_str()));
            return AZ::Uuid::CreateNull();
        }

        traceRecorder.GetDump().clear();

        bool openResult = false;
        ShaderManagementConsoleDocumentRequestBus::EventResult(openResult, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::Open, requestedPath);
        if (!openResult)
        {
            QMessageBox::critical(
                QApplication::activeWindow(), QString("Document could not be opened"),
                QString("Failed to open: \n%1\n\n%2").arg(requestedPath.c_str()).arg(traceRecorder.GetDump().c_str()));
            ShaderManagementConsoleDocumentSystemRequestBus::Broadcast(&ShaderManagementConsoleDocumentSystemRequestBus::Events::DestroyDocument, documentId);
            return AZ::Uuid::CreateNull();
        }

        return documentId;
    }
}
