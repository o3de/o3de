/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <source/models/AssetBundlerAbstractFileTableModel.h>
#include <source/models/AssetBundlerFileTableFilterModel.h>

#include <source/utils/GUIApplicationManager.h>

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzQtComponents/Components/Widgets/TableView.h>

#include <QLabel>
#include <QWidget>
#include <QItemSelection>
#endif

namespace AssetBundler
{
    // Keys for the corresponding scan path lists in the asset bundling settings file
    extern const QString AssetBundlingFileTypes[];

    extern const int MarginSize;

    class AssetBundlerTabWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit AssetBundlerTabWidget(QWidget* parent, GUIApplicationManager* guiApplicationManager);
        virtual ~AssetBundlerTabWidget();

        void Activate();

        virtual QString GetTabTitle() = 0;

        virtual QString GetFileTypeDisplayName() = 0;

        virtual AssetBundlingFileType GetFileType() = 0;

        virtual bool HasUnsavedChanges() = 0;

        //! Reload all the files on display.
        virtual void Reload() = 0;

        //! Saves the selected file to disk.
        //! @return true on successful save, or throws an error message and returns false on a failed save.
        virtual bool SaveCurrentSelection() = 0;

        //! Saves all modified files to disk.
        //! @return true on successful save, or throws an error message and returns false on a failed save.
        virtual bool SaveAll() = 0;

        //! Set watched folders and files for the model
        virtual void SetModelDataSource() = 0;

        virtual AzQtComponents::TableView* GetFileTableView() = 0;

        virtual QModelIndex GetSelectedFileTableIndex() = 0;

        virtual AssetBundlerAbstractFileTableModel* GetFileTableModel() = 0;

        virtual void SetActiveProjectLabel(const QString& labelText) = 0;

        virtual void ApplyConfig() = 0;

        virtual void FileSelectionChanged(
            const QItemSelection& /*selected*/ = QItemSelection(),
            const QItemSelection& /*deselected*/ = QItemSelection()) = 0;

        static void InitAssetBundlerSettings(const char* currentProjectFolderPath);

        void AddScanPathToAssetBundlerSettings(AssetBundlingFileType fileType, AZStd::string filePath);

        GUIApplicationManager* GetGUIApplicationManager() { return m_guiApplicationManager; }

    protected:
        void OnFileTableContextMenuRequested(const QPoint& pos);
        //! Asks the user to confirm they wish to permanently delete the selected file, then removes it from disk and memory.
        void OnDeleteSelectedFileRequested();
        bool ConfirmFileDeleteDialog(const QString& absoluteFilePath);
        void ReadScanPathsFromAssetBundlerSettings(AssetBundlingFileType fileType);
        void RemoveScanPathFromAssetBundlerSettings(AssetBundlingFileType fileType, const QString& filePath);

        GUIApplicationManager* m_guiApplicationManager = nullptr;
        QScopedPointer<AssetBundler::AssetBundlerFileTableFilterModel> m_fileTableFilterModel;

        QSet<QString> m_watchedFolders;
        QSet<QString> m_watchedFiles;
        AZStd::unordered_map<AZStd::string, AZStd::string> m_filePathToGemNameMap;

    private slots:
        void OnUpdateTab(const AZStd::string& path);
        void OnUpdateFiles(AssetBundlingFileType fileType, const AZStd::vector<AZStd::string>& absoluteFilePaths);

    private:
        void SetupContextMenu();
        void AddScanPathToAssetBundlerSettings(AssetBundlingFileType fileType, const QString& filePath);
        void ReadAssetBundlerSettings(const AZStd::string& filePath, AssetBundlingFileType fileType);

        static AZStd::string GetAssetBundlerUserSettingsFile(const char* currentProjectFolderPath);
        static AZStd::string GetAssetBundlerCommonSettingsFile(const char* currentProjectFolderPath);
        static void CreateEmptyAssetBundlerSettings(const AZStd::string& filePath);
    };

} // namespace AssetBundler
