/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <source/utils/GUIApplicationManager.h>

#include <source/ui/MainWindow.h>
#include <source/utils/utils.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/Asset/AssetCatalog.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalogBus.h>

#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>

#include <QApplication>
#include <QLocale>
#include <QStringList>

// Forward declare platform-specific functions
namespace Platform
{
    /// On Windows this will return the setting for our custom title bar
    /// On other platforms this will return the setting for using platform default
    /// This ensures that functions like Exit, Maximize, and Minimize appear in the right platform-specific style
    AzQtComponents::WindowDecorationWrapper::Option GetWindowDecorationWrapperOption();
} // namespace Platform

const char AssetBundlingFolderName [] = "AssetBundling";
const char SeedListsFolderName [] = "SeedLists";
const char AssetListsFolderName [] = "AssetLists";
const char RulesFolderName[] = "Rules";
const char BundleSettingsFolderName [] = "BundleSettings";
const char BundlesFolderName [] = "Bundles";

namespace AssetBundler
{

    GUIApplicationManager::Config GUIApplicationManager::loadConfig(QSettings& settings)
    {
        using namespace AzQtComponents;

        Config config = defaultConfig();

        // Error Log
        {
            ConfigHelpers::GroupGuard details(&settings, QStringLiteral("ErrorLogDetails"));
            ConfigHelpers::read<int>(settings, QStringLiteral("LogTypeColumnWidth"), config.logTypeColumnWidth);
            ConfigHelpers::read<int>(settings, QStringLiteral("LogSourceColumnWidth"), config.logSourceColumnWidth);
        }

        // General File Tables
        {
            ConfigHelpers::GroupGuard details(&settings, QStringLiteral("GeneralFileTableDetails"));
            ConfigHelpers::read<int>(settings, QStringLiteral("FileTableWidth"), config.fileTableWidth);
            ConfigHelpers::read<int>(settings, QStringLiteral("FileNameColumnWidth"), config.fileNameColumnWidth);
        }

        // Seeds Tab
        {
            ConfigHelpers::GroupGuard details(&settings, QStringLiteral("SeedsTabDetails"));
            ConfigHelpers::read<int>(settings, QStringLiteral("CheckBoxColumnWidth"), config.checkBoxColumnWidth);
            ConfigHelpers::read<int>(settings, QStringLiteral("SeedListFileNameColumnWidth"), config.seedListFileNameColumnWidth);
            ConfigHelpers::read<int>(settings, QStringLiteral("ProjectNameColumnWidth"), config.projectNameColumnWidth);
            ConfigHelpers::read<int>(settings, QStringLiteral("SeedListContentsNameColumnWidth"), config.seedListContentsNameColumnWidth);
        }

        // Asset Lists Tab
        {
            ConfigHelpers::GroupGuard details(&settings, QStringLiteral("AssetListsTabDetails"));
            ConfigHelpers::read<int>(settings, QStringLiteral("AssetListFileNameColumnWidth"), config.assetListFileNameColumnWidth);
            ConfigHelpers::read<int>(settings, QStringLiteral("AssetListPlatformColumnWidth"), config.assetListPlatformColumnWidth);
            ConfigHelpers::read<int>(settings, QStringLiteral("ProductAssetNameColumnWidth"), config.productAssetNameColumnWidth);
            ConfigHelpers::read<int>(
                settings, QStringLiteral("ProductAssetRelativePathColumnWidth"), config.productAssetRelativePathColumnWidth);
        }

        return config;
    }

    GUIApplicationManager::Config GUIApplicationManager::defaultConfig()
    {
        // These are used if the values can't be read from AssetBundlerConfig.ini.
        Config config;

        config.logTypeColumnWidth = 150;
        config.logSourceColumnWidth = 150;

        config.fileTableWidth = 250;
        config.fileNameColumnWidth = 150;

        config.checkBoxColumnWidth = 150;
        config.seedListFileNameColumnWidth = 150;
        config.projectNameColumnWidth = 150;
        config.seedListContentsNameColumnWidth = 150;

        config.assetListFileNameColumnWidth = 150;
        config.assetListPlatformColumnWidth = 150;
        config.productAssetNameColumnWidth = 150;
        config.productAssetRelativePathColumnWidth = 150;

        return config;
    }

    GUIApplicationManager::GUIApplicationManager(int* argc, char*** argv, QObject* parent)
        : ApplicationManager(argc, argv, parent)
    {
    }

    GUIApplicationManager::~GUIApplicationManager()
    {
        // Reset this before DestroyApplication, BusDisconnect needs to happen before Application::Stop() destroys the allocators.
        m_platformCatalogManager.reset();
    }

    bool GUIApplicationManager::Init()
    {
        m_isInitializing = true;
        // Initialize Asset Bundler Batch 
        ApplicationManager::Init();

        AZ::IO::FixedMaxPath engineRoot = GetEngineRoot();

        if (engineRoot.empty())
        {
            // Error has already been thrown
            return false;
        }

        // Determine the name of the current project
        auto projectOutcome = AssetBundler::GetCurrentProjectName();
        if (!projectOutcome.IsSuccess())
        {
            AZ_Error("AssetBundler", false, projectOutcome.GetError().c_str());
            return false;
        }
        m_currentProjectName = projectOutcome.GetValue();

        // Set up paths to the Project folder, Project Cache folder, and determine enabled platforms
        auto pathOutcome = InitializePaths();
        if (!pathOutcome.IsSuccess())
        {
            AZ_Error("AssetBundler", false, pathOutcome.GetError().c_str());
            return false;
        }

        // Set up platform-specific Asset Catalogs
        m_platformCatalogManager = AZStd::make_unique<AzToolsFramework::PlatformAddressedAssetCatalogManager>();

        // Define some application-level settings
        QApplication::setOrganizationName("O3DE");
        QApplication::setApplicationName("Asset Bundler");

        QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));

        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

        m_isInitializing = false;

        // Create the actual Qt Application
        m_qApp.reset(new QApplication(*GetArgC(), *GetArgV()));

        // Create the Main Window
        m_mainWindow.reset(new MainWindow(this));

        return true;
    }

    bool GUIApplicationManager::Run() 
    {
        // Set up the Style Manager
        AzQtComponents::StyleManager styleManager(qApp);
        styleManager.initialize(qApp, GetEngineRoot());

        AZ::IO::FixedMaxPath engineRoot(GetEngineRoot());
        QDir engineRootDir(engineRoot.c_str());
        AzQtComponents::StyleManager::addSearchPaths(
            QStringLiteral("style"),
            engineRootDir.filePath(QStringLiteral("Code/Tools/AssetBundler/source/ui/style")),
            QStringLiteral(":/AssetBundler/style"),
            engineRoot);
        AzQtComponents::StyleManager::setStyleSheet(m_mainWindow.data(), QStringLiteral("style:AssetBundler.qss"));

        AzQtComponents::ConfigHelpers::loadConfig<Config, GUIApplicationManager>(
            &m_fileWatcher,
            &m_config,
            QStringLiteral("style:AssetBundlerConfig.ini"),
            this,
            std::bind(&GUIApplicationManager::ApplyConfig, this));
        ApplyConfig();

        qApp->setWindowIcon(QIcon("style:AssetBundler-Icon-256x256@x2.ico"));

        // Set up the Main Window
        auto wrapper = new AzQtComponents::WindowDecorationWrapper(Platform::GetWindowDecorationWrapperOption());
        wrapper->setGuest(m_mainWindow.data());
        m_mainWindow->Activate();
        wrapper->show();
        m_mainWindow->show();

        qApp->setQuitOnLastWindowClosed(true);

        // Run the application
        return qApp->exec();
    }

    void GUIApplicationManager::AddWatchedPath(const QString& folderPath)
    {
        m_fileWatcher.addPath(folderPath);
    }

    void GUIApplicationManager::AddWatchedPaths(const QSet<QString>& folderPaths)
    {
        m_fileWatcher.addPaths(folderPaths.values());
    }

    void GUIApplicationManager::RemoveWatchedPath(const QString& path)
    {
        m_fileWatcher.removePath(path);
    }

    void GUIApplicationManager::RemoveWatchedPaths(const QSet<QString>& paths)
    {
        // Check whether the list is empty to get rid of the warning from Qt
        if (paths.isEmpty())
        {
            return;
        }

        m_fileWatcher.removePaths(paths.values());
    }

    bool GUIApplicationManager::OnPreError(
        const char* /*window*/,
        const char* /*fileName*/,
        int /*line*/,
        const char* /*func*/,
        const char* message)
    {
        // We want to display errors during initialization, then let the MainWindow handle errors during runtime
        if (m_isInitializing)
        {
            // These are fatal initialization errors, and the application will shut down after the user closes the message box
            m_qApp.reset(new QApplication(*GetArgC(), *GetArgV()));
            QMessageBox errorMessageBox;
            errorMessageBox.setWindowTitle("Asset Bundler");
            errorMessageBox.setText(message);
            errorMessageBox.setStandardButtons(QMessageBox::Ok);
            errorMessageBox.setDefaultButton(QMessageBox::Ok);
            errorMessageBox.exec();

            return true;
        }

        return false;
    }

    bool GUIApplicationManager::OnPreWarning(
        const char* /*window*/,
        const char* /*fileName*/,
        int /*line*/,
        const char* /*func*/,
        const char* /*message*/)
    {
        // Don't handle warnings, let the MainWindow print them
        return false;
    }


    bool GUIApplicationManager::OnPrintf(const char* /*window*/, const char* /*message*/)
    {
        // This is disabled during initialization to prevent a lot of message spam printed to the CLI that gets generated on setup
        return m_isInitializing;
    }

    AZ::Outcome<void, AZStd::string> GUIApplicationManager::InitializePaths()
    {
        // Calculate the path to the Cache for the current project
        auto pathOutcome = AssetBundler::GetProjectCacheFolderPath();
        if (!pathOutcome.IsSuccess())
        {
            return AZ::Failure(pathOutcome.GetError());
        }

        m_currentProjectCacheFolder = pathOutcome.GetValue().String();

        // Calculate the path to the current project folder
        pathOutcome = AssetBundler::GetProjectFolderPath();
        if (!pathOutcome.IsSuccess())
        {
            return AZ::Failure(pathOutcome.GetError());
        }

        m_currentProjectFolder = pathOutcome.GetValue().String();

        // Generate the AssetBundling folder inside the current project
        AzFramework::StringFunc::Path::ConstructFull(m_currentProjectFolder.c_str(), AssetBundlingFolderName, m_assetBundlingFolder);

        // Seed Lists folder
        AzFramework::StringFunc::Path::ConstructFull(m_assetBundlingFolder.c_str(), SeedListsFolderName, m_seedListsFolder);
        AZ::Outcome<void, AZStd::string> createPathOutcome = AssetBundler::MakePath(m_seedListsFolder);
        if (!createPathOutcome.IsSuccess())
        {
            return createPathOutcome;
        }

        // Asset Lists folder
        AzFramework::StringFunc::Path::ConstructFull(m_assetBundlingFolder.c_str(), AssetListsFolderName, m_assetListsFolder);
        createPathOutcome = AssetBundler::MakePath(m_assetListsFolder);
        if (!createPathOutcome.IsSuccess())
        {
            return createPathOutcome;
        }

        // Rules folder
        AzFramework::StringFunc::Path::ConstructFull(m_assetBundlingFolder.c_str(), RulesFolderName, m_rulesFolder);
        createPathOutcome = AssetBundler::MakePath(m_rulesFolder);
        if (!createPathOutcome.IsSuccess())
        {
            return createPathOutcome;
        }

        // Bundle Settings Folder
        AzFramework::StringFunc::Path::ConstructFull(m_assetBundlingFolder.c_str(), BundleSettingsFolderName, m_bundleSettingsFolder);
        createPathOutcome = AssetBundler::MakePath(m_bundleSettingsFolder);
        if (!createPathOutcome.IsSuccess())
        {
            return createPathOutcome;
        }

        // Bundles Folder
        AzFramework::StringFunc::Path::ConstructFull(m_assetBundlingFolder.c_str(), BundlesFolderName, m_bundlesFolder);
        createPathOutcome = AssetBundler::MakePath(m_bundlesFolder);
        if (!createPathOutcome.IsSuccess())
        {
            return createPathOutcome;
        }

        // Determine the enabled platforms
        const char* appRoot = nullptr;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(appRoot, &AzFramework::ApplicationRequests::GetAppRoot);
        m_enabledPlatforms = GetEnabledPlatformFlags(GetEngineRoot(), appRoot, AZ::Utils::GetProjectPath().c_str());

        // Determine which Gems are enabled for the current project
        if (!AzFramework::GetGemsInfo(m_gemInfoList, *m_settingsRegistry))
        {
            return AZ::Failure(AZStd::string::format("Failed to read Gems for project: %s\n", m_currentProjectName.c_str()));
        }

        QObject::connect(&m_fileWatcher, &QFileSystemWatcher::directoryChanged, this, &GUIApplicationManager::DirectoryChanged);
        QObject::connect(&m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &GUIApplicationManager::FileChanged);

        return AZ::Success();
    }

    void GUIApplicationManager::DirectoryChanged(const QString& directory)
    {
        UpdateTab(directory.toUtf8().data());
    }

    void GUIApplicationManager::FileChanged(const QString& path)
    {
        // FileChanged will only be called when engine or gem seed files are updated
        // Otherwilse DirectoryChanged should be triggered
        AZStd::string extension;
        AzFramework::StringFunc::Path::GetExtension(path.toUtf8().data(), extension);
        extension = extension.starts_with(".") ? extension.substr(1) : extension;
        if (extension == AzToolsFramework::AssetSeedManager::GetSeedFileExtension())
        {
            UpdateTab(GetSeedListsFolder());
        }

        // Many applications save an open file by writing a new file and then deleting the old one
        // Add the file path back if it has been removed from the watcher file list
        if (!m_fileWatcher.files().contains(path))
        {
            m_fileWatcher.addPath(path);
        }
    }

    void GUIApplicationManager::ApplyConfig()
    {
        m_mainWindow->ApplyConfig();
    }

} // namespace AssetBundler

#include <source/utils/moc_GUIApplicationManager.cpp>
