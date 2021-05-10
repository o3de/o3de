/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

    QScopedPointer<Ui::AzAssetBrowserWindowClass> m_ui;
    QScopedPointer<AzToolsFramework::AssetBrowser::AssetBrowserFilterModel> m_filterModel;
    QScopedPointer<AzToolsFramework::AssetBrowser::AssetBrowserTableModel> m_tableModel;
    QScopedPointer<AzToolsFramework::AssetBrowser::AssetBrowserTableFilterModel> m_tableFilterModel;
    AzToolsFramework::AssetBrowser::AssetBrowserModel* m_assetBrowserModel;
    
    void UpdatePreview() const;

private Q_SLOTS:
    void SelectionChangedSlot(const QItemSelection& selected, const QItemSelection& deselected) const;
    void DoubleClickedItem(const QModelIndex& element);
    void SwitchDisplayView(const int state);
};

extern const char* AZ_ASSET_BROWSER_PREVIEW_NAME;
