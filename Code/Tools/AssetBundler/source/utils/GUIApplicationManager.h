/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <source/utils/applicationManager.h>

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/vector.h>

#include <AzFramework/Asset/AssetCatalog.h>
#include <AzFramework/Platform/PlatformDefaults.h>

#include <QCoreApplication>
#include <QDir>
#include <QMap>
#include <QSettings>
#include <QSharedPointer>
#include <QString>
#include <QFileSystemWatcher>
#endif

namespace AssetBundler
{
    enum AssetBundlingFileType : int
    {
        SeedListFileType = 0,
        AssetListFileType,
        BundleSettingsFileType,
        BundleFileType,
        RulesFileType,
        NumBundlingFileTypes
    };

    class MainWindow;

    class GUIApplicationManager
        : public ApplicationManager
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(GUIApplicationManager, AZ::SystemAllocator)

        struct Config
        {
            // These default values are used if the values can't be read from AssetBundlerConfig.ini,
            // and the call to defaultConfig fails.

            // Error Log
            int logTypeColumnWidth = -1;
            int logSourceColumnWidth = -1;

            // General File Tables
            int fileTableWidth = -1;
            int fileNameColumnWidth = -1;

            // Seeds Tab
            int checkBoxColumnWidth = -1;
            int seedListFileNameColumnWidth = -1;
            int projectNameColumnWidth = -1;
            int seedListContentsNameColumnWidth = -1;

            // Asset Lists Tab
            int assetListFileNameColumnWidth = -1;
            int assetListPlatformColumnWidth = -1;
            int productAssetNameColumnWidth = -1;
            int productAssetRelativePathColumnWidth = -1;
        };

        /*!
         * Loads the button config data from a settings object.
         */
        static Config loadConfig(QSettings& settings);

        /*!
         * Returns default button config data.
         */
        static Config defaultConfig();

        explicit GUIApplicationManager(int* argc, char*** argv, QObject* parent = 0);
        virtual ~GUIApplicationManager();

        bool Init() override;

        bool Run() override;

        AZStd::string GetCurrentProjectFolder() { return m_currentProjectFolder; }
        AZStd::string GetAssetBundlingFolder() { return m_assetBundlingFolder; }
        AZStd::string GetSeedListsFolder() { return m_seedListsFolder; }
        AZStd::string GetAssetListsFolder() { return m_assetListsFolder; }
        AZStd::string GetRulesFolder() { return m_rulesFolder; }
        AZStd::string GetBundleSettingsFolder() { return m_bundleSettingsFolder; }
        AZStd::string GetBundlesFolder() { return m_bundlesFolder; }
        AZStd::string GetCurrentProjectCacheFolder() { return m_currentProjectCacheFolder; }

        AzFramework::PlatformFlags GetEnabledPlatforms() { return m_enabledPlatforms; }

        void AddWatchedPath(const QString& path);
        void AddWatchedPaths(const QSet<QString>& paths);
        void RemoveWatchedPath(const QString& path);
        void RemoveWatchedPaths(const QSet<QString>& paths);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Override the ApplicationManager TraceMessageBus methods so that messages go through MainWindow and not the CLI

        bool OnPreError(const char* window, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message) override;
        bool OnPreWarning(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* /*message*/) override;
        bool OnPrintf(const char* /*window*/, const char* /*message*/) override;
        ////////////////////////////////////////////////////////////////////////////////////////////

        const Config& GetConfig() { return m_config; }

    Q_SIGNALS:
        void ShowWindow();
        void UpdateTab(const AZStd::string& directory);
        void UpdateFiles(AssetBundlingFileType fileType, const AZStd::vector<AZStd::string>& absoluteFilePaths);

    protected Q_SLOTS:
        void DirectoryChanged(const QString& directory);
        void FileChanged(const QString& path);
        void ApplyConfig();

    private:
        /**
        * Generates directory information for all paths used in this tool
        * @return void on success, error message on failure
        */
        AZ::Outcome<void, AZStd::string> InitializePaths();

        QSharedPointer<QCoreApplication> m_qApp;

        Config m_config;

        QSharedPointer<MainWindow> m_mainWindow;

        AZStd::string m_currentProjectFolder;
        AZStd::string m_assetBundlingFolder;
        AZStd::string m_seedListsFolder;
        AZStd::string m_assetListsFolder;
        AZStd::string m_rulesFolder;
        AZStd::string m_bundleSettingsFolder;
        AZStd::string m_bundlesFolder;
        AZStd::string m_currentProjectCacheFolder;

        AzFramework::PlatformFlags m_enabledPlatforms = AzFramework::PlatformFlags::Platform_NONE;

        AZStd::unique_ptr<AzToolsFramework::PlatformAddressedAssetCatalogManager> m_platformCatalogManager;

        bool m_isInitializing = false;

        QFileSystemWatcher m_fileWatcher;
    };

} // namespace AssetBundler
