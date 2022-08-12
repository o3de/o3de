
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "AzAssetBrowserRequestHandler.h"

// Qt
#include <QDesktopServices>
#include <QMenu>
#include <QObject>
#include <QString>

// AzCore
#include <AzCore/std/string/wildcard.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>

// AzFramework
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/GenericAssetHandler.h>

// AzToolsFramework
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserSourceDropBus.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeView.h>
#include <AzToolsFramework/AssetEditor/AssetEditorBus.h>
#include <AzToolsFramework/Commands/EntityStateCommand.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/SliceEditorEntityOwnershipServiceBus.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorLayerComponent.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/UI/Slice/SliceRelationshipBus.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

// AzQtComponents
#include <AzQtComponents/DragAndDrop/ViewportDragAndDrop.h>

// Editor
#include "IEditor.h"
#include "Include/IObjectManager.h"
#include "CryEditDoc.h"
#include "QtViewPaneManager.h"

namespace AzAssetBrowserRequestHandlerPrivate
{
    using namespace AzToolsFramework;
    using namespace AzToolsFramework::AssetBrowser;
    // return true ONLY if we can handle the drop request in the viewport.
    bool CanSpawnEntityForProduct(const ProductAssetBrowserEntry* product,
        AZStd::optional<const AZStd::vector<AZ::Data::AssetType>> optionalProductAssetTypes = AZStd::nullopt)
    {
        if (!product)
        {
            return false;
        }

        if (product->GetAssetType() == AZ::AzTypeInfo<AZ::SliceAsset>::Uuid())
        {
            return true; // we can always spawn slices.
        }

        bool canCreateComponent = false;
        AZ::AssetTypeInfoBus::EventResult(canCreateComponent, product->GetAssetType(), &AZ::AssetTypeInfo::CanCreateComponent, product->GetAssetId());
        if (!canCreateComponent)
        {
            return false;
        }

        AZ::Uuid componentTypeId = AZ::Uuid::CreateNull();
        AZ::AssetTypeInfoBus::EventResult(componentTypeId, product->GetAssetType(), &AZ::AssetTypeInfo::GetComponentTypeId);
        if (componentTypeId.IsNull())
        {
            // we have a component type that handles this asset.
            return false;
        }

        if (optionalProductAssetTypes.has_value())
        {
            bool hasConflictingProducts = false;
            AZ::AssetTypeInfoBus::EventResult(hasConflictingProducts, product->GetAssetType(), &AZ::AssetTypeInfo::HasConflictingProducts, optionalProductAssetTypes.value());
            if (hasConflictingProducts)
            {
                return false;
            }
        }

        // additional operations can be added here.

        return true;
    }

    void SpawnEntityAtPoint(
        AZStd::vector<const ProductAssetBrowserEntry*> products,
        AzQtComponents::ViewportDragContext* viewportDragContext,
        EntityIdList& spawnList,
        AzFramework::SliceInstantiationTicket& spawnTicket)
    {
        if ((!viewportDragContext) || products.empty())
        {
            return;
        }

        QWidget* mainWindow = nullptr;
        EditorRequests::Bus::BroadcastResult(mainWindow, &EditorRequests::GetMainWindow);

        const AZ::Transform worldTransform = AZ::Transform::CreateTranslation(viewportDragContext->m_hitLocation);

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
        AZStd::vector<AssetIdAndComponentTypeId> assetIdAndComponentTypeId;
        AZ::ComponentTypeList componentsToAdd;

        for (const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* product : products)
        {
            // Handle instantiation of slices.
            if (product->GetAssetType() == AZ::AzTypeInfo<AZ::SliceAsset>::Uuid())
            {
                // Instantiate the slice at the specified location.
                AZ::Data::Asset<AZ::SliceAsset> asset = AZ::Data::AssetManager::Instance().FindOrCreateAsset<AZ::SliceAsset>(
                    product->GetAssetId(), AZ::Data::AssetLoadBehavior::Default);
                if (asset)
                {
                    SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(
                        spawnTicket, &SliceEditorEntityOwnershipServiceRequests::InstantiateEditorSlice, asset, worldTransform);
                }
            }
            else
            {
                AZ::Uuid componentTypeId = AZ::Uuid::CreateNull();
                AZ::AssetTypeInfoBus::EventResult(componentTypeId, product->GetAssetType(), &AZ::AssetTypeInfo::GetComponentTypeId);
                if (!componentTypeId.IsNull())
                {
                    assetIdAndComponentTypeId.push_back(AssetIdAndComponentTypeId(componentTypeId,product->GetAssetId()));

                    componentsToAdd.push_back(componentTypeId);
                }
            }
        }

        if (assetIdAndComponentTypeId.empty())
        {
            return;
        }

        ScopedUndoBatch undo("Create entities from asset");


        AZStd::string entityName;

        // If the entity is being created from an asset, name it after said asset.
        // If there is more than one product, use the first to generate the name.
        AZStd::string assetPath;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            assetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, assetIdAndComponentTypeId[0].m_assetId);
        if (!assetPath.empty())
        {
            AzFramework::StringFunc::Path::GetFileName(assetPath.c_str(), entityName);
        }

        // If not sourced from an asset, generate a generic name.
        if (entityName.empty())
        {
            entityName = AZStd::string::format("Entity%d", GetIEditor()->GetObjectManager()->GetObjectCount());
        }

        AZ::EntityId targetEntityId;
        EditorRequests::Bus::BroadcastResult(targetEntityId, &EditorRequests::CreateNewEntityAtPosition, worldTransform.GetTranslation(), AZ::EntityId());

        AZ::Entity* newEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(newEntity, &AZ::ComponentApplicationRequests::FindEntity, targetEntityId);

        if (newEntity == nullptr)
        {
            QMessageBox::warning(mainWindow, QObject::tr("Asset Drop Failed"), QStringLiteral("Could not create entity from selected asset(s)."));
            return;
        }

        // Deactivate the entity so the properties on the components can be set.
        newEntity->Deactivate();

        newEntity->SetName(entityName);


        // Add all dropped products as components to this entity.
        AZStd::vector<AZ::EntityId> entityIds = { targetEntityId };
        EntityCompositionRequests::AddComponentsOutcome addComponentsOutcome = AZ::Failure(AZStd::string());
        EntityCompositionRequestBus::BroadcastResult(
            addComponentsOutcome, &EntityCompositionRequests::AddComponentsToEntities, entityIds, componentsToAdd);

        if (!addComponentsOutcome.IsSuccess())
        {
            QMessageBox::warning(mainWindow, QObject::tr("Asset Drop Failed"), QStringLiteral("Could not create components for entity from selected asset(s)."));

            EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::DestroyEditorEntity, targetEntityId);
            return;
        }

        
        for (const AssetIdAndComponentTypeId& assetAndComponent : assetIdAndComponentTypeId)
        {
            AZ::Component* componentAdded = newEntity->FindComponent(assetAndComponent.m_componentTypeId);
            if (componentAdded)
            {
                Components::EditorComponentBase* editorComponent = GetEditorComponent(componentAdded);
                if (editorComponent)
                {
                    editorComponent->SetPrimaryAsset(assetAndComponent.m_assetId);
                }
            }
        }

        newEntity->Activate();

        bool isPrefabSystemEnabled = false;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(
            isPrefabSystemEnabled, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);

        if (!isPrefabSystemEnabled)
        {
            // Prepare undo command last so it captures the final state of the entity.
            EntityCreateCommand* command = aznew EntityCreateCommand(static_cast<AZ::u64>(newEntity->GetId()));
            command->Capture(newEntity);
            command->SetParent(undo.GetUndoBatch());
        }

        ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::AddDirtyEntity, newEntity->GetId());
        spawnList.push_back(newEntity->GetId());
    }
}

AzAssetBrowserRequestHandler::AzAssetBrowserRequestHandler()
{
    using namespace AzToolsFramework::AssetBrowser;

    AssetBrowserInteractionNotificationBus::Handler::BusConnect();
    AzQtComponents::DragAndDropEventsBus::Handler::BusConnect(AzQtComponents::DragAndDropContexts::EditorViewport);
}

AzAssetBrowserRequestHandler::~AzAssetBrowserRequestHandler()
{
    AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
    AzQtComponents::DragAndDropEventsBus::Handler::BusDisconnect();
}

void AzAssetBrowserRequestHandler::AddCreateMenu(QMenu* menu, AZStd::string fullFilePath)
{
    using namespace AzToolsFramework::AssetBrowser;

    AZStd::string folderPath;

    AzFramework::StringFunc::Path::GetFolderPath(fullFilePath.c_str(), folderPath);

    AZ::Uuid sourceID = AZ::Uuid::CreateNull();
    SourceFileCreatorList creators;
    AssetBrowserInteractionNotificationBus::Broadcast(
        &AssetBrowserInteractionNotificationBus::Events::AddSourceFileCreators, folderPath.c_str(), sourceID, creators);
    if (!creators.empty())
    {
        QMenu* createMenu = menu->addMenu(QObject::tr("Create"));
        for (const SourceFileCreatorDetails& creatorDetails : creators)
        {
            if (creatorDetails.m_creator)
            {
                createMenu->addAction(creatorDetails.m_iconToUse, QObject::tr(creatorDetails.m_displayText.c_str()), [sourceID, fullFilePath, creatorDetails]()
                {
                    creatorDetails.m_creator(fullFilePath.c_str(), sourceID);
                });
            }
        }
    }
}

void AzAssetBrowserRequestHandler::AddContextMenuActions(QWidget* caller, QMenu* menu, const AZStd::vector<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>& entries)
{
    using namespace AzToolsFramework::AssetBrowser;

    AssetBrowserTreeView* treeView = qobject_cast<AssetBrowserTreeView*> (caller);
    if (!treeView)
    {
        return;
    }

    AssetBrowserEntry* entry = entries.empty() ? nullptr : entries.front();
    if (!entry)
    {
        return;
    }
    bool calledFromAssetBrowser = treeView->GetName() == "AssetBrowserTreeView_main" ? true : false;
    size_t numOfEntries = entries.size();

    AZStd::string fullFilePath;
    AZStd::string extension;

    switch (entry->GetEntryType())
    {
    case AssetBrowserEntry::AssetEntryType::Product:
        // if its a product, we actually want to perform these operations on the source
        // which will be the parent of the product.
        entry = entry->GetParent();
        if ((!entry) || (entry->GetEntryType() != AssetBrowserEntry::AssetEntryType::Source))
        {
            AZ_Assert(false, "Asset Browser entry product has a non-source parent?");
            break;     // no valid parent.
        }
    // the fall through to the next case is intentional here.
    case AssetBrowserEntry::AssetEntryType::Source:
    {
        AZ::Uuid sourceID = azrtti_cast<SourceAssetBrowserEntry*>(entry)->GetSourceUuid();
        fullFilePath = entry->GetFullPath();
        AzFramework::StringFunc::Path::GetExtension(fullFilePath.c_str(), extension);

        // Context menu entries that only make sense when there is only one selection should go in here
        // For example, open and rename
        if (numOfEntries == 1)
        {
            // Add the "Open" menu item.
            // Note that source file openers are allowed to "veto" the showing of the "Open" menu if it is 100% known that they aren't
            // openable! for example, custom data formats that are made by Open 3D Engine that can not have a program associated in the
            // operating system to view them. If the only opener that can open that file has no m_opener, then it is not openable.
            SourceFileOpenerList openers;
            AssetBrowserInteractionNotificationBus::Broadcast(
                &AssetBrowserInteractionNotificationBus::Events::AddSourceFileOpeners, fullFilePath.c_str(), sourceID, openers);
            bool validOpenersFound = false;
            bool vetoOpenerFound = false;
            for (const SourceFileOpenerDetails& openerDetails : openers)
            {
                if (openerDetails.m_opener)
                {
                    // we found a valid opener (non-null).  This means that the system is saying that it knows how to internally
                    // edit this source file and has a custom editor for it.
                    validOpenersFound = true;
                }
                else
                {
                    // if we get here it means someone intentionally registered a callback with a null function pointer
                    // the API treats this as a 'veto' opener - meaning that the system wants us NOT to allow the operating system
                    // to open this source file as a default fallback.
                    vetoOpenerFound = true;
                }
            }

            if (validOpenersFound)
            {
                // if we get here then there is an opener installed for this kind of asset
                // and it is not null, meaning that it is not vetoing our ability to open the file.
                for (const SourceFileOpenerDetails& openerDetails : openers)
                {
                    // bind that function to the current loop element.
                    if (openerDetails.m_opener) // only VALID openers with an actual callback.
                    {
                        menu->addAction(
                            openerDetails.m_iconToUse, QObject::tr(openerDetails.m_displayText.c_str()),
                            [sourceID, fullFilePath, openerDetails]()
                            {
                                openerDetails.m_opener(fullFilePath.c_str(), sourceID);
                            });
                    }
                }
            }

            // we always add the default "open with your operating system" unless a veto opener is found
            if (!vetoOpenerFound)
            {
                // if we found no valid openers and no veto openers then just allow it to be opened with the operating system itself.
                menu->addAction(
                    QObject::tr("Open with associated application..."),
                    [fullFilePath]()
                    {
                        OpenWithOS(fullFilePath);
                    });
            }

            AZStd::vector<const ProductAssetBrowserEntry*> products;
            entry->GetChildrenRecursively<ProductAssetBrowserEntry>(products);

            // slice source files need to react by adding additional menu items, regardless of status of compile or presence of products.
            if (AzFramework::StringFunc::Equal(extension.c_str(), AzToolsFramework::SliceUtilities::GetSliceFileExtension().c_str(), false))
            {
                AzToolsFramework::SliceUtilities::CreateSliceAssetContextMenu(menu, fullFilePath);

                // SliceUtilities is in AZToolsFramework and can't open viewports, so add the relationship view open command here.
                if (!products.empty())
                {
                    const ProductAssetBrowserEntry* productEntry = products[0];
                    menu->addAction(
                        "Open in Slice Relationship View",
                        [productEntry]()
                        {
                            QtViewPaneManager::instance()->OpenPane(LyViewPane::SliceRelationships);

                            const ProductAssetBrowserEntry* product = azrtti_cast<const ProductAssetBrowserEntry*>(productEntry);

                            AzToolsFramework::SliceRelationshipRequestBus::Broadcast(
                                &AzToolsFramework::SliceRelationshipRequests::OnSliceRelationshipViewRequested, product->GetAssetId());
                        });
                }
            }
            else if (AzFramework::StringFunc::Equal(
                         extension.c_str(), AzToolsFramework::Layers::EditorLayerComponent::GetLayerExtensionWithDot().c_str(), false))
            {
                QString levelPath = Path::GetPath(GetIEditor()->GetDocument()->GetActivePathName());
                AzToolsFramework::Layers::EditorLayerComponent::CreateLayerAssetContextMenu(menu, fullFilePath, levelPath);
            }

            if (!products.empty() || (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source))
            {
                CFileUtil::PopulateQMenu(caller, menu, fullFilePath);
            }
            if (calledFromAssetBrowser)
            {
                // Add Rename option
                QAction* action = menu->addAction(
                    QObject::tr("Rename asset"),
                    [treeView]()
                    {
                        treeView->RenameEntry();
                    });
                action->setShortcut(Qt::Key_F2);
                action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
            }
        }

        if (calledFromAssetBrowser)
        {
            // Add Delete option
            QAction* action = menu->addAction(
                QObject::tr("Delete asset%1").arg(numOfEntries > 1 ? "s" : ""),
                [treeView]()
                {
                    treeView->DeleteEntries();
                });
            action->setShortcut(QKeySequence::Delete);
            action->setShortcutContext(Qt::WidgetWithChildrenShortcut);

            // Add Duplicate option
            action = menu->addAction(
                QObject::tr("Duplicate asset"),
                [treeView]()
                {
                    treeView->DuplicateEntries();
                });
            action->setShortcut(QKeySequence("Ctrl+D"));
            action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        }
    }
    break;
    case AssetBrowserEntry::AssetEntryType::Folder:
    {
        fullFilePath = entry->GetFullPath();

        CFileUtil::PopulateQMenu(caller, menu, fullFilePath);

        AddCreateMenu(menu, fullFilePath);
    }
    break;
    default:
        break;
    }
}

bool AzAssetBrowserRequestHandler::CanAcceptDragAndDropEvent(
    QDropEvent* event,
    AzQtComponents::DragAndDropContextBase& context,
    AZStd::optional<AZStd::vector<const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry*>*> outSources,
    AZStd::optional<AZStd::vector<const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry*>*> outProducts
) const
{
    using namespace AzQtComponents;
    using namespace AzToolsFramework;
    using namespace AzToolsFramework::AssetBrowser;
    using namespace AzAssetBrowserRequestHandlerPrivate;

    // if a listener with a higher priority already claimed this event, do not touch it.
    ViewportDragContext* viewportDragContext = azrtti_cast<ViewportDragContext*>(&context);
    if ((!event) || (!event->mimeData()) || (event->isAccepted()) || (!viewportDragContext))
    {
        return false;
    }

    bool canAcceptEvent = false;

    // Detects Source Asset Entries whose extensions are handled by a system
    AzToolsFramework::AssetBrowser::AssetBrowserEntry::ForEachEntryInMimeData<SourceAssetBrowserEntry>(
        event->mimeData(), [&](const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* source) {
            if (AssetBrowser::AssetBrowserSourceDropBus::HasHandlers(source->GetExtension()))
            {
                if (outSources.has_value())
                {
                    outSources.value()->push_back(source);
                }
                canAcceptEvent = true;
            }
        });

    // Detects Product Assets that are dragged directly, or child Products of other entry types.
    AzToolsFramework::AssetBrowser::AssetBrowserEntry::ForEachEntryInMimeData<ProductAssetBrowserEntry>(
        event->mimeData(), [&](const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* product)
    {
        // Skip if this product is a child of a source file that is handled
        if (outSources.has_value() && !outSources.value()->empty())
        {
            auto parent = azrtti_cast<const SourceAssetBrowserEntry*>(product->GetParent());
            if (parent != nullptr && AZStd::find(outSources.value()->begin(), outSources.value()->end(), parent) != outSources.value()->end())
            {
                return;
            }
        }

        if (CanSpawnEntityForProduct(product))
        {
            if (outProducts.has_value())
            {
                outProducts.value()->push_back(product);
            }
            canAcceptEvent = true;
        }
    });

    return canAcceptEvent;
}

void AzAssetBrowserRequestHandler::DragEnter(QDragEnterEvent* event, AzQtComponents::DragAndDropContextBase& context)
{
    if (CanAcceptDragAndDropEvent(event, context))
    {
        event->setDropAction(Qt::CopyAction);
        event->setAccepted(true);
    }
}

void AzAssetBrowserRequestHandler::DragMove(QDragMoveEvent* event, AzQtComponents::DragAndDropContextBase& context)
{
    if (CanAcceptDragAndDropEvent(event, context))
    {
        event->setDropAction(Qt::CopyAction);
        event->setAccepted(true);
    }
}

void AzAssetBrowserRequestHandler::DragLeave(QDragLeaveEvent* /*event*/)
{
    // opportunities to show ghosted entities or previews here.
}

// There are two paths for generating entities by dragging and dropping from the asset browser.
// This logic handles dropping them into the viewport. Dropping them in the outliner is handled by OutlinerListModel::DropMimeDataAssets.
void AzAssetBrowserRequestHandler::Drop(QDropEvent* event, AzQtComponents::DragAndDropContextBase& context)
{
    using namespace AzToolsFramework;
    using namespace AzToolsFramework::AssetBrowser;
    using namespace AzQtComponents;
    using namespace AzAssetBrowserRequestHandlerPrivate;

    AZStd::vector<const SourceAssetBrowserEntry*> sources;
    AZStd::vector<const ProductAssetBrowserEntry*> products;

    if (!CanAcceptDragAndDropEvent(event, context, &sources, &products))
    {
        // ALWAYS CHECK - you are not the only one connected to this bus, and someone else may have already
        // handled the event or accepted the drop - it might not contain types relevant to you.
        // you still get informed about the drop event in case you did some stuff in your gui and need to clean it up.
        return;
    }

    //  we wouldn't reach this code if the following cast is null or the event was null or accepted was already true.
    ViewportDragContext* viewportDragContext = azrtti_cast<ViewportDragContext*>(&context);

    event->setDropAction(Qt::CopyAction);
    event->setAccepted(true);

    EntityIdList spawnedEntities;
    AzFramework::SliceInstantiationTicket spawnTicket;

    // Make a scoped undo that covers the ENTIRE operation.
    ScopedUndoBatch undo("Create entities from asset");

    // Handle sources
    for (const SourceAssetBrowserEntry* source : sources)
    {
        AssetBrowser::AssetBrowserSourceDropBus::Event(
            source->GetExtension(),
            &AssetBrowser::AssetBrowserSourceDropEvents::HandleSourceFileType,
            source->GetFullPath(),
            AZ::EntityId(),
            viewportDragContext->m_hitLocation
        );
    }

    // Handle products
    SpawnEntityAtPoint(products, viewportDragContext, spawnedEntities, spawnTicket);

    // Select the new entity (and deselect others).
    if (!spawnedEntities.empty())
    {
        ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, spawnedEntities);
    }
}

void AzAssetBrowserRequestHandler::AddSourceFileOpeners(
    [[maybe_unused]] const char* fullSourceFileName,
    const AZ::Uuid& sourceUUID,
    AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers)
{
    using namespace AzToolsFramework;

    //Get asset group to support a variety of file extensions
    const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* fullDetails =
        AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry::GetSourceByUuid(sourceUUID);
    if (!fullDetails)
    {
        return;
    }

    // check to see if it is a default "generic" serializable asset
    // and open the asset editor if so. Check whether the Generic Asset handler handles this kind of asset.
    // to do so we need the actual type of that asset, which requires an asset type, not a source type.
    AZ::Data::AssetManager& manager = AZ::Data::AssetManager::Instance();

    // find a product type to query against.
    AZStd::vector<const AssetBrowser::ProductAssetBrowserEntry*> candidates;
    fullDetails->GetChildrenRecursively<AssetBrowser::ProductAssetBrowserEntry>(candidates);

    // find the first one that is handled by something:
    for (const AssetBrowser::ProductAssetBrowserEntry* productEntry : candidates)
    {
        // is there a Generic Asset Handler for it?
        AZ::Data::AssetType productAssetType = productEntry->GetAssetType();
        if ((productAssetType == AZ::Data::s_invalidAssetType) || (!productEntry->GetAssetId().IsValid()))
        {
            continue;
        }

        if (const AZ::Data::AssetHandler* assetHandler = manager.GetHandler(productAssetType))
        {
            if (!azrtti_istypeof<AzFramework::GenericAssetHandlerBase*>(assetHandler))
            {
                // it is not the generic asset handler.
                continue;
            }
            
            // yes, it is the generic asset handler, so install an opener that sends it to the Asset Editor.

            AZ::Data::AssetId assetId = productEntry->GetAssetId();
            AZ::Data::AssetType assetType = productEntry->GetAssetType();

            openers.push_back(
            {
                "Open_In_Asset_Editor",
                "Open in Asset Editor...",
                QIcon(),
                [assetId, assetType](const char* /*fullSourceFileNameInCallback*/, const AZ::Uuid& /*sourceUUID*/)
            {
                AZ::Data::Asset<AZ::Data::AssetData> asset = AZ::Data::AssetManager::Instance().FindOrCreateAsset(assetId, assetType, AZ::Data::AssetLoadBehavior::Default);
                AzToolsFramework::AssetEditor::AssetEditorRequestsBus::Broadcast(&AzToolsFramework::AssetEditor::AssetEditorRequests::OpenAssetEditor, asset);
            }
            });
            break; // no need to proceed further
        }
    }
}

void AzAssetBrowserRequestHandler::AddSourceFileCreators([[maybe_unused]] const char* fullSourceFileName, [[maybe_unused]] const AZ::Uuid& sourceUUID, [[maybe_unused]] AzToolsFramework::AssetBrowser::SourceFileCreatorList& creators)
{
}

void AzAssetBrowserRequestHandler::OpenAssetInAssociatedEditor(const AZ::Data::AssetId& assetId, bool& alreadyHandled)
{
    using namespace AzToolsFramework::AssetBrowser;
    if (alreadyHandled)
    {
        // a higher priority listener has already taken this request.
        return;
    }
    const SourceAssetBrowserEntry* source = SourceAssetBrowserEntry::GetSourceByUuid(assetId.m_guid);
    if (!source)
    {
        return;
    }

    AZStd::string fullEntryPath = source->GetFullPath();
    AZ::Uuid sourceID = source->GetSourceUuid();

    if (fullEntryPath.empty())
    {
        return;
    }

    QWidget* mainWindow = nullptr;
    AzToolsFramework::EditorRequestBus::BroadcastResult(mainWindow, &AzToolsFramework::EditorRequests::GetMainWindow);

    SourceFileOpenerList openers;
    AssetBrowserInteractionNotificationBus::Broadcast(&AssetBrowserInteractionNotificationBus::Events::AddSourceFileOpeners, fullEntryPath.c_str(), sourceID, openers);

    // did anyone actually accept it?
    if (!openers.empty())
    {
        // yes, call the opener and return.

        // are there more than one opener(s)?
        const SourceFileOpenerDetails* openerToUse = nullptr;
        // a function which reassigns openerToUse to be the selected one.
        AZStd::function<void(const SourceFileOpenerDetails*)> switchToOpener = [&openerToUse](const SourceFileOpenerDetails* switchTo)
            {
                openerToUse = switchTo;
            };

        // callers are allowed to add nullptr to openers.  So we only evaluate the valid ones.
        // and if there is only one valid one, we use that one.
        const SourceFileOpenerDetails* firstValidOpener = nullptr;
        int numValidOpeners = 0;
        QMenu menu(mainWindow);
        for (const SourceFileOpenerDetails& openerDetails : openers)
        {
            // bind that function to the current loop element.
            if (openerDetails.m_opener) // only VALID openers with an actual callback.
            {
                ++numValidOpeners;
                if (!firstValidOpener)
                {
                    firstValidOpener = &openerDetails;
                }
                // bind a callback such that when the menu item is clicked, it sets that as the opener to use.
                menu.addAction(openerDetails.m_iconToUse, QObject::tr(openerDetails.m_displayText.c_str()), mainWindow, [switchToOpener, details = &openerDetails] { return switchToOpener(details); });
            }
        }

        if (numValidOpeners > 1) // more than one option was added
        {
            menu.addSeparator();
            menu.addAction(QObject::tr("Cancel"), [switchToOpener] { return switchToOpener(nullptr); }); // just something to click on to avoid doing anything.
            menu.exec(QCursor::pos());
        }
        else if (numValidOpeners == 1)
        {
            openerToUse = firstValidOpener;
        }

        // did we select one and did it have a function to call?
        if ((openerToUse) && (openerToUse->m_opener))
        {
            openerToUse->m_opener(fullEntryPath.c_str(), sourceID);
        }
        alreadyHandled = true;
        return; // an opener handled this, no need to proceed further.
    }

    // if we get here, nothing handled it, so try the operating system.
    alreadyHandled = OpenWithOS(fullEntryPath);
}

bool AzAssetBrowserRequestHandler::OpenWithOS(const AZStd::string& fullEntryPath)
{
    bool openedSuccessfully = QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromUtf8(fullEntryPath.c_str())));
    if (!openedSuccessfully)
    {
        AZ_Printf("Asset Browser", "Unable to open '%s' using the operating system.  There might be no editor associated with this kind of file.\n", fullEntryPath.c_str());
    }

    return openedSuccessfully;
}
