/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>

#include <QWidget>
#include <QMenu>
#endif

class QItemSelection;

namespace Ui
{
    class AzAssetBrowserWindowClass;
}

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserEntry;
        class AssetBrowserFilterModel;
        class AssetBrowserListModel;
        class AssetBrowserModel;
        class AssetBrowserFilterModel;
        class AssetBrowserTreeView;

        enum class AssetBrowserDisplayState : int
        {
            TreeViewMode,
            ListViewMode,
            Invalid
        };

        enum class AssetBrowserMode : int
        {
            ThumbnailView,
            TableView,
            ListView
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework

class AzAssetBrowserWindow : public QWidget
{
    Q_OBJECT
public:
    AZ_CLASS_ALLOCATOR(AzAssetBrowserWindow, AZ::SystemAllocator);

    explicit AzAssetBrowserWindow(QWidget* parent = nullptr);
    virtual ~AzAssetBrowserWindow();

    void SelectAsset(const QString& assetPath, bool assetIsFolder = false);

    static void RegisterViewClass();

    static QObject* createListenerForShowAssetEditorEvent(QObject* parent);

    bool ViewWidgetBelongsTo(QWidget* viewWidget);

    AzToolsFramework::AssetBrowser::AssetBrowserMode GetCurrentMode() const;
    void SetCurrentMode(const AzToolsFramework::AssetBrowser::AssetBrowserMode mode);

    void SetFavoritesWindowHeight(int height);

Q_SIGNALS:
    void SizeChangedSignal(int newWidth);

protected:
    void resizeEvent(QResizeEvent* resizeEvent) override;

private:
    void UpdateDisplayInfo();
    void SetNarrowMode(bool narrow);
    void SelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    int GetSelectionCount();

protected slots:
    void CreateToolsMenu();
    void AddCreateMenu();
    void SetTreeViewMode();
    void SetListViewMode();
    void UpdateWidgetAfterFilter();
    void SetTwoColumnMode(QWidget* viewToShow);
    void SetOneColumnMode();
    void AddFavoriteSearchButtonPressed();
    void AddFavoriteEntriesButtonPressed();
    void OnDoubleClick(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry);

private:
    QScopedPointer<Ui::AzAssetBrowserWindowClass> m_ui;
    QScopedPointer<AzToolsFramework::AssetBrowser::AssetBrowserFilterModel> m_filterModel;
    QScopedPointer<AzToolsFramework::AssetBrowser::AssetBrowserListModel> m_listModel;
    AzToolsFramework::AssetBrowser::AssetBrowserModel* m_assetBrowserModel;
    QMenu* m_toolsMenu = nullptr;
    QMenu* m_createMenu = nullptr;
    QAction* m_treeViewMode = nullptr;
    QAction* m_listViewMode = nullptr;
    AzToolsFramework::AssetBrowser::AssetBrowserDisplayState m_assetBrowserDisplayState =
        AzToolsFramework::AssetBrowser::AssetBrowserDisplayState::ListViewMode;
    AzToolsFramework::AssetBrowser::AssetBrowserMode m_currentMode = AzToolsFramework::AssetBrowser::AssetBrowserMode::ThumbnailView;

    //! Updates breadcrumbs with the selectedEntry relative path if it's a folder or with the
    //! relative path of the first folder parent of the passed entry.
    //! Clears breadcrumbs if nullptr is passed or there's no folder parent.
    void UpdateBreadcrumbs(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* selectedEntry) const;
    bool m_inNarrowMode = false;

private Q_SLOTS:
    void CurrentIndexChangedSlot(const QModelIndex& idx) const;
    void DoubleClickedItem(const QModelIndex& element);
    void BreadcrumbsPathChangedSlot(const QString& path) const;
    void OnFilterCriteriaChanged();
};

extern const char* AZ_ASSET_BROWSER_PREVIEW_NAME;
