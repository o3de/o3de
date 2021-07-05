/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Document/MaterialDocumentSystemComponent.h>

#include <Atom/Document/MaterialDocumentNotificationBus.h>
#include <Atom/Document/MaterialDocumentRequestBus.h>
#include <Atom/Document/MaterialDocumentSettings.h>
#include <Atom/Document/MaterialDocumentSystemRequestBus.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <AtomToolsFramework/Debug/TraceRecorder.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QString>
#include <QStyle>
AZ_POP_DISABLE_WARNING

namespace MaterialEditor
{
    MaterialDocumentSystemComponent::MaterialDocumentSystemComponent()
    {
    }

    void MaterialDocumentSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        MaterialDocumentSettings::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MaterialDocumentSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<MaterialDocumentSystemComponent>("MaterialDocumentSystemComponent", "Tool for editing Atom material files")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<MaterialDocumentSystemRequestBus>("MaterialDocumentSystemRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "materialeditor")
                ->Event("CreateDocument", &MaterialDocumentSystemRequestBus::Events::CreateDocument)
                ->Event("DestroyDocument", &MaterialDocumentSystemRequestBus::Events::DestroyDocument)
                ->Event("OpenDocument", &MaterialDocumentSystemRequestBus::Events::OpenDocument)
                ->Event("CreateDocumentFromFile", &MaterialDocumentSystemRequestBus::Events::CreateDocumentFromFile)
                ->Event("CloseDocument", &MaterialDocumentSystemRequestBus::Events::CloseDocument)
                ->Event("CloseAllDocuments", &MaterialDocumentSystemRequestBus::Events::CloseAllDocuments)
                ->Event("CloseAllDocumentsExcept", &MaterialDocumentSystemRequestBus::Events::CloseAllDocumentsExcept)
                ->Event("SaveDocument", &MaterialDocumentSystemRequestBus::Events::SaveDocument)
                ->Event("SaveDocumentAsCopy", &MaterialDocumentSystemRequestBus::Events::SaveDocumentAsCopy)
                ->Event("SaveDocumentAsChild", &MaterialDocumentSystemRequestBus::Events::SaveDocumentAsChild)
                ->Event("SaveAllDocuments", &MaterialDocumentSystemRequestBus::Events::SaveAllDocuments)
                ;

            behaviorContext->EBus<MaterialDocumentRequestBus>("MaterialDocumentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "materialeditor")
                ->Event("GetAbsolutePath", &MaterialDocumentRequestBus::Events::GetAbsolutePath)
                ->Event("GetRelativePath", &MaterialDocumentRequestBus::Events::GetRelativePath)
                ->Event("GetPropertyValue", &MaterialDocumentRequestBus::Events::GetPropertyValue)
                ->Event("SetPropertyValue", &MaterialDocumentRequestBus::Events::SetPropertyValue)
                ->Event("Open", &MaterialDocumentRequestBus::Events::Open)
                ->Event("Rebuild", &MaterialDocumentRequestBus::Events::Rebuild)
                ->Event("Close", &MaterialDocumentRequestBus::Events::Close)
                ->Event("Save", &MaterialDocumentRequestBus::Events::Save)
                ->Event("SaveAsChild", &MaterialDocumentRequestBus::Events::SaveAsChild)
                ->Event("SaveAsCopy", &MaterialDocumentRequestBus::Events::SaveAsCopy)
                ->Event("IsOpen", &MaterialDocumentRequestBus::Events::IsOpen)
                ->Event("IsModified", &MaterialDocumentRequestBus::Events::IsModified)
                ->Event("IsSavable", &MaterialDocumentRequestBus::Events::IsSavable)
                ->Event("CanUndo", &MaterialDocumentRequestBus::Events::CanUndo)
                ->Event("CanRedo", &MaterialDocumentRequestBus::Events::CanRedo)
                ->Event("Undo", &MaterialDocumentRequestBus::Events::Undo)
                ->Event("Redo", &MaterialDocumentRequestBus::Events::Redo)
                ->Event("BeginEdit", &MaterialDocumentRequestBus::Events::BeginEdit)
                ->Event("EndEdit", &MaterialDocumentRequestBus::Events::EndEdit)
                ;
        }
    }

    void MaterialDocumentSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("AssetProcessorToolsConnection", 0x734669bc));
        required.push_back(AZ_CRC("AssetDatabaseService", 0x3abf5601));
        required.push_back(AZ_CRC("PropertyManagerService", 0x63a3d7ad));
        required.push_back(AZ_CRC("RPISystem", 0xf2add773));
    }

    void MaterialDocumentSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("MaterialDocumentSystemService"));
    }

    void MaterialDocumentSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("MaterialDocumentSystemService"));
    }

    void MaterialDocumentSystemComponent::Init()
    {
    }

    void MaterialDocumentSystemComponent::Activate()
    {
        m_documentMap.clear();
        m_settings = AZ::UserSettings::CreateFind<MaterialDocumentSettings>(AZ::Crc32("MaterialDocumentSettings"), AZ::UserSettings::CT_GLOBAL);
        MaterialDocumentSystemRequestBus::Handler::BusConnect();
        MaterialDocumentNotificationBus::Handler::BusConnect();
    }

    void MaterialDocumentSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        MaterialDocumentNotificationBus::Handler::BusDisconnect();
        MaterialDocumentSystemRequestBus::Handler::BusDisconnect();
        m_documentMap.clear();
    }

    AZ::Uuid MaterialDocumentSystemComponent::CreateDocument()
    {
        auto document = AZStd::make_unique<MaterialDocument>();
        if (!document)
        {
            AZ_Error("MaterialDocument", false, "Failed to create new document");
            return AZ::Uuid::CreateNull();
        }

        AZ::Uuid documentId = document->GetId();
        m_documentMap.emplace(documentId, document.release());
        return documentId;
    }

    bool MaterialDocumentSystemComponent::DestroyDocument(const AZ::Uuid& documentId)
    {
        return m_documentMap.erase(documentId) != 0;
    }

    void MaterialDocumentSystemComponent::OnDocumentExternallyModified(const AZ::Uuid& documentId)
    {
        m_documentIdsToReopen.insert(documentId);
        if (!AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void MaterialDocumentSystemComponent::OnDocumentDependencyModified(const AZ::Uuid& documentId)
    {
        m_documentIdsToRebuild.insert(documentId);
        if (!AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void MaterialDocumentSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        for (const AZ::Uuid& documentId : m_documentIdsToReopen)
        {
            AZStd::string documentPath;
            MaterialDocumentRequestBus::EventResult(documentPath, documentId, &MaterialDocumentRequestBus::Events::GetAbsolutePath);

            if (m_settings->m_showReloadDocumentPrompt &&
                (QMessageBox::question(QApplication::activeWindow(),
                QString("Material document was externally modified"),
                QString("Would you like to reopen the document:\n%1?").arg(documentPath.c_str()),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes))
            {
                continue;
            }

            AtomToolsFramework::TraceRecorder traceRecorder(m_maxMessageBoxLineCount);

            bool openResult = false;
            MaterialDocumentRequestBus::EventResult(openResult, documentId, &MaterialDocumentRequestBus::Events::Open, documentPath);
            if (!openResult)
            {
                QMessageBox::critical(
                    QApplication::activeWindow(), QString("Material document could not be opened"),
                    QString("Failed to open: \n%1\n\n%2").arg(documentPath.c_str()).arg(traceRecorder.GetDump().c_str()));
                MaterialDocumentSystemRequestBus::Broadcast(&MaterialDocumentSystemRequestBus::Events::CloseDocument, documentId);
            }
        }

        for (const AZ::Uuid& documentId : m_documentIdsToRebuild)
        {
            AZStd::string documentPath;
            MaterialDocumentRequestBus::EventResult(documentPath, documentId, &MaterialDocumentRequestBus::Events::GetAbsolutePath);

            if (m_settings->m_showReloadDocumentPrompt &&
                (QMessageBox::question(QApplication::activeWindow(),
                QString("Material document dependencies have changed"),
                QString("Would you like to update the document with these changes:\n%1?").arg(documentPath.c_str()),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes))
            {
                continue;
            }

            AtomToolsFramework::TraceRecorder traceRecorder(m_maxMessageBoxLineCount);

            bool openResult = false;
            MaterialDocumentRequestBus::EventResult(openResult, documentId, &MaterialDocumentRequestBus::Events::Rebuild);
            if (!openResult)
            {
                QMessageBox::critical(
                    QApplication::activeWindow(), QString("Material document could not be opened"),
                    QString("Failed to open: \n%1\n\n%2").arg(documentPath.c_str()).arg(traceRecorder.GetDump().c_str()));
                MaterialDocumentSystemRequestBus::Broadcast(&MaterialDocumentSystemRequestBus::Events::CloseDocument, documentId);
            }
        }

        m_documentIdsToRebuild.clear();
        m_documentIdsToReopen.clear();
        AZ::TickBus::Handler::BusDisconnect();
    }

    AZ::Uuid MaterialDocumentSystemComponent::OpenDocument(AZStd::string_view sourcePath)
    {
        return OpenDocumentImpl(sourcePath, true);
    }

    AZ::Uuid MaterialDocumentSystemComponent::CreateDocumentFromFile(AZStd::string_view sourcePath, AZStd::string_view targetPath)
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

        // Send document open notification after creating new material
        MaterialDocumentNotificationBus::Broadcast(&MaterialDocumentNotificationBus::Events::OnDocumentOpened, documentId);
        return documentId;
    }

    bool MaterialDocumentSystemComponent::CloseDocument(const AZ::Uuid& documentId)
    {
        bool isOpen = false;
        MaterialDocumentRequestBus::EventResult(isOpen, documentId, &MaterialDocumentRequestBus::Events::IsOpen);
        if (!isOpen)
        {
            // immediately destroy unopened documents
            MaterialDocumentSystemRequestBus::Broadcast(&MaterialDocumentSystemRequestBus::Events::DestroyDocument, documentId);
            return true;
        }

        AZStd::string documentPath;
        MaterialDocumentRequestBus::EventResult(documentPath, documentId, &MaterialDocumentRequestBus::Events::GetAbsolutePath);

        bool isModified = false;
        MaterialDocumentRequestBus::EventResult(isModified, documentId, &MaterialDocumentRequestBus::Events::IsModified);
        if (isModified)
        {
            auto selection = QMessageBox::question(QApplication::activeWindow(),
                QString("Material document has unsaved changes"),
                QString("Do you want to save changes to\n%1?").arg(documentPath.c_str()),
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            if (selection == QMessageBox::Cancel)
            {
                AZ_TracePrintf("MaterialDocument", "Close document canceled: %s", documentPath.c_str());
                return false;
            }
            if (selection == QMessageBox::Yes)
            {
                if (!SaveDocument(documentId))
                {
                    AZ_Error("MaterialDocument", false, "Close document failed because document was not saved: %s", documentPath.c_str());
                    return false;
                }
            }
        }

        AtomToolsFramework::TraceRecorder traceRecorder(m_maxMessageBoxLineCount);

        bool closeResult = true;
        MaterialDocumentRequestBus::EventResult(closeResult, documentId, &MaterialDocumentRequestBus::Events::Close);
        if (!closeResult)
        {
            QMessageBox::critical(
                QApplication::activeWindow(), QString("Material document could not be closed"),
                QString("Failed to close: \n%1\n\n%2").arg(documentPath.c_str()).arg(traceRecorder.GetDump().c_str()));
            return false;
        }

        MaterialDocumentSystemRequestBus::Broadcast(&MaterialDocumentSystemRequestBus::Events::DestroyDocument, documentId);
        return true;
    }

    bool MaterialDocumentSystemComponent::CloseAllDocuments()
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

    bool MaterialDocumentSystemComponent::CloseAllDocumentsExcept(const AZ::Uuid& documentId)
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

    bool MaterialDocumentSystemComponent::SaveDocument(const AZ::Uuid& documentId)
    {
        AZStd::string saveMaterialPath;
        MaterialDocumentRequestBus::EventResult(saveMaterialPath, documentId, &MaterialDocumentRequestBus::Events::GetAbsolutePath);

        if (saveMaterialPath.empty() || !AzFramework::StringFunc::Path::Normalize(saveMaterialPath))
        {
            return false;
        }

        const QFileInfo saveInfo(saveMaterialPath.c_str());
        if (saveInfo.exists() && !saveInfo.isWritable())
        {
            QMessageBox::critical(QApplication::activeWindow(), "Error", QString("Material document could not be overwritten:\n%1").arg(saveMaterialPath.c_str()));
            return false;
        }

        AtomToolsFramework::TraceRecorder traceRecorder(m_maxMessageBoxLineCount);

        bool result = false;
        MaterialDocumentRequestBus::EventResult(result, documentId, &MaterialDocumentRequestBus::Events::Save);
        if (!result)
        {
            QMessageBox::critical(
                QApplication::activeWindow(), QString("Material document could not be saved"),
                QString("Failed to save: \n%1\n\n%2").arg(saveMaterialPath.c_str()).arg(traceRecorder.GetDump().c_str()));
            return false;
        }

        return true;
    }

    bool MaterialDocumentSystemComponent::SaveDocumentAsCopy(const AZ::Uuid& documentId, AZStd::string_view targetPath)
    {
        AZStd::string saveMaterialPath = targetPath;
        if (saveMaterialPath.empty() || !AzFramework::StringFunc::Path::Normalize(saveMaterialPath))
        {
            return false;
        }

        const QFileInfo saveInfo(saveMaterialPath.c_str());
        if (saveInfo.exists() && !saveInfo.isWritable())
        {
            QMessageBox::critical(QApplication::activeWindow(), "Error", QString("Material document could not be overwritten:\n%1").arg(saveMaterialPath.c_str()));
            return false;
        }

        AtomToolsFramework::TraceRecorder traceRecorder(m_maxMessageBoxLineCount);

        bool result = false;
        MaterialDocumentRequestBus::EventResult(result, documentId, &MaterialDocumentRequestBus::Events::SaveAsCopy, saveMaterialPath);
        if (!result)
        {
            QMessageBox::critical(
                QApplication::activeWindow(), QString("Material document could not be saved"),
                QString("Failed to save: \n%1\n\n%2").arg(saveMaterialPath.c_str()).arg(traceRecorder.GetDump().c_str()));
            return false;
        }

        return true;
    }

    bool MaterialDocumentSystemComponent::SaveDocumentAsChild(const AZ::Uuid& documentId, AZStd::string_view targetPath)
    {
        AZStd::string saveMaterialPath = targetPath;
        if (saveMaterialPath.empty() || !AzFramework::StringFunc::Path::Normalize(saveMaterialPath))
        {
            return false;
        }

        const QFileInfo saveInfo(saveMaterialPath.c_str());
        if (saveInfo.exists() && !saveInfo.isWritable())
        {
            QMessageBox::critical(QApplication::activeWindow(), "Error", QString("Material document could not be overwritten:\n%1").arg(saveMaterialPath.c_str()));
            return false;
        }

        AtomToolsFramework::TraceRecorder traceRecorder(m_maxMessageBoxLineCount);

        bool result = false;
        MaterialDocumentRequestBus::EventResult(result, documentId, &MaterialDocumentRequestBus::Events::SaveAsChild, saveMaterialPath);
        if (!result)
        {
            QMessageBox::critical(
                QApplication::activeWindow(), QString("Material document could not be saved"),
                QString("Failed to save: \n%1\n\n%2").arg(saveMaterialPath.c_str()).arg(traceRecorder.GetDump().c_str()));
            return false;
        }

        return true;
    }

    bool MaterialDocumentSystemComponent::SaveAllDocuments()
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

    AZ::Uuid MaterialDocumentSystemComponent::OpenDocumentImpl(AZStd::string_view sourcePath, bool checkIfAlreadyOpen)
    {
        AZStd::string requestedPath = sourcePath;
        if (requestedPath.empty())
        {
            return AZ::Uuid::CreateNull();
        }

        if (!AzFramework::StringFunc::Path::Normalize(requestedPath))
        {
            QMessageBox::critical(QApplication::activeWindow(), "Error", QString("Material document path is invalid:\n%1").arg(requestedPath.c_str()));
            return AZ::Uuid::CreateNull();
        }

        // Determine if the file is already open and select it
        if (checkIfAlreadyOpen)
        {
            for (const auto& documentPair : m_documentMap)
            {
                AZStd::string openMaterialPath;
                MaterialDocumentRequestBus::EventResult(openMaterialPath, documentPair.first, &MaterialDocumentRequestBus::Events::GetAbsolutePath);
                if (openMaterialPath == requestedPath)
                {
                    MaterialDocumentNotificationBus::Broadcast(&MaterialDocumentNotificationBus::Events::OnDocumentOpened, documentPair.first);
                    return documentPair.first;
                }
            }
        }

        AtomToolsFramework::TraceRecorder traceRecorder(m_maxMessageBoxLineCount);

        AZ::Uuid documentId = AZ::Uuid::CreateNull();
        MaterialDocumentSystemRequestBus::BroadcastResult(documentId, &MaterialDocumentSystemRequestBus::Events::CreateDocument);
        if (documentId.IsNull())
        {
            QMessageBox::critical(
                QApplication::activeWindow(), QString("Material document could not be created"),
                QString("Failed to create: \n%1\n\n%2").arg(requestedPath.c_str()).arg(traceRecorder.GetDump().c_str()));
            return AZ::Uuid::CreateNull();
        }

        traceRecorder.GetDump().clear();

        bool openResult = false;
        MaterialDocumentRequestBus::EventResult(openResult, documentId, &MaterialDocumentRequestBus::Events::Open, requestedPath);
        if (!openResult)
        {
            QMessageBox::critical(
                QApplication::activeWindow(), QString("Material document could not be opened"),
                QString("Failed to open: \n%1\n\n%2").arg(requestedPath.c_str()).arg(traceRecorder.GetDump().c_str()));
            MaterialDocumentSystemRequestBus::Broadcast(&MaterialDocumentSystemRequestBus::Events::DestroyDocument, documentId);
            return AZ::Uuid::CreateNull();
        }

        return documentId;
    }
}
