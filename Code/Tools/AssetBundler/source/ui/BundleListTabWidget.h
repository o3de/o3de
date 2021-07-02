/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <source/ui/AssetBundlerTabWidget.h>

#include <QItemSelection>
#include <QModelIndex>
#include <QSharedPointer>
#include <QString>
#include <QStringListModel>
#include <QWidget>
#endif

namespace Ui
{
    class BundleListTabWidget;
}

namespace AssetBundler
{
    class BundleFileListModel;

    class BundleListTabWidget
        : public AssetBundlerTabWidget
    {
        Q_OBJECT
    public:
        explicit BundleListTabWidget(QWidget* parent, GUIApplicationManager* guiApplicationManager);
        virtual ~BundleListTabWidget() {}

        QString GetTabTitle() override { return tr("Completed Bundles"); }
        QString GetFileTypeDisplayName() override { return tr("Bundle"); }
        AssetBundlingFileType GetFileType() override { return AssetBundlingFileType::BundleFileType; }
        bool HasUnsavedChanges() override { return false; }
        void Reload() override;
        bool SaveCurrentSelection() override { return true; }
        bool SaveAll() override { return true; }
        void SetModelDataSource() override;
        AzQtComponents::TableView* GetFileTableView() override;
        QModelIndex GetSelectedFileTableIndex() override;
        AssetBundlerAbstractFileTableModel* GetFileTableModel() override;
        void SetActiveProjectLabel(const QString& labelText) override;
        void ApplyConfig() override;
        void FileSelectionChanged(
            const QItemSelection& /*selected*/ = QItemSelection(),
            const QItemSelection& /*deselected*/ = QItemSelection()) override;

    private:
        void ClearDisplayedBundleValues();

        QSharedPointer<Ui::BundleListTabWidget> m_ui;
        QSharedPointer<BundleFileListModel> m_fileTableModel;
        QModelIndex m_selectedFileTableIndex;
        QSharedPointer<QStringListModel> m_relatedBundlesListModel;
    };

}
