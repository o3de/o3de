/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/Application/ToolsApplication.h>

#include <type_traits>
#include <QList>
#include <QString>
#include <QObject>
#include <QDir>
#include <QTimer>
#include <QDateTime>
#include "native/assetprocessor.h"
#endif

class QCoreApplication;

namespace AZ
{
    class Entity;
}

namespace AssetProcessor
{
    class ThreadWorker;
}

class AssetProcessorAZApplication
    : public QObject
    , public AzToolsFramework::ToolsApplication
{
    Q_OBJECT
public:
    AZ_CLASS_ALLOCATOR(AssetProcessorAZApplication, AZ::SystemAllocator)
    AssetProcessorAZApplication(int* argc, char*** argv, QObject* parent = nullptr);
    AssetProcessorAZApplication(int* argc, char*** argv, QObject* parent, AZ::ComponentApplicationSettings componentAppSettings);
    AssetProcessorAZApplication(int* argc, char*** argv, AZ::ComponentApplicationSettings componentAppSettings);

    ~AssetProcessorAZApplication() override = default;
    /////////////////////////////////////////////////////////
    //// AzFramework::Application overrides
    AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    void RegisterCoreComponents() override;
    void ResolveModulePath(AZ::OSString& modulePath) override;
    void SetSettingsRegistrySpecializations(AZ::SettingsRegistryInterface::Specializations& specializations) override;
    ///////////////////////////////////////////////////////////

Q_SIGNALS:
    void AssetProcessorStatus(AssetProcessor::AssetProcessorStatusEntry entry);

private:
    AZ::ModuleManagerRequests::PreModuleLoadEvent::Handler m_preModuleLoadHandler;
};

struct ApplicationDependencyInfo;

//This global function is required, if we want to use uuid as a key in a QSet
uint qHash(const AZ::Uuid& key, uint seed = 0);

//! This class allows you to register any number of objects to it
//! and when quit is requested, it will send a signal "QuitRequested()" to the registered object.
//! (You must implement this slot in your object!)
//! It will then expect each of those objects to send it the "ReadyToQuit(QObject*)" message when its ready
//! once every object is ready, qApp() will be told to quit.
//! the QObject parameter is the object that was originally registered and serves as the handle.
//! if your registered object is destroyed, it will automatically remove it from the list for you, no need to
//! unregister.

class ApplicationManager
    : public QObject
{
    Q_OBJECT
public:
    //! This enum is used by the BeforeRun method  and is useful in deciding whether we can run the application
    //! or whether we need to exit the application either because of an error or because we are restarting
    enum BeforeRunStatus
    {
        Status_Success = 0,
        Status_Restarting,
        Status_Failure,
    };
    ApplicationManager(int* argc, char*** argv, QObject* parent = nullptr);
    ApplicationManager(int* argc, char*** argv, AZ::ComponentApplicationSettings componentAppSettings);
    ApplicationManager(int* argc, char*** argv, QObject* parent, AZ::ComponentApplicationSettings componentAppSettings);
    virtual ~ApplicationManager();
    //! Prepares all the prerequisite needed for the main application functionality
    //! For eg Starts the AZ Framework,Activates logging ,Initialize Qt etc
    //! This method can return the following states success,failure and restarting.The latter two will cause the application to exit.
    virtual ApplicationManager::BeforeRunStatus BeforeRun();
    //! This method actually runs the main functionality of the application ,if BeforeRun method succeeds
    virtual bool Run() = 0;
    //! Returns a pointer to the QCoreApplication
    QCoreApplication* GetQtApplication();

    QDir GetSystemRoot() const;
    QString GetProjectPath() const;

    void RegisterComponentDescriptor(const AZ::ComponentDescriptor* descriptor);

    enum class RegistryCheckInstructions
    {
        Continue,
        Exit,
        Restart,
    };
    RegistryCheckInstructions CheckForRegistryProblems(QWidget* parentWidget, bool showPopupMessage);
    virtual bool IsAssetProcessorManagerIdle() const = 0;

Q_SIGNALS:
    void AssetProcessorStatusChanged(AssetProcessor::AssetProcessorStatusEntry entry);

public Q_SLOTS:
    void ReadyToQuit(QObject* source);
    virtual void QuitRequested();
    void ObjectDestroyed(QObject* source);
    void Restart();

private Q_SLOTS:
    void CheckQuit();
    void CheckForUpdate();

protected:
    //! Deactivate all your class member objects in this method
    virtual void Destroy() = 0;
    //! Prepares Qt Directories,Install Qt Translator etc
    virtual bool Activate();
    //! Runs late stage set up code
    virtual bool PostActivate();
    //! Override this method to create either QApplication or QCoreApplication
    virtual void CreateQtApplication() = 0;

    QString GetOrganizationName() const;
    QString GetApplicationName() const;

    void RegisterObjectForQuit(QObject* source, bool insertInFront = false);
    bool NeedRestart() const;
    void addRunningThread(AssetProcessor::ThreadWorker* thread);

    template<class BuilderClass>
    void RegisterInternalBuilder(const QString& builderName);

    //! Load the Modules (Such as Gems) and have them be reflected.
    bool ActivateModules();
    void PopulateApplicationDependencies();
    bool InitiatedShutdown() const;

    bool m_duringStartup = true;
    AssetProcessorAZApplication m_frameworkApp;
    QCoreApplication* m_qApp = nullptr;

    virtual void Reflect() {}
    virtual const char* GetLogBaseName() = 0;
    virtual RegistryCheckInstructions PopupRegistryProblemsMessage(QString warningText) = 0;
private:
    bool StartAZFramework();

    // QuitPair - Object pointer and "is ready" boolean pair.
    typedef QPair<QObject*, bool> QuitPair;
    QList<QuitPair> m_objectsToNotify;
    bool m_duringShutdown = false;
    QList<ApplicationDependencyInfo*> m_appDependencies;
    QList<QString> m_filesOfInterest;
    QList<AssetProcessor::ThreadWorker*> m_runningThreads;
    QTimer m_updateTimer;
    bool m_needRestart = false;
    bool m_queuedCheckQuit = false;
    QDir m_systemRoot;
    AZ::Entity* m_entity = nullptr;

};

///This class stores all the information of files that
/// we need to monitor for relaunching assetprocessor
struct ApplicationDependencyInfo
{
    QString m_fileName;
    QDateTime m_timestamp;

    ApplicationDependencyInfo(QString fileName, QDateTime timestamp)
        : m_fileName(fileName)
        , m_timestamp(timestamp)
    {
    }

public:
    QString FileName() const;
    void SetFileName(QString FileName);
    QDateTime Timestamp() const;
    void SetTimestamp(const QDateTime& Timestamp);
};
