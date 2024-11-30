
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include <AzAssetBrowser/AzAssetBrowserMultiWindow.h>
#include "AzAssetBrowser/AzAssetBrowserWindow.h"
#include "AzAssetBrowserRequestHandler.h"

// Qt
#include <QDesktopServices>
#include <QMenu>
#include <QObject>
#include <QString>

// AzCore
#include <AzCore/std/string/wildcard.h>
#include <AzCore/std/sort.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>

// AzFramework
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/GenericAssetHandler.h>

// AzToolsFramework
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryUtils.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Favorites/AssetBrowserFavoritesView.h>

#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTableView.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserListView.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserThumbnailView.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeView.h>
#include <AzToolsFramework/AssetEditor/AssetEditorBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

// AzQtComponents
#include <AzQtComponents/DragAndDrop/ViewportDragAndDrop.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerDragAndDropContext.h>

// Editor
#include "IEditor.h"
#include "CryEditDoc.h"
#include "QtViewPaneManager.h"

namespace AzAssetBrowserRequestHandlerPrivate
{
    using namespace AzToolsFramework;
    using namespace AzToolsFramework::AssetBrowser;

    bool ProductHasAssociatedComponent(const ProductAssetBrowserEntry* product);

    // given a list of products all belonging to the same parent source file
    // select just one that can best represent the entire source file.
    // it does so by collecting all of the assets that have valid create-able
    // components associated with them and sorts them using
    // a heuristic which prefers exact name matches, and operates in alphabetic order otherwise.
    // for example in the scenario, where we pass in the products all from bike.fbx:
    // bike.fbx
    //   +--- wheel01.model
    //   +--- wheel02.model
    //   +--- chasis.model
    //   +--- bike_gloss_texture.texture
    //   +--- bike_gloss_material.material
    //   +--- bike.model        <--- select this one, its an exact match
    //
    // this behavior can be controlled by an AssetTypeInfo listener, it can add priority
    // to the sort order of an asset by type.  For example in the above scenario, an asset
    // type listener could override the priority of 'material' and thus cause it to end up
    // higher up or lower down on the final list of products to choose from.

    const ProductAssetBrowserEntry* GetPrimaryProduct(AZStd::vector<const ProductAssetBrowserEntry*>& entries)
    {
        const SourceAssetBrowserEntry* parentSource = nullptr;

        AZStd::vector<const ProductAssetBrowserEntry*> validEntries;
        validEntries.reserve(entries.size());
        for (const auto entry : entries)
        {
            if (!ProductHasAssociatedComponent(entry))
            {
                continue;
            }
            validEntries.push_back(entry);
        }

        if (validEntries.empty())
        {
            return nullptr;
        }

        // make sure all the valid entries share the same parent source file, and get the source!
        for (const auto entry : validEntries)
        {
            if (entry->GetParent() == nullptr)
            {
                return entry; // return the first one in the case of a weird disconnected situation
            }
            if ((parentSource) && (parentSource != entry->GetParent()))
            {
                return entry; // if they have different parents, something is wrong, return the first one we find.
            }
            if (entry->GetParent()->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source)
            {
                parentSource = static_cast<const SourceAssetBrowserEntry*>(entry->GetParent());
            }
        }

        if (!parentSource)
        {
            return nullptr;
        }
        AZ::IO::Path parentName = parentSource->GetName();
        AZ::IO::PathView parentNameWithoutExtension = parentName.Stem();
        AZ::IO::PathView parentNameWithoutExtensionCaseInsensitive(parentNameWithoutExtension.Native(), AZ::IO::WindowsPathSeparator);

        // if we get here, we know that all the entries have the same source parent and are thus related to each other.
        // in this case, we can run the heuristic.
        // sort the valid entries so that they are arranged from most preferred to least preferred
        // asset to create a component for:

        AZStd::sort(
            validEntries.begin(),
            validEntries.end(),
            [&](const ProductAssetBrowserEntry* a, const ProductAssetBrowserEntry* b)
            {
                // first, we use the priority system:
                int sortPriorityA = 0;
                int sortPriorityB = 0;
                AZ::AssetTypeInfoBus::EventResult(
                    sortPriorityA, a->GetAssetType(),
                    &AZ::AssetTypeInfo::GetAssetTypeDragAndDropCreationPriority);
                AZ::AssetTypeInfoBus::EventResult(
                    sortPriorityB, b->GetAssetType(),
                    &AZ::AssetTypeInfo::GetAssetTypeDragAndDropCreationPriority);

                if (sortPriorityA > sortPriorityB)
                {
                    return true; // A definitely before B
                }
                else if (sortPriorityA < sortPriorityB)
                {
                    return false; // B definitely before A
                }

                // Getting here means their sort priority is equal.  When this is the case, we use a
                // secondary sort scheme, which is to prefer the assets that have the exact
                // same name as the parent source asset:

                // Make a temporary PathView that uses a WindowsPathSeparator, to have the path comparisons be case-insensitive on
                // non-Windows platforms as well. By default Windows uses the WindowsPathSeparator so it is already case-insensitive on
                // Windows
                AZ::IO::PathView nameAWithoutExtension = AZ::IO::PathView(a->GetName(), AZ::IO::WindowsPathSeparator).Stem();
                AZ::IO::PathView nameBWithoutExtension = AZ::IO::PathView(b->GetName(), AZ::IO::WindowsPathSeparator).Stem();

                const bool aMatches = nameAWithoutExtension == parentNameWithoutExtensionCaseInsensitive;
                const bool bMatches = nameBWithoutExtension == parentNameWithoutExtensionCaseInsensitive;

                if ((aMatches) && (!bMatches))
                {
                    return true; // A definitely before B
                }
                if ((!aMatches) && (bMatches))
                {
                    return false; // B definitely before A
                }

                // if nothing else, sort alphabetically.  A is before B if
                // strcmp A, B < 0.  Otherwise A is not before B.  Use the extension
                // here so as to eliminate ties.
                return a->GetName() < b->GetName();
            });

        // since the valid entries are already sorted, just return the first one.
        return validEntries[0];
    }

    // return true if a given product has an associated component type.
    bool ProductHasAssociatedComponent(const ProductAssetBrowserEntry* product)
    {
        if (!product)
        {
            return false;
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

        return true;
    }

    // given a list of product assets, create an entity for each one where
    // appropriate.  Note that the list of product assets is expected to already have been filtered
    // by the above functions, or are expected to be direct user choices, not sources.
    void CreateEntitiesAtPoint(
        AZStd::vector<const ProductAssetBrowserEntry*> products,
        AZ::Vector3 location,
        AZ::EntityId parentEntityId, // if valid, will treat the location as a local transform relative to this entity.
        EntityIdList& createdEntities)
    {
        if (products.empty())
        {
            return;
        }

        ScopedUndoBatch undo("Create entities from assets");

        QWidget* mainWindow = nullptr;
        EditorRequests::Bus::BroadcastResult(mainWindow, &EditorRequests::GetMainWindow);

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

        for (const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* product : products)
        {
            AZ::Uuid componentTypeId = AZ::Uuid::CreateNull();
            AZ::AssetTypeInfoBus::EventResult(componentTypeId, product->GetAssetType(), &AZ::AssetTypeInfo::GetComponentTypeId);
            if (!componentTypeId.IsNull())
            {
                AZ::IO::Path entryPath(product->GetName());
                AZStd::string entityName = entryPath.Stem().Native();
                if (entityName.empty())
                {
                    // if we can't use the file name, use the type of asset like "Model".
                    AZ::AssetTypeInfoBus::EventResult(entityName, product->GetAssetType(), &AZ::AssetTypeInfo::GetAssetTypeDisplayName);
                    if (entityName.empty())
                    {
                        entityName = "Entity";
                    }
                }

                AZ::EntityId targetEntityId;
                EditorRequests::Bus::BroadcastResult(
                    targetEntityId, &EditorRequests::CreateNewEntityAtPosition, worldTransform.GetTranslation(), parentEntityId);

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

                newEntity->SetName(entityName);

                AZ::ComponentTypeList componentsToAdd;
                componentsToAdd.push_back(componentTypeId);

                    // Add the product as components to this entity.
                AZStd::vector<AZ::EntityId> entityIds = { targetEntityId };
                EntityCompositionRequests::AddComponentsOutcome addComponentsOutcome = AZ::Failure(AZStd::string());
                EntityCompositionRequestBus::BroadcastResult(
                    addComponentsOutcome, &EntityCompositionRequests::AddComponentsToEntities, entityIds, componentsToAdd);

                if (!addComponentsOutcome.IsSuccess())
                {
                    AZ_Error(
                        "AssetBrowser",
                        false,
                        "Could not create the requested components from the selected assets: %s",
                        addComponentsOutcome.GetError().c_str());
                    EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::DestroyEditorEntity, targetEntityId);
                    return;
                }
                // activate the entity first, so that the primary asset change is done in the context of it being awake.
                newEntity->Activate();

                AZ::Component* componentAdded = newEntity->FindComponent(componentTypeId);
                if (componentAdded)
                {
                    Components::EditorComponentBase* editorComponent = GetEditorComponent(componentAdded);
                    if (editorComponent)
                    {
                        editorComponent->SetPrimaryAsset(product->GetAssetId());
                    }
                }

                ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::AddDirtyEntity, newEntity->GetId());
                createdEntities.push_back(newEntity->GetId());
            }
        }

        // Select the new entity (and deselect others).
        if (!createdEntities.empty())
        {
            ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, createdEntities);
        }
    }
}

AzAssetBrowserRequestHandler::AzAssetBrowserRequestHandler()
{
    using namespace AzToolsFramework::AssetBrowser;

    AssetBrowserInteractionNotificationBus::Handler::BusConnect();
    AzQtComponents::DragAndDropEventsBus::Handler::BusConnect(AzQtComponents::DragAndDropContexts::EditorViewport);
    AzQtComponents::DragAndDropItemViewEventsBus::Handler::BusConnect(AzQtComponents::DragAndDropContexts::EntityOutliner);
}

AzAssetBrowserRequestHandler::~AzAssetBrowserRequestHandler()
{
    AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
    AzQtComponents::DragAndDropEventsBus::Handler::BusDisconnect();
    AzQtComponents::DragAndDropItemViewEventsBus::Handler::BusDisconnect();
}

void AzAssetBrowserRequestHandler::CreateSortAction(
    QMenu* menu,
    AzToolsFramework::AssetBrowser::AssetBrowserThumbnailView* thumbnailView,
    AzToolsFramework::AssetBrowser::AssetBrowserTreeView* treeView,
    QString name,
    AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntrySortMode sortMode)
{
    QAction* action = menu->addAction(
        name,
        [thumbnailView, treeView, sortMode]()
        {
            if (thumbnailView)
            {
                thumbnailView->SetSortMode(sortMode);
            }
            else if (treeView)
            {
                treeView->SetSortMode(sortMode);
                if (treeView->GetAttachedThumbnailView())
                {
                    treeView->GetAttachedThumbnailView()->SetSortMode(sortMode);
                }
            }
        });

    action->setCheckable(true);

    if (thumbnailView)
    {
        if (thumbnailView->GetSortMode() == sortMode)
        {
            action->setChecked(true);
        }
    }
    else if (treeView)
    {
        // If there is a thumbnailView attached, the sorting will be happening in there.
        if (treeView->GetAttachedThumbnailView())
        {
            if (treeView->GetAttachedThumbnailView()->GetSortMode() == sortMode)
            {
                action->setChecked(true);
            }
        }
        else if (treeView->GetSortMode() == sortMode)
        {
            action->setChecked(true);
        }
    }
}


void AzAssetBrowserRequestHandler::AddSortMenu(
    QMenu* menu,
    AzToolsFramework::AssetBrowser::AssetBrowserThumbnailView* thumbnailView,
    AzToolsFramework::AssetBrowser::AssetBrowserTreeView* treeView,
    AzToolsFramework::AssetBrowser::AssetBrowserTableView* tableView)
{
    using namespace AzToolsFramework::AssetBrowser;

    //TableView handles its own sorting.
    if (tableView)
    {
        return;
    }

    // Check for the menu being called from the treeview when a tableview is active.
    if (treeView && treeView->GetAttachedTableView() && treeView->GetAttachedTableView()->GetTableViewActive())
    {
        return;
    }

    QMenu* sortMenu = menu->addMenu(QObject::tr("Sort by"));

    CreateSortAction(
        sortMenu,
        thumbnailView,
        treeView,
        QObject::tr("Name"), AssetBrowserEntry::AssetEntrySortMode::Name);

    CreateSortAction(
        sortMenu,
        thumbnailView,
        treeView,
        QObject::tr("Type"), AssetBrowserEntry::AssetEntrySortMode::FileType);

    CreateSortAction(
        sortMenu,
        thumbnailView,
        treeView,
        QObject::tr("Date"), AssetBrowserEntry::AssetEntrySortMode::LastModified);

    CreateSortAction(
        sortMenu,
        thumbnailView,
        treeView,
        QObject::tr("Size"), AssetBrowserEntry::AssetEntrySortMode::Size);
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

void AzAssetBrowserRequestHandler::AddContextMenuActions(QWidget* caller, QMenu* menu, const AZStd::vector<const AzToolsFramework::AssetBrowser::AssetBrowserEntry*>& entries)
{
    using namespace AzToolsFramework::AssetBrowser;

    QString callerName = QString();
    bool calledFromAssetBrowser = false;

    AssetBrowserTreeView* treeView = qobject_cast<AssetBrowserTreeView*>(caller);
    if (treeView)
    {
        calledFromAssetBrowser = treeView->GetIsAssetBrowserMainView();
    }

    AssetBrowserListView* listView = qobject_cast<AssetBrowserListView*>(caller);
    if (listView)
    {
        calledFromAssetBrowser |= listView->GetIsAssetBrowserMainView();
    }

    AssetBrowserThumbnailView* thumbnailView = qobject_cast<AssetBrowserThumbnailView*>(caller);
    if (thumbnailView)
    {
        calledFromAssetBrowser |= thumbnailView->GetIsAssetBrowserMainView();
    }

    AssetBrowserTableView* tableView = qobject_cast<AssetBrowserTableView*>(caller);
    if (tableView)
    {
        calledFromAssetBrowser |= tableView->GetIsAssetBrowserMainView();
    }

    AssetBrowserFavoritesView* favoritesView = qobject_cast<AssetBrowserFavoritesView*>(caller);
    if (favoritesView)
    {
        calledFromAssetBrowser = false;
    }

    if (!treeView && !tableView && !thumbnailView && !listView && !favoritesView)
    {
        return;
    }

    const AssetBrowserEntry* entry = entries.empty() ? nullptr : entries.front();
    if (!entry)
    {
        return;
    }

    if (favoritesView)
    {
        menu->addAction(
            QObject::tr("View in Asset Browser"),
            [favoritesView, entry]()
            {
                AssetBrowserFavoriteRequestBus::Broadcast(
                    &AssetBrowserFavoriteRequestBus::Events::ViewEntryInAssetBrowser, favoritesView, entry);
            });
    }

    bool isFavorite = false;
    AssetBrowserFavoriteRequestBus::BroadcastResult(isFavorite, &AssetBrowserFavoriteRequestBus::Events::GetIsFavoriteAsset, entry);

    if (isFavorite)
    {
        menu->addAction(
            QObject::tr("Remove from Favorites"),
            [entry]()
            {
                AssetBrowserFavoriteRequestBus::Broadcast(&AssetBrowserFavoriteRequestBus::Events::RemoveEntryFromFavorites, entry);
            });
    }
    else
    {
        if (entry->GetEntryType() != AssetBrowserEntry::AssetEntryType::Product)
        {
            menu->addAction(
                QObject::tr("Add to Favorites"),
                [caller]()
                {
                    AssetBrowserFavoriteRequestBus::Broadcast(&AssetBrowserFavoriteRequestBus::Events::AddFavoriteEntriesButtonPressed, caller);
                });
        }
    }

    if (calledFromAssetBrowser)
    {
        AddSortMenu(menu, thumbnailView, treeView, tableView);
    }

    size_t numOfEntries = entries.size();

    AZStd::string fullFilePath;
    AZStd::string extension;
    bool selectionIsSource{ true };

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
        selectionIsSource = false;
    // the fall through to the next case is intentional here.
    case AssetBrowserEntry::AssetEntryType::Source:
    {
        AZ::Uuid sourceID = azrtti_cast<const SourceAssetBrowserEntry*>(entry)->GetSourceUuid();
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

            menu->addAction(
                QObject::tr("Open in another Asset Browser"),
                [fullFilePath, thumbnailView, tableView]()
                {
                    AzAssetBrowserWindow* newAssetBrowser = AzAssetBrowserMultiWindow::OpenNewAssetBrowserWindow();

                    if (thumbnailView)
                    {
                        newAssetBrowser->SetCurrentMode(AssetBrowserMode::ThumbnailView);
                    }
                    else if (tableView)
                    {
                        newAssetBrowser->SetCurrentMode(AssetBrowserMode::TableView);
                    }
                    else
                    {
                        newAssetBrowser->SetCurrentMode(AssetBrowserMode::ListView);
                    }

                    newAssetBrowser->SelectAsset(fullFilePath.c_str());
                });

            AZStd::vector<const ProductAssetBrowserEntry*> products;
            entry->GetChildrenRecursively<ProductAssetBrowserEntry>(products);

            if (!products.empty() || (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source))
            {
                CFileUtil::PopulateQMenu(caller, menu, fullFilePath);
            }
            if (calledFromAssetBrowser && selectionIsSource)
            {
                // Add Rename option
                QAction* action = menu->addAction(
                    QObject::tr("Rename asset"),
                    [treeView, listView, thumbnailView, tableView]()
                    {
                        if (treeView)
                        {
                            treeView->RenameEntry();
                        }
                        else if (listView)
                        {
                            listView->RenameEntry();
                        }
                        else if (thumbnailView)
                        {
                            thumbnailView->RenameEntry();
                        }
                        else if (tableView)
                        {
                            tableView->RenameEntry();
                        }
                    });
                action->setShortcut(Qt::Key_F2);
                action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
            }
        }

        if (calledFromAssetBrowser && selectionIsSource)
        {
            // Add Delete option
            QAction* action = menu->addAction(
                QObject::tr("Delete asset%1").arg(numOfEntries > 1 ? "s" : ""),
                [treeView, listView, thumbnailView, tableView]()
                {
                    if (treeView)
                    {
                        treeView->DeleteEntries();
                    }
                    else if (listView)
                    {
                        listView->DeleteEntries();
                    }
                    else if (thumbnailView)
                    {
                        thumbnailView->DeleteEntries();
                    }
                    else if (tableView)
                    {
                        tableView->DeleteEntries();
                    }
                });
            action->setShortcut(QKeySequence::Delete);
            action->setShortcutContext(Qt::WidgetWithChildrenShortcut);

            // Add Duplicate option
            action = menu->addAction(
                QObject::tr("Duplicate asset%1").arg(numOfEntries > 1 ? "s" : ""),
                [treeView, listView, thumbnailView, tableView]()
                {
                    if (treeView)
                    {
                        treeView->DuplicateEntries();
                    }
                    else if (listView)
                    {
                        listView->DuplicateEntries();
                    }
                    else if (thumbnailView)
                    {
                        thumbnailView->DuplicateEntries();
                    }
                    else if (tableView)
                    {
                        tableView->DuplicateEntries();
                    }
                });
            action->setShortcut(QKeySequence("Ctrl+D"));
            action->setShortcutContext(Qt::WidgetWithChildrenShortcut);

            // Add Move to option
            menu->addAction(
                QObject::tr("Move to"),
                [treeView, listView, thumbnailView, tableView]()
                {
                    if (treeView)
                    {
                        treeView->MoveEntries();
                    }
                    else if (listView)
                    {
                        listView->MoveEntries();
                    }
                    else if (thumbnailView)
                    {
                        thumbnailView->MoveEntries();
                    }
                    else if (tableView)
                    {
                        tableView->MoveEntries();
                    }
                });
        }
    }
    break;
    case AssetBrowserEntry::AssetEntryType::Folder:
    {
        fullFilePath = entry->GetFullPath();

        CFileUtil::PopulateQMenu(caller, menu, fullFilePath);

        if (calledFromAssetBrowser)
        {
            if (numOfEntries == 1)
            {
                // Add Rename option
                QAction* action = menu->addAction(
                    QObject::tr("Rename Folder"),
                    [treeView, listView, thumbnailView, tableView]()
                    {
                        if (treeView)
                        {
                            treeView->RenameEntry();
                        }
                        else if (listView)
                        {
                            listView->RenameEntry();
                        }
                        else if (thumbnailView)
                        {
                            thumbnailView->RenameEntry();
                        }
                        else if (tableView)
                        {
                            tableView->RenameEntry();
                        }
                    });
                action->setShortcut(Qt::Key_F2);
                action->setShortcutContext(Qt::WidgetWithChildrenShortcut);

                // Add Delete option
                action = menu->addAction(
                    QObject::tr("Delete Folder"),
                    [treeView, listView, thumbnailView, tableView]()
                    {
                        if (treeView)
                        {
                            treeView->DeleteEntries();
                        }
                        else if (listView)
                        {
                            listView->DeleteEntries();
                        }
                        else if (thumbnailView)
                        {
                            thumbnailView->DeleteEntries();
                        }
                        else if (tableView)
                        {
                            tableView->DeleteEntries();
                        }
                    });
                action->setShortcut(QKeySequence::Delete);
                action->setShortcutContext(Qt::WidgetWithChildrenShortcut);

                // Add Move to option
                menu->addAction(
                    QObject::tr("Move to"),
                    [treeView, listView, thumbnailView, tableView]()
                    {
                        if (treeView)
                        {
                            treeView->MoveEntries();
                        }
                        else if (listView)
                        {
                            listView->MoveEntries();
                        }
                        else if (thumbnailView)
                        {
                            thumbnailView->MoveEntries();
                        }
                        else if (tableView)
                        {
                            tableView->MoveEntries();
                        }
                    });
                AddCreateMenu(menu, fullFilePath);
            }
        }
    }
    break;
    default:
        break;
    }
}

bool AzAssetBrowserRequestHandler::CanAcceptDragAndDropEvent(QDropEvent* event, AzQtComponents::DragAndDropContextBase& context) const
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

    return DecodeDragMimeData(event->mimeData());
}

bool AzAssetBrowserRequestHandler::DecodeDragMimeData(const QMimeData* mimeData,
    AZStd::vector<const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry*>* outProducts) const
{
    using namespace AzToolsFramework;
    using namespace AzToolsFramework::AssetBrowser;
    using namespace AzAssetBrowserRequestHandlerPrivate;

    // what we'd like to do with drop events is create an entity per selected logical asset
    // Note that some types of source files (FBX,...) produce more than one product.  In this case,
    // this default fallback handler will choose the most likely representitive one using a heuristic
    // and only return that one.
    // once AB supports multi-select, the data might contain a mixture of
    // selected source files, selected products.  Selecting a source file should evaluate its products
    // selecting a product should always use the product.

    bool canAcceptEvent = false;
    AZStd::vector<const AssetBrowserEntry*> allEntries;
    if (!AzToolsFramework::AssetBrowser::Utils::FromMimeData(mimeData, allEntries))
    {
        return false;
    }
    // all entries now contains the actual list of actually selected entries (not their children)
    for (const AssetBrowserEntry* entry : allEntries)
    {
        if (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source)
        {
            // This default fallback handler doesn't care about source files.
            // If you want to support source file drag operations, implement a listener with a higher priority
            // in your gem, and connect to the drag and drop busses.  You can then intercept it and use code similar
            // to the above to decode the mime data and do whatever you need to do, consuming the event so that
            // we don't reach this point.

            // Because of that, this handler only cares about products, so we convert any source entries to the most
            // reasonable target product candidate.
            AZStd::vector<const ProductAssetBrowserEntry*> candidateProducts;
            entry->GetChildrenRecursively<ProductAssetBrowserEntry>(candidateProducts);
            const ProductAssetBrowserEntry* mostReasonable = GetPrimaryProduct(candidateProducts);
            if (mostReasonable)
            {
                canAcceptEvent = true;
                if (outProducts)
                {
                    if (AZStd::ranges::find(*outProducts, mostReasonable) == outProducts->end())
                    {
                        outProducts->push_back(mostReasonable);
                    }
                }
            }
        }
        else if (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Product)
        {
            const ProductAssetBrowserEntry* product = static_cast<const ProductAssetBrowserEntry*>(entry);
            // a product is directly selected - this overrides any rules we have about heuristics
            // since its an explicit user action
            if (ProductHasAssociatedComponent(product))
            {
                // its a creatable one.
                canAcceptEvent = true;
                if (outProducts)
                {
                    if (AZStd::ranges::find(*outProducts, product) == outProducts->end())
                    {
                        outProducts->push_back(product);
                    }
                }
            }
        }
    }
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
    // opportunities to clean up any preview objects here.
}

// listview/outliner dragging:
void AzAssetBrowserRequestHandler::CanDropItemView(bool& accepted, AzQtComponents::DragAndDropContextBase& context)
{
    using namespace AzToolsFramework;
    if (accepted)
    {
        return; // someone else already took this!
    }

    if (EntityOutlinerDragAndDropContext* outlinerContext = azrtti_cast<EntityOutlinerDragAndDropContext*>(&context))
    {
        if (DecodeDragMimeData(outlinerContext->m_dataBeingDropped))
        {
            accepted = true;
        }
    }
}

void AzAssetBrowserRequestHandler::DoDropItemView(bool& accepted, AzQtComponents::DragAndDropContextBase& context)
{
    using namespace AzToolsFramework;
    using namespace AzToolsFramework::AssetBrowser;
    using namespace AzAssetBrowserRequestHandlerPrivate;

    if (accepted)
    {
        return; // someone else already took this!
    }

    if (EntityOutlinerDragAndDropContext* outlinerContext = azrtti_cast<EntityOutlinerDragAndDropContext*>(&context))
    {
        // drop the item(s)
        AZStd::vector<const ProductAssetBrowserEntry*> products;
        if (DecodeDragMimeData(outlinerContext->m_dataBeingDropped, &products))
        {
            accepted = true;
            // in this case, it should behave just like dropping the entity into the world at world origin.
            // Make a scoped undo that covers the ENTIRE operation.
            AZ::Vector3 createLocation = AZ::Vector3::CreateZero();
            if (!outlinerContext->m_parentEntity.IsValid())
            {
                EditorRequestBus::BroadcastResult(createLocation, &EditorRequestBus::Events::GetWorldPositionAtViewportCenter);
            }

            EntityIdList createdEntities;
            CreateEntitiesAtPoint(products, createLocation, outlinerContext->m_parentEntity, createdEntities);
        }
    }
}

// There are multiple paths for generating entities by dragging and dropping from the asset browser.
// This logic handles dropping them into the viewport.
// Dropping them in the outliner is handled by OutlinerListModel::DropMimeDataAssets.
// There's also a higher priority listener for dropping things containing prefabs, by instantiating the prefab.
// This is used just when there's no higher priority listener to handle basic drops.
void AzAssetBrowserRequestHandler::Drop(QDropEvent* event, AzQtComponents::DragAndDropContextBase& context)
{
    using namespace AzToolsFramework;
    using namespace AzToolsFramework::AssetBrowser;
    using namespace AzQtComponents;
    using namespace AzAssetBrowserRequestHandlerPrivate;

    if (event->isAccepted())
    {
        return; // don't double handle drop events.
    }

    AZStd::vector<const ProductAssetBrowserEntry*> products;
    ViewportDragContext* viewportDragContext = azrtti_cast<ViewportDragContext*>(&context);

    if ((!viewportDragContext)||(!DecodeDragMimeData(event->mimeData(), &products)))
    {
        return;
    }

    event->setDropAction(Qt::CopyAction);
    event->setAccepted(true);

    EntityIdList createdEntities;
    CreateEntitiesAtPoint(products, viewportDragContext->m_hitLocation, AZ::EntityId(), createdEntities);
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

AzAssetBrowserWindow* AzAssetBrowserRequestHandler::FindAzAssetBrowserWindow(QWidget* widgetToStartSearchFrom)
{
    AzAssetBrowserWindow* assetBrowserWindow = nullptr;
    if (widgetToStartSearchFrom)
    {
        assetBrowserWindow = FindAzAssetBrowserWindowThatContainsWidget(widgetToStartSearchFrom);
    }

    if (!assetBrowserWindow)
    {
        assetBrowserWindow = AzToolsFramework::GetViewPaneWidget<AzAssetBrowserWindow>(LyViewPane::AssetBrowser);
    }

    return assetBrowserWindow;
}

AzAssetBrowserWindow* AzAssetBrowserRequestHandler::FindAzAssetBrowserWindowThatContainsWidget(QWidget* widget)
{
    AzAssetBrowserWindow* targetAssetBrowserWindow = nullptr;
    QWidget* candidate = widget;
    while (!targetAssetBrowserWindow)
    {
        targetAssetBrowserWindow = qobject_cast<AzAssetBrowserWindow*>(candidate);
        if (targetAssetBrowserWindow)
        {
            return targetAssetBrowserWindow;
        }

        if (!candidate->parentWidget())
        {
            return nullptr;
        }

        candidate = candidate->parentWidget();
    }

    return nullptr;
}

void AzAssetBrowserRequestHandler::SelectAsset(QWidget* caller, const AZStd::string& fullFilePath)
{
    if (AzAssetBrowserWindow* assetBrowserWindow = FindAzAssetBrowserWindow(caller))
    {
        assetBrowserWindow->SelectAsset(fullFilePath.c_str(), false);
    }
}

void AzAssetBrowserRequestHandler::SelectFolderAsset([[maybe_unused]] QWidget* caller, [[maybe_unused]] const AZStd::string& fullFolderPath)
{
    if (AzAssetBrowserWindow* assetBrowserWindow = FindAzAssetBrowserWindow(caller))
    {
        assetBrowserWindow->SelectAsset(fullFolderPath.c_str(), true);
    }
}
