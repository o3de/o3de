/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "native/utilities/ApplicationManagerBase.h"
#include "native/utilities/AssetUtilEBusHelper.h"
#include "native/FileWatcher/FileWatcher.h"
#include <QMap>
#include <QAtomicInt>
#include <QFileSystemWatcher>
#include <AzCore/UserSettings/UserSettingsProvider.h>
#include <native/ui/MainWindow.h>
#include <QSystemTrayIcon>
#endif

class ConnectionManager;
class IniConfiguration;
class ApplicationServer;
class FileServer;

namespace AssetProcessor
{
    class AssetRequestHandler;
}

class GUIApplicationManager;

struct ErrorCollector
{
    explicit ErrorCollector(QWidget* parent = nullptr) : m_parent(parent){}
    ~ErrorCollector();

    void AddError(AZStd::string message);

    QWidget* m_parent{};
    QStringList m_errorMessages;
};

//! This class is the Application manager for the GUI Mode


class GUIApplicationManager
    : public ApplicationManagerBase
{
    Q_OBJECT
public:
    GUIApplicationManager(int* argc, char*** argv, QObject* parent = nullptr);
    GUIApplicationManager(int* argc, char*** argv, AZ::ComponentApplicationSettings componentAppSettings);
    GUIApplicationManager(int* argc, char*** argv, QObject* parent, AZ::ComponentApplicationSettings componentAppSettings);
    ~GUIApplicationManager() override;

    ApplicationManager::BeforeRunStatus BeforeRun() override;
    IniConfiguration* GetIniConfiguration() const;
    FileServer* GetFileServer() const;

    bool Run() override;
    ////////////////////////////////////////////////////
    ///MessageInfoBus::Listener interface///////////////
    void NegotiationFailed() override;
    void OnAssetFailed(const AZStd::string& sourceFileName) override;
    void OnErrorMessage(const char* error) override;
    ///////////////////////////////////////////////////

    //! TraceMessageBus::Handler
    bool OnError(const char* window, const char* message) override;
    bool OnAssert(const char* message) override;

    WId GetWindowId() const override;

private:
    bool Activate() override;
    bool PostActivate() override;
    void CreateQtApplication() override;
    bool InitApplicationServer() override;
    void InitConnectionManager() override;
    void InitIniConfiguration();
    void DestroyIniConfiguration();
    void InitFileServer();
    void DestroyFileServer();
    void Destroy() override;

Q_SIGNALS:
    void ShowWindow();

protected Q_SLOTS:
    void FileChanged(QString path);
    void DirectoryChanged(QString path);
    void ShowMessageBox(QString title, QString msg, bool isCritical);
    void ShowTrayIconMessage(QString msg);
    void ShowTrayIconErrorMessage(QString msg);
    void QuitRequested() override;

private:
    bool Restart();

    void Reflect() override;
    const char* GetLogBaseName() override;
    ApplicationManager::RegistryCheckInstructions PopupRegistryProblemsMessage(QString warningText) override;
    void InitSourceControl() override;
    bool GetShouldExitOnIdle() const override;

    IniConfiguration* m_iniConfiguration = nullptr;
    FileServer* m_fileServer = nullptr;

    QFileSystemWatcher m_qtFileWatcher;
    AZ::UserSettingsProvider m_localUserSettings;
    bool m_messageBoxIsVisible = false;
    bool m_startedSuccessfully = true;

    QPointer<QSystemTrayIcon> m_trayIcon;
    QPointer<MainWindow> m_mainWindow;
    AZStd::unique_ptr<ErrorCollector> m_startupErrorCollector; // Collects errors during start up to display when startup has finished

    AZStd::chrono::steady_clock::time_point m_timeWhenLastWarningWasShown;
};
