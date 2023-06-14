/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <source/ui/AssetBundlerTabWidget.h>

#include <source/utils/GUIApplicationManager.h>
#include <source/utils/utils.h>

#include <AzCore/Utils/Utils.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzQtComponents/Utilities/DesktopUtilities.h>

#include <QApplication>
#include <QClipboard>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMenu>
#include <QMessageBox>

namespace AssetBundler
{
    constexpr AZ::IO::PathView AssetBundlerCommonSettingsFile = "AssetBundlerCommonSettings.json";
    constexpr AZ::IO::PathView AssetBundlerUserSettingsFile = "AssetBundlerUserSettings.json";

    const char ScanPathsKey[] = "ScanPaths";

    const QString AssetBundlingFileTypes[] =
    {
        "SeedLists",
        "AssetLists",
        "BundleSettings",
        "Bundles",
        "Rules"
    };

    const int MarginSize = 10;

    AssetBundlerTabWidget::AssetBundlerTabWidget(QWidget* parent, GUIApplicationManager* guiApplicationManager)
        : QWidget(parent)
        , m_guiApplicationManager(guiApplicationManager)
    {
        connect(m_guiApplicationManager, &GUIApplicationManager::UpdateTab, this, &AssetBundlerTabWidget::OnUpdateTab);
        connect(m_guiApplicationManager, &GUIApplicationManager::UpdateFiles, this, &AssetBundlerTabWidget::OnUpdateFiles);

        QAction* deleteFileAction = new QAction(tr("Delete"), this);
        addAction(deleteFileAction);
        deleteFileAction->setShortcut(QKeySequence::Delete);
        connect(deleteFileAction, &QAction::triggered, this, &AssetBundlerTabWidget::OnDeleteSelectedFileRequested);
    }

    AssetBundlerTabWidget::~AssetBundlerTabWidget()
    {
        m_guiApplicationManager = nullptr;
    }

    void AssetBundlerTabWidget::Activate()
    {
        SetActiveProjectLabel(tr("Active Project: %1").arg(m_guiApplicationManager->GetCurrentProjectName().c_str()));

        SetupContextMenu();
        Reload();

        connect(GetFileTableView()->header(),
            &QHeaderView::sortIndicatorChanged,
            m_fileTableFilterModel.get(),
            &AssetBundlerFileTableFilterModel::sort);
        GetFileTableView()->header()->setSortIndicatorShown(true);
        // Setting this in descending order will ensure the most recent files are at the top
        GetFileTableView()->header()->setSortIndicator(GetFileTableModel()->GetTimeStampColumnIndex(), Qt::DescendingOrder);
        GetFileTableView()->setSortingEnabled(true);
    }

    void AssetBundlerTabWidget::InitAssetBundlerSettings(const char* currentProjectFolderPath)
    {
        AZStd::string commonSettingsPath = GetAssetBundlerCommonSettingsFile(currentProjectFolderPath);
        if (!AZ::IO::FileIOBase::GetInstance()->Exists(commonSettingsPath.c_str()))
        {
            CreateEmptyAssetBundlerSettings(commonSettingsPath);
        }

        AZStd::string userSettingsPath = GetAssetBundlerUserSettingsFile(currentProjectFolderPath);
        if (!AZ::IO::FileIOBase::GetInstance()->Exists(userSettingsPath.c_str()))
        {
            CreateEmptyAssetBundlerSettings(userSettingsPath);
        }
    }

    void AssetBundlerTabWidget::OnFileTableContextMenuRequested(const QPoint& pos)
    {
        AZStd::string selectedFileAbsolutePath(GetFileTableModel()->GetFileAbsolutePath(GetSelectedFileTableIndex()));

        QMenu* contextMenu = new QMenu(this);
        contextMenu->setToolTipsVisible(true);

        bool arePathOperationsEnabled = !selectedFileAbsolutePath.empty();
        QString emptyPathToolTip(tr("This file is not present on-disk."));

        QAction* action = contextMenu->addAction(AzQtComponents::fileBrowserActionName(), [=]()
            {
                AzQtComponents::ShowFileOnDesktop(selectedFileAbsolutePath.c_str());
            });
        if (arePathOperationsEnabled)
        {
            action->setToolTip(tr("Shows the location of this file on your computer"));
        }
        else
        {
            action->setToolTip(emptyPathToolTip);
            action->setEnabled(false);
        }

        action = contextMenu->addAction(tr("Copy Path to Clipboard"), [=]()
            {
                QApplication::clipboard()->setText(QString(selectedFileAbsolutePath.c_str()));
            });
        if (arePathOperationsEnabled)
        {
            action->setToolTip(tr("Copies the absolute path of this file to your Clipboard"));
        }
        else
        {
            action->setToolTip(emptyPathToolTip);
            action->setEnabled(false);
        }

        contextMenu->addSeparator();

        // We can't use the same Delete action as the constructor because we need to modify a lot of values
        action = new QAction(tr("Delete"), this);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        action->setShortcut(QKeySequence::Delete);
        connect(action, &QAction::triggered, this, &AssetBundlerTabWidget::OnDeleteSelectedFileRequested);
        if (arePathOperationsEnabled)
        {
            action->setToolTip(tr("Deletes the selected file from disk."));
        }
        else
        {
            action->setToolTip(emptyPathToolTip);
            action->setEnabled(false);
        }
        contextMenu->addAction(action);

        contextMenu->exec(GetFileTableView()->mapToGlobal(pos));
        delete contextMenu;
    }

    void AssetBundlerTabWidget::OnDeleteSelectedFileRequested()
    {
        AZStd::string selectedFileAbsolutePath(GetFileTableModel()->GetFileAbsolutePath(GetSelectedFileTableIndex()));
        if (selectedFileAbsolutePath.empty())
        {
            return;
        }

        QString messageBoxText =
            QString(tr("Are you sure you would like to delete %1? \n\nThis will permanently delete the file.")).arg(QString(selectedFileAbsolutePath.c_str()));

        QMessageBox::StandardButton confirmDeleteFileResult =
            QMessageBox::question(this, QString(tr("Delete %1")).arg(GetFileTypeDisplayName()), messageBoxText);
        if (confirmDeleteFileResult != QMessageBox::StandardButton::Yes)
        {
            // User canceled out of the confirmation dialog
            return;
        }

        if (GetFileTableModel()->DeleteFile(GetSelectedFileTableIndex()))
        {
            RemoveScanPathFromAssetBundlerSettings(GetFileType(), selectedFileAbsolutePath.c_str());
        }
    }

    void AssetBundlerTabWidget::ReadScanPathsFromAssetBundlerSettings(AssetBundlingFileType fileType)
    {
        AZStd::string currentProjectFolderPath = m_guiApplicationManager->GetCurrentProjectFolder();
        ReadAssetBundlerSettings(GetAssetBundlerUserSettingsFile(currentProjectFolderPath.c_str()), fileType);
        ReadAssetBundlerSettings(GetAssetBundlerCommonSettingsFile(currentProjectFolderPath.c_str()), fileType);
    }

    void AssetBundlerTabWidget::AddScanPathToAssetBundlerSettings(AssetBundlingFileType fileType, AZStd::string filePath)
    {
        AZ::IO::Path defaultFolderPath;
        switch (fileType)
        {
        case AssetBundlingFileType::SeedListFileType:
            defaultFolderPath = m_guiApplicationManager->GetSeedListsFolder();
            break;
        case AssetBundlingFileType::AssetListFileType:
            defaultFolderPath = m_guiApplicationManager->GetAssetListsFolder();
            break;
        case AssetBundlingFileType::RulesFileType:
            defaultFolderPath = m_guiApplicationManager->GetRulesFolder();
            break;
        case AssetBundlingFileType::BundleSettingsFileType:
            defaultFolderPath = m_guiApplicationManager->GetBundleSettingsFolder();
            break;
        case AssetBundlingFileType::BundleFileType:
            defaultFolderPath = m_guiApplicationManager->GetBundlesFolder();
            break;
        default:
            AZ_Warning(AssetBundler::AppWindowName, false,
                "No default folder is defined for AssetBundlingFileType ( %i ).", static_cast<int>(fileType));
            break;
        }

        auto normalizedFilePath = AZ::IO::PathView(filePath).LexicallyNormal();
        defaultFolderPath = defaultFolderPath.LexicallyNormal();

        if (normalizedFilePath.IsRelativeTo(defaultFolderPath))
        {
            // file is already in a watched folder, no need to add it to the settings file
            return;
        }

        AddScanPathToAssetBundlerSettings(fileType, QString(filePath.c_str()));
    }

    void AssetBundlerTabWidget::AddScanPathToAssetBundlerSettings(AssetBundlingFileType fileType, const QString& filePath)
    {
        AZStd::string assetBundlerSettingsFileAbsolutePath =
            GetAssetBundlerUserSettingsFile(m_guiApplicationManager->GetCurrentProjectFolder().c_str());
        QJsonObject assetBundlerSettings = AssetBundler::ReadJson(assetBundlerSettingsFileAbsolutePath);
        QJsonObject scanPathsSettings = assetBundlerSettings[ScanPathsKey].toObject();
        QJsonArray scanPaths = scanPathsSettings[AssetBundlingFileTypes[fileType]].toArray();

        QFileInfo inputFileInfo(filePath);

        for (const QJsonValue scanPath : m_watchedFiles + m_watchedFolders)
        {
            auto scanFilePathStr = (AZ::IO::Path(AZStd::string_view{ AZ::Utils::GetEnginePath() })
                / scanPath.toString().toUtf8().data()).LexicallyNormal();

            // Check whether the file has already been watched
            // Get absolute file paths via QFileInfo to keep consistency in the letter case
            if (inputFileInfo.absoluteFilePath().startsWith(
                QFileInfo(scanFilePathStr.c_str()).absoluteFilePath()))
            {
                return;
            }
        }

        scanPaths.push_back(filePath);
        scanPathsSettings[AssetBundlingFileTypes[fileType]] = scanPaths;
        assetBundlerSettings[ScanPathsKey] = scanPathsSettings;
        AssetBundler::SaveJson(assetBundlerSettingsFileAbsolutePath, assetBundlerSettings);

        m_watchedFiles.insert(filePath);
        m_guiApplicationManager->AddWatchedPath(filePath);
    }

    void AssetBundlerTabWidget::RemoveScanPathFromAssetBundlerSettings(AssetBundlingFileType fileType, const QString& filePath)
    {
        AZStd::string assetBundlerSettingsFileAbsolutePath =
            GetAssetBundlerUserSettingsFile(m_guiApplicationManager->GetCurrentProjectFolder().c_str());
        QJsonObject assetBundlerSettings = AssetBundler::ReadJson(assetBundlerSettingsFileAbsolutePath);
        QJsonObject scanPathsSettings = assetBundlerSettings[ScanPathsKey].toObject();
        QJsonArray scanPaths = scanPathsSettings[AssetBundlingFileTypes[fileType]].toArray();

        for (auto itr = scanPaths.begin(); itr != scanPaths.end(); ++itr)
        {
            QJsonValueRef scanPathValueRef = *itr;
            auto scanPath = (AZ::IO::Path(AZStd::string_view{ AZ::Utils::GetEnginePath() })
                / scanPathValueRef.toString().toUtf8().data()).LexicallyNormal();

            // Check whether the file is being watched
            // Get absolute file paths via QFileInfo to keep consistency in the letter case
            if (QFileInfo(filePath).absoluteFilePath() == QFileInfo(scanPath.c_str()).absoluteFilePath())
            {
                itr = scanPaths.erase(itr);
                break;
            }
        }

        scanPathsSettings[AssetBundlingFileTypes[fileType]] = scanPaths;
        assetBundlerSettings[ScanPathsKey] = scanPathsSettings;
        AssetBundler::SaveJson(assetBundlerSettingsFileAbsolutePath, assetBundlerSettings);

        m_guiApplicationManager->RemoveWatchedPath(filePath);
    }

    void AssetBundlerTabWidget::OnUpdateTab(const AZStd::string& path)
    {
        if (m_watchedFolders.contains(path.c_str()) ||
            m_watchedFiles.contains(path.c_str()))
        {
            Reload();
        }
    }

    void AssetBundlerTabWidget::OnUpdateFiles(AssetBundlingFileType fileType, const AZStd::vector<AZStd::string>& absoluteFilePaths)
    {
        if (fileType == GetFileType())
        {
            GetFileTableModel()->ReloadFiles(absoluteFilePaths, m_filePathToGemNameMap);
            m_fileTableFilterModel->sort(m_fileTableFilterModel->sortColumn(), m_fileTableFilterModel->sortOrder());
            FileSelectionChanged();
        }
    }

    void AssetBundlerTabWidget::SetupContextMenu()
    {
        GetFileTableView()->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(GetFileTableView(),
            &QTreeView::customContextMenuRequested,
            this,
            &AssetBundlerTabWidget::OnFileTableContextMenuRequested);
    }

    void AssetBundlerTabWidget::ReadAssetBundlerSettings(const AZStd::string& filePath, AssetBundlingFileType fileType)
    {
        // Read the config file which contains the customized scan paths information
        AZStd::vector<AZStd::string> assetBundlerSettingsFileList;
        QJsonObject assetBundlerSettings = AssetBundler::ReadJson(filePath);
        QJsonObject scanPaths = assetBundlerSettings.find(ScanPathsKey).value().toObject();

        for (const QJsonValue scanPath : scanPaths[AssetBundlingFileTypes[fileType]].toArray())
        {
            auto absoluteScanPath = (AZ::IO::Path(AZStd::string_view{ AZ::Utils::GetEnginePath() })
                / scanPath.toString().toUtf8().data()).LexicallyNormal();

            if (AZ::IO::FileIOBase::GetInstance()->IsDirectory(absoluteScanPath.c_str()))
            {
                // The path specified in the config file is a directory
                m_watchedFolders.insert(absoluteScanPath.c_str());
            }
            else if (AZ::IO::FileIOBase::GetInstance()->Exists(absoluteScanPath.c_str()))
            {
                // The path specified in the config file is a file
                m_watchedFiles.insert(absoluteScanPath.c_str());
            }
        }
    }

    AZStd::string AssetBundlerTabWidget::GetAssetBundlerUserSettingsFile(const char* currentProjectFolderPath)
    {
        AZ::IO::Path absoluteFilePath = AZ::IO::Path(currentProjectFolderPath)
            / AZ::SettingsRegistryInterface::DevUserRegistryFolder
            / AssetBundlerUserSettingsFile;
        return absoluteFilePath.Native();
    }

    AZStd::string AssetBundlerTabWidget::GetAssetBundlerCommonSettingsFile(const char* currentProjectFolderPath)
    {
        AZ::IO::Path absoluteFilePath = AZ::IO::Path(currentProjectFolderPath)
            / AZ::SettingsRegistryInterface::RegistryFolder
            / AssetBundlerCommonSettingsFile;
        return absoluteFilePath.Native();
    }

    void AssetBundlerTabWidget::CreateEmptyAssetBundlerSettings(const AZStd::string& filePath)
    {
        QJsonObject assetBundlerSettings;
        QJsonObject scanPathSettings;

        for (const auto& fileType : AssetBundlingFileTypes)
        {
            scanPathSettings.insert(fileType, QJsonArray());
        }

        assetBundlerSettings.insert(ScanPathsKey, scanPathSettings);

        AssetBundler::SaveJson(filePath, assetBundlerSettings);
    }

} // namespace AssetBundler
#include <source/ui/moc_AssetBundlerTabWidget.cpp>
