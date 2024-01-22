/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <SandboxAPI.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzQtComponents/Buses/DragAndDrop.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/Previewer/PreviewerBus.h>

namespace AZ
{
    class Entity;
}

namespace AzQtComponents
{
    class DragAndDropContextBase;
}

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserEntry;
        class PreviewerFactory;
        class ProductAssetBrowserEntry;
        class SourceAssetBrowserEntry;
        class AssetBrowserTreeView;
        class AssetBrowserTableView;
        class AssetBrowserThumbnailView;
    }
}

// on windows, this is a DLL Exported class that derives from non-dll-exported
// baseclasses, which issues a warning (which is then treated as an error).
// However, all of the derived classes are ebus handlers that are all available in static
// libraries or by direct header inclusions, so they ARE available:
AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING

// this also triggers a MEMBER warning on the actual EBus Handler we derive from
// because it has members like m_node.
AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING

class AzAssetBrowserWindow;

class SANDBOX_API AzAssetBrowserRequestHandler
    : protected AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
    , protected AzQtComponents::DragAndDropEventsBus::Handler
    , protected AzQtComponents::DragAndDropItemViewEventsBus::Handler
{
public:
    AzAssetBrowserRequestHandler();
    ~AzAssetBrowserRequestHandler() override;

    //////////////////////////////////////////////////////////////////////////
    // AssetBrowserInteractionNotificationBus
    //////////////////////////////////////////////////////////////////////////
    void AddContextMenuActions(QWidget* caller, QMenu* menu, const AZStd::vector<const AzToolsFramework::AssetBrowser::AssetBrowserEntry*>& entries) override;
    void AddSourceFileOpeners(const char* fullSourceFileName, const AZ::Uuid& sourceUUID, AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers) override;
    void AddSourceFileCreators(const char* fullSourceFileName, const AZ::Uuid& sourceUUID, AzToolsFramework::AssetBrowser::SourceFileCreatorList& openers) override;
    void OpenAssetInAssociatedEditor(const AZ::Data::AssetId& assetId, bool& alreadyHandled) override;
    void SelectAsset(QWidget* caller, const AZStd::string& fullFilePath) override;
    void SelectFolderAsset([[maybe_unused]] QWidget* caller, [[maybe_unused]] const AZStd::string& fullFolderPath) override;

    static bool OpenWithOS(const AZStd::string& fullEntryPath);
    void AddCreateMenu(QMenu* menu, const AZStd::string fullFolderPath);
    void CreateSortAction(
        QMenu* menu,
        AzToolsFramework::AssetBrowser::AssetBrowserThumbnailView* thumbnailView,
        AzToolsFramework::AssetBrowser::AssetBrowserTreeView* treeView,
        QString name,
        AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntrySortMode sortMode);
    void AddSortMenu(QMenu* menu,
        AzToolsFramework::AssetBrowser::AssetBrowserThumbnailView* thumbnailView,
        AzToolsFramework::AssetBrowser::AssetBrowserTreeView* treeView,
        AzToolsFramework::AssetBrowser::AssetBrowserTableView* tableView
        );

protected:

    //////////////////////////////////////////////////////////////////////////
    // AzQtComponents::DragAndDropEventsBus::Handler
    //////////////////////////////////////////////////////////////////////////
    void DragEnter(QDragEnterEvent* event, AzQtComponents::DragAndDropContextBase& context) override;
    void DragMove(QDragMoveEvent* event, AzQtComponents::DragAndDropContextBase& context) override;
    void DragLeave(QDragLeaveEvent* event) override;
    void Drop(QDropEvent* event, AzQtComponents::DragAndDropContextBase& context) override;
    // listview/outliner dragging:
    void CanDropItemView(bool& accepted, AzQtComponents::DragAndDropContextBase& context) override;
    void DoDropItemView(bool& accepted, AzQtComponents::DragAndDropContextBase& context) override;

    bool CanAcceptDragAndDropEvent(QDropEvent* event, AzQtComponents::DragAndDropContextBase& context) const;

    bool DecodeDragMimeData(const QMimeData* mimeData,
                            AZStd::vector<const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry*>* outVector = nullptr) const;
    AzAssetBrowserWindow* FindAzAssetBrowserWindow(QWidget* widgetToStartSearchFrom);
    AzAssetBrowserWindow* FindAzAssetBrowserWindowThatContainsWidget(QWidget* widget);
};

AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING

