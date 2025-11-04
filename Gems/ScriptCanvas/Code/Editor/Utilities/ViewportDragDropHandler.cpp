/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ViewportDragDropHandler.h"


#include <QUrl>
#include <QDirIterator>
#include <QMessageBox>

#include <API/EntityCompositionRequestBus.h>
#include <API/ToolsApplicationAPI.h>

#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

#include <AzQtComponents/DragAndDrop/ViewportDragAndDrop.h>

#include <Entity/EditorEntityHelpers.h>

#include <ScriptCanvas/Components/EditorScriptCanvasComponent.h>
#include <AzCore/Utils/Utils.h>

bool ScriptCanvasAssetDragDropHandler::m_dragAccepted;

ScriptCanvasAssetDragDropHandler::ScriptCanvasAssetDragDropHandler()
{
    AzQtComponents::DragAndDropEventsBus::Handler::BusConnect(AzQtComponents::DragAndDropContexts::EditorViewport);
}

ScriptCanvasAssetDragDropHandler::~ScriptCanvasAssetDragDropHandler()
{
    AzQtComponents::DragAndDropEventsBus::Handler::BusDisconnect(AzQtComponents::DragAndDropContexts::EditorViewport);
}

int ScriptCanvasAssetDragDropHandler::GetDragAndDropEventsPriority() const
{
    return AzQtComponents::CommonDragAndDropBusTraits::s_HighPriority;
}

void ScriptCanvasAssetDragDropHandler::DragEnter(QDragEnterEvent* event, AzQtComponents::DragAndDropContextBase&)
{
    m_dragAccepted = false;

    const QMimeData* mimeData = event->mimeData();

    // if the event hasn't been accepted already and the mimeData hasUrls()
    if (event->isAccepted() || !mimeData->hasUrls())
    {
        return;
    }

    QList<QUrl> urlList = mimeData->urls();
    int urlListSize = urlList.size();

    for (int i = 0; i < urlListSize; ++i)
    {
        // Get the local file path
        QString path = urlList.at(i).toLocalFile();

        QDir dir(path);
        QString relativePath = dir.relativeFilePath(path);
        QString absPath = dir.absolutePath();

        // check if the files/folders are under the project root directory
        AZ::IO::FixedMaxPathString projectPath = AZ::Utils::GetProjectPath();

        QDir gameRoot(projectPath.c_str());
        QString gameRootAbsPath = gameRoot.absolutePath();

        QDirIterator it(absPath, QDir::NoDotAndDotDot | QDir::Files, QDirIterator::Subdirectories);

        QFileInfo info(absPath);
        QString extension = info.completeSuffix();

        if (extension.compare("scriptcanvas", Qt::CaseInsensitive) != 0)
        {
            break;
        }

        // if it's not an empty folder directory or if it's a file,
        // then allow the drag and drop process.
        // Otherwise, prevent users from dragging and dropping empty folders
        if (it.hasNext() || !extension.isEmpty())
        {
            // this is used in Drop()
            m_dragAccepted = true;
        }
    }

    // at this point, all files should be legal to be imported
    // since they are not in the database
    if (m_dragAccepted)
    {
        event->acceptProposedAction();
    }
}

void ScriptCanvasAssetDragDropHandler::Drop(QDropEvent* event, AzQtComponents::DragAndDropContextBase& context)
{
    if (!m_dragAccepted)
    {
        return;
    }

    QStringList fileList = GetFileList(event);

    if (!fileList.isEmpty())
    {
        if (AzQtComponents::ViewportDragContext* viewportDragContext = azrtti_cast<AzQtComponents::ViewportDragContext*>(&context))
        {
            AzToolsFramework::EntityIdList createdEntities;
            CreateEntitiesAtPoint(fileList, viewportDragContext->m_hitLocation, AZ::EntityId(), createdEntities);

            event->setAccepted(true);
        }
    }

    // reset
    m_dragAccepted = false;
}

void ScriptCanvasAssetDragDropHandler::CreateEntitiesAtPoint(QStringList fileList, AZ::Vector3 location, AZ::EntityId parentEntityId, AzToolsFramework::EntityIdList& /*createdEntities*/)
{
    if (fileList.isEmpty())
    {
        return;
    }

    for (auto& filePath : fileList)
    {

        AZ::IO::FixedMaxPathString projectPath = AZ::Utils::GetProjectPath();
        QDir gameRoot(projectPath.c_str());
        QString relativePath = gameRoot.relativeFilePath(filePath);

        AzToolsFramework::ScopedUndoBatch undo("Create entities from assets");

        QWidget* mainWindow = nullptr;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(mainWindow, &AzToolsFramework::EditorRequests::GetMainWindow);

        const AZ::Transform worldTransform = AZ::Transform::CreateTranslation(location);

        struct AssetIdAndComponentTypeId
        {
            AssetIdAndComponentTypeId(AZ::Uuid componentTypeId, AZ::Data::AssetId assetId)
                : m_componentTypeId(componentTypeId)
                , m_assetId(assetId)
            {
            }
            AZ::Uuid m_componentTypeId = AZ::Uuid::CreateNull();
            AZ::Data::AssetId m_assetId;
        };

        AZ::EntityId targetEntityId;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(
            targetEntityId, &AzToolsFramework::EditorRequests::CreateNewEntityAtPosition, worldTransform.GetTranslation(), parentEntityId);

        AZ::Entity* newEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(newEntity, &AZ::ComponentApplicationRequests::FindEntity, targetEntityId);

        if (newEntity == nullptr)
        {
            QMessageBox::warning(
                mainWindow,
                QObject::tr("Asset Drop Failed"),
                QStringLiteral("Could not create entity from selected asset(s)."));
            return;
        }

        // Deactivate the entity so the properties on the components can be set.
        newEntity->Deactivate();
        newEntity->SetName("Script Canvas");

        auto scriptCanvasAssetType = azrtti_typeid<ScriptCanvas::RuntimeAsset>();

        AZ::Uuid componentTypeId = AZ::Uuid::CreateNull();
        AZ::AssetTypeInfoBus::EventResult(componentTypeId, scriptCanvasAssetType, &AZ::AssetTypeInfo::GetComponentTypeId);

        auto scriptCanvsaComponentType = azrtti_typeid<ScriptCanvasEditor::EditorScriptCanvasComponent>();

        AZ::ComponentTypeList componentsToAdd;
        componentsToAdd.push_back(scriptCanvsaComponentType);

        // ScriptCanvas uses the source asset reference in its component
        bool sourceAssetInfoFound = false;
        AZ::Data::AssetInfo sourceAssetInfo;
        AZStd::string watchFolder;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(sourceAssetInfoFound,
            &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, filePath.toStdString().c_str(), sourceAssetInfo, watchFolder);

        AZStd::vector<AZ::EntityId> entityIds = { targetEntityId };

        AzToolsFramework::EntityCompositionRequests::AddComponentsOutcome addComponentsOutcome = AZ::Failure(AZStd::string());
        AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(
            addComponentsOutcome, &AzToolsFramework::EntityCompositionRequests::AddComponentsToEntities, entityIds, componentsToAdd);

        if (!addComponentsOutcome.IsSuccess())
        {
            AZ_Error(
                "AssetBrowser",
                false,
                "Could not create the requested components from the selected assets: %s",
                addComponentsOutcome.GetError().c_str());
            AzToolsFramework::EditorEntityContextRequestBus::Broadcast(&AzToolsFramework::EditorEntityContextRequests::DestroyEditorEntity, targetEntityId);
            return;
        }

        // activate the entity first, so that the primary asset change is done in the context of it being awake.
        newEntity->Activate();

        if (AZ::Component* componentAdded = newEntity->FindComponent(scriptCanvsaComponentType))
        {
            AzToolsFramework::Components::EditorComponentBase* editorComponent = AzToolsFramework::GetEditorComponent(componentAdded);
            if (editorComponent)
            {
                editorComponent->SetPrimaryAsset(sourceAssetInfo.m_assetId);
            }
        }
    }
}

QStringList ScriptCanvasAssetDragDropHandler::GetFileList(QDropEvent* event)
{
    QStringList fileList;
    const QMimeData* mimeData = event->mimeData();

    QList<QUrl> urlList = mimeData->urls();

    for (int i = 0; i < urlList.size(); ++i)
    {
        QUrl currentUrl = urlList.at(i);

        if (currentUrl.isLocalFile())
        {
            QString path = urlList.at(i).toLocalFile();

            fileList.append(path);
        }
    }

    return fileList;
}
