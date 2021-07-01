/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Document/ShaderManagementConsoleDocumentSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

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

    AZ::Uuid ShaderManagementConsoleDocumentSystemComponent::OpenDocument(AZStd::string_view path)
    {
        return OpenDocumentImpl(path, true);
    }

    bool ShaderManagementConsoleDocumentSystemComponent::CloseDocument(const AZ::Uuid& documentId)
    {
        bool isOpen = false;
        ShaderManagementConsoleDocumentRequestBus::EventResult(isOpen, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::IsOpen);
        if (!isOpen)
        {
            return true;
        }

        bool isModified = false;
        ShaderManagementConsoleDocumentRequestBus::EventResult(isModified, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::IsModified);
        if (isModified)
        {
            if (QMessageBox::question(QApplication::activeWindow(), "document has unsaved changes", "Would you like to close anyway?",
                QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
            {
                return false;
            }
        }

        ShaderManagementConsoleDocumentResult closeResult = AZ::Success(AZStd::string("There is no active document"));
        ShaderManagementConsoleDocumentRequestBus::EventResult(closeResult, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::Close);
        if (!closeResult)
        {
            QMessageBox::critical(QApplication::activeWindow(), "Failed to close document",
                QString::fromUtf8(closeResult.GetError().data(), (int)closeResult.GetError().size()));
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

    bool ShaderManagementConsoleDocumentSystemComponent::SaveDocument(const AZ::Uuid& documentId)
    {
        AZStd::string documentPath;
        ShaderManagementConsoleDocumentRequestBus::EventResult(documentPath, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetAbsolutePath);

        const QFileInfo saveInfo(documentPath.c_str());
        if (saveInfo.absoluteFilePath().isEmpty())
        {
            return false;
        }

        if (saveInfo.exists() && !saveInfo.isWritable())
        {
            QMessageBox::critical(QApplication::activeWindow(), "Error", QString("Unable to save document. File can not be overwritten."));
            return false;
        }

        ShaderManagementConsoleDocumentResult result = AZ::Failure(AZStd::string("There is no active document"));
        ShaderManagementConsoleDocumentRequestBus::EventResult(result, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::Save);
        if (!result)
        {
            QMessageBox::critical(QApplication::activeWindow(), "document not saved",
                QString::fromUtf8(result.GetError().data(), (int)result.GetError().size()));
            return false;
        }

        AZ_TracePrintf("ShaderManagementConsole", "%s\n", result.GetValue().c_str());
        return true;
    }

    bool ShaderManagementConsoleDocumentSystemComponent::SaveDocumentAsCopy(const AZ::Uuid& documentId)
    {
        AZStd::string documentPath;
        ShaderManagementConsoleDocumentRequestBus::EventResult(documentPath, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetAbsolutePath);

        const QFileInfo& saveInfo = AtomToolsFramework::GetSaveFileInfo(documentPath.c_str());
        if (saveInfo.absoluteFilePath().isEmpty())
        {
            return false;
        }

        AZStd::string saveDocumentPath = saveInfo.absoluteFilePath().toUtf8().constData();
        AzFramework::StringFunc::Path::Normalize(saveDocumentPath);

        ShaderManagementConsoleDocumentResult result = AZ::Failure(AZStd::string("There is no active document"));
        ShaderManagementConsoleDocumentRequestBus::EventResult(result, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::SaveAsCopy, saveDocumentPath);
        if (!result)
        {
            QMessageBox::critical(QApplication::activeWindow(), "document copy not saved",
                QString::fromUtf8(result.GetError().data(), (int)result.GetError().size()));
            return false;
        }

        AZ_TracePrintf("ShaderManagementConsole", "%s\n", result.GetValue().c_str());
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

    AZ::Uuid ShaderManagementConsoleDocumentSystemComponent::OpenDocumentImpl(AZStd::string_view path, bool checkIfAlreadyOpen)
    {
        AZStd::string requestedPath = path;
        if (requestedPath.empty() || !AzFramework::StringFunc::Path::Normalize(requestedPath))
        {
            QMessageBox::critical(QApplication::activeWindow(), "document path is invalid",
                QString::fromUtf8(requestedPath.data(), (int)requestedPath.size()));
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

        AZ::Uuid documentId = AZ::Uuid::CreateNull();
        ShaderManagementConsoleDocumentSystemRequestBus::BroadcastResult(documentId, &ShaderManagementConsoleDocumentSystemRequestBus::Events::CreateDocument);
        if (documentId.IsNull())
        {
            QMessageBox::critical(QApplication::activeWindow(), "Failed to create document",
                QString::fromUtf8(requestedPath.data(), (int)requestedPath.size()));
            return AZ::Uuid::CreateNull();
        }

        ShaderManagementConsoleDocumentResult openResult = AZ::Failure(AZStd::string("Failed to open document"));
        ShaderManagementConsoleDocumentRequestBus::EventResult(openResult, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::Open, requestedPath);
        if (!openResult)
        {
            QMessageBox::critical(QApplication::activeWindow(), "Failed to open document",
                QString::fromUtf8(openResult.GetError().data(), (int)openResult.GetError().size()));
            ShaderManagementConsoleDocumentSystemRequestBus::Broadcast(&ShaderManagementConsoleDocumentSystemRequestBus::Events::DestroyDocument, documentId);
            return AZ::Uuid::CreateNull();
        }

        return documentId;
    }
}
