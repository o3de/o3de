/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <source/ui/AssetBundlerTabWidget.h>

#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Platform/PlatformDefaults.h>

#include <QDir>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QSharedPointer>
#include <QString>
#include <QWidget>
#endif

namespace Ui
{
    class SeedTabWidget;
}

class QFileSystemModel;
class QStringListModel;

namespace AssetBundler
{
    class GUIApplicationManager;

    class SeedListFileTableModel;
    class SeedListTableModel;

    class NewFileDialog;
    class EditSeedDialog;
    class AddSeedDialog;

    class SeedTabWidget
        : public AssetBundlerTabWidget
        , public AZ::Debug::TraceMessageBus::Handler
    {
        Q_OBJECT

    public:
        explicit SeedTabWidget(QWidget* parent, GUIApplicationManager* guiApplicationManager, const QString& assetBundlingDirectory);
        virtual ~SeedTabWidget();

        QString GetTabTitle() override { return tr("Seeds"); }

        QString GetFileTypeDisplayName() override { return tr("Seed List file"); }

        AssetBundlingFileType GetFileType() override { return AssetBundlingFileType::SeedListFileType; }

        bool HasUnsavedChanges() override;

        void Reload() override;

        bool SaveCurrentSelection() override;

        bool SaveAll() override;

        void SetModelDataSource() override;

        AzQtComponents::TableView* GetFileTableView() override;

        QModelIndex GetSelectedFileTableIndex() override;

        AssetBundlerAbstractFileTableModel* GetFileTableModel() override;

        void SetActiveProjectLabel(const QString& labelText) override;

        void ApplyConfig() override;

        void FileSelectionChanged(
            const QItemSelection& /*selected*/ = QItemSelection(),
            const QItemSelection& /*deselected*/ = QItemSelection()) override;

        void UncheckSelectDefaultSeedListsCheckBox();

        void SetGenerateAssetListsButtonEnabled(bool isEnabled);

        /////////////////////////////////////////////////////////
        // TraceMessageBus overrides
        bool OnPreError(const char* window, const char* fileName, int line, const char* func, const char* message) override;
        bool OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message) override;
        /////////////////////////////////////////////////////////

    private:

        void OnNewFileButtonPressed();

        void OnSelectDefaultSeedListsCheckBoxChanged();

        void OnGenerateAssetListsButtonPressed();

        void OnEditSeedButtonPressed();
        void OnEditAllButtonPressed();

        void OnAddSeedButtonPressed();
        void OnRemoveSeedButtonPressed();

        void OnSeedListContentsTableContextMenuRequested(const QPoint& pos);

        QSharedPointer<Ui::SeedTabWidget> m_ui;
        QDir m_assetBundlingFolder;

        QSharedPointer<SeedListFileTableModel> m_fileTableModel;
        QModelIndex m_selectedFileTableIndex;

        QSharedPointer<NewFileDialog> m_generateAssetListsDialog;

        QSharedPointer<AssetBundler::AssetBundlerFileTableFilterModel> m_seedListContentsFilterModel;
        QSharedPointer<SeedListTableModel> m_seedListContentsModel;

        QSharedPointer<EditSeedDialog> m_editSeedDialog;
        QSharedPointer<AddSeedDialog> m_addSeedDialog;

        bool m_hasWarningsOrErrors = false;
    };
} // namespace AssetBundler
