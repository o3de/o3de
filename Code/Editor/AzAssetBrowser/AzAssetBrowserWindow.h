/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>

#include <QWidget>
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
        class AssetBrowserFilterModel;
        class AssetBrowserTableModel;
        class AssetBrowserModel;
        class AssetBrowserTableFilterModel;

        enum class AssetBrowserDisplayState : int
        {
            StandardMode,
            SearchViewMode,
            Invalid
        };
    }
}

class AzAssetBrowserWindow
    : public QWidget
{
    Q_OBJECT
public:
    AZ_CLASS_ALLOCATOR(AzAssetBrowserWindow, AZ::SystemAllocator, 0);

    explicit AzAssetBrowserWindow(QWidget* parent = nullptr);
    virtual ~AzAssetBrowserWindow();

    void SelectAsset(const QString& assetPath);

    static void RegisterViewClass();

    static QObject* createListenerForShowAssetEditorEvent(QObject* parent);

private:
    void OnInitViewToggleButton();
    void UpdateDisplayInfo();
protected slots:
    void CreateSwitchViewMenu();
    void SetStandardAssetBrowserMode();
    void SetSearchViewAssetBrowserMode();
    void UpdateTableModelAfterFilter();
    void SetTableViewVisibleAfterFilter();

private:

    QScopedPointer<Ui::AzAssetBrowserWindowClass> m_ui;
    QScopedPointer<AzToolsFramework::AssetBrowser::AssetBrowserFilterModel> m_filterModel;
    QScopedPointer<AzToolsFramework::AssetBrowser::AssetBrowserTableModel> m_tableModel;
    AzToolsFramework::AssetBrowser::AssetBrowserModel* m_assetBrowserModel;
    AZStd::unique_ptr<QMenu> m_viewSwitchMenu;
    AZStd::unique_ptr <QAction> m_standardAssetBrowserMode;
    AZStd::unique_ptr <QAction> m_searchViewAssetBrowserMode;
    AzToolsFramework::AssetBrowser::AssetBrowserDisplayState m_assetBrowserDisplayState;
    
    void UpdatePreview() const;

private Q_SLOTS:
    void SelectionChangedSlot(const QItemSelection& selected, const QItemSelection& deselected) const;
    void DoubleClickedItem(const QModelIndex& element);
};

extern const char* AZ_ASSET_BROWSER_PREVIEW_NAME;
