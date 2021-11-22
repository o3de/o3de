/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "native/utilities/ApplicationManager.h"

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>

#include <AzFramework/Logging/LoggingComponent.h>
#include <AzFramework/Asset/AssetSystemComponent.h>

#include <native/resourcecompiler/RCBuilder.h>
#include <native/utilities/StatsCapture.h>

#include <QLocale>
#include <QTranslator>
#include <QCommandLineParser>

#include <QSettings>

#include <AzFramework/Asset/AssetCatalogComponent.h>
#include <AzToolsFramework/Entity/EditorEntityFixupComponent.h>
#include <AzToolsFramework/ToolsComponents/ToolsAssetCatalogComponent.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserComponent.h>
#include <AzToolsFramework/SourceControl/PerforceComponent.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponent.h>

namespace AssetProcessor
{
    void MessageHandler(QtMsgType type, [[maybe_unused]] const QMessageLogContext& context, [[maybe_unused]] const QString& msg)
    {
        switch (type)
        {
        case QtDebugMsg:
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Qt-Debug: %s (%s:%u, %s)\n", msg.toUtf8().constData(), context.file, context.line, context.function);
            break;
        case QtWarningMsg:
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Qt-Warning: %s (%s:%u, %s)\n", msg.toUtf8().constData(), context.file, context.line, context.function);
            break;
        case QtCriticalMsg:
            AZ_Warning(AssetProcessor::ConsoleChannel, false, "Qt-Critical: %s (%s:%u, %s)\n", msg.toUtf8().constData(), context.file, context.line, context.function);
            break;
        case QtFatalMsg:
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Qt-Fatal: %s (%s:%u, %s)\n", msg.toUtf8().constData(), context.file, context.line, context.function);
            abort();
        }
    }

    //! we filter the main app logs to only include non-job-thread messages:
    class FilteredLogComponent
        : public AzFramework::LogComponent
    {
    public:
        void OutputMessage(AzFramework::LogFile::SeverityLevel severity, const char* window, const char* message) override
        {
            // if we receive an exception it means we are likely to crash.  in that case, even if it occurred in a job thread
            // it occurred in THIS PROCESS, which will now die.  So we log these even in the case of them happening in a job thread.
            if ((m_inException)||(severity == AzFramework::LogFile::SEV_EXCEPTION))
            {
                if (!m_inException)
                {
                    m_inException = true; // from this point on, consume all messages regardless of what severity they are.
                    AZ::Debug::Trace::HandleExceptions(false);
                }
                AzFramework::LogComponent::OutputMessage(AzFramework::LogFile::SEV_EXCEPTION, ConsoleChannel, message);
                // note that OutputMessage will only output to the log, we want this kind of info to make its way into the
                // regular stderr too
                fprintf(stderr, "Exception log: %s - %s", window, message);
                fflush(stderr);
                return;
            }

            if (AssetProcessor::GetThreadLocalJobId() != 0)
            {
                // we are in a job thread - return early to make it so that the global log file does not get this message
                // there will also be a log listener in the actual job log thread which will get the message too, and that one
                // will write it to the individual log.
                return;
            }

            AzFramework::LogComponent::OutputMessage(severity, window, message);
        }

    protected:
        bool m_inException = false;
    };
}

uint qHash(const AZ::Uuid& key, uint seed)
{
    (void) seed;
    return azlossy_caster(AZStd::hash<AZ::Uuid>()(key));
}

namespace AssetProcessorBuildTarget
{
    // Added a declaration of GetBuiderTargetName which retrieves the name of the build target
    // The build target changes depending on which shared library/executable ApplicationManager.cpp
    // is linked to
    AZStd::string_view GetBuildTargetName();
}


AssetProcessorAZApplication::AssetProcessorAZApplication(int* argc, char*** argv, QObject* parent)
    : QObject(parent)
    , AzToolsFramework::ToolsApplication(argc, argv)
{
    // The settings registry has been created at this point, so add the CMake target
    // specialization to the settings
    AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddBuildSystemTargetSpecialization(
        *AZ::SettingsRegistry::Get(), AssetProcessorBuildTarget::GetBuildTargetName());

    // Adding the PreModuleLoad event to the AssetProcessor application for logging when a gem loads
    m_preModuleLoadHandler = AZ::ModuleManagerRequests::PreModuleLoadEvent::Handler{
        []([[maybe_unused]] AZStd::string_view modulePath)
        {
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Loading (Gem) Module '%.*s'...\n", aznumeric_cast<int>(modulePath.size()), modulePath.data());
        }
    };

    m_preModuleLoadHandler.Connect(m_moduleManager->m_preModuleLoadEvent);
}

AZ::ComponentTypeList AssetProcessorAZApplication::GetRequiredSystemComponents() const
{
    AZ::ComponentTypeList components = AzFramework::Application::GetRequiredSystemComponents();

    for (auto iter = components.begin(); iter != components.end();)
    {
        if (*iter == azrtti_typeid<AzFramework::AssetSystem::AssetSystemComponent>() // AP does not need asset system component to handle AssetRequestBus calls
            || *iter == azrtti_typeid<AzFramework::AssetCatalogComponent>() // AP will use its own AssetCatalogComponent
            || *iter == AZ::Uuid("{624a7be2-3c7e-4119-aee2-1db2bdb6cc89}") // ScriptDebugAgent
            || *iter == AZ::Uuid("{CAF3A025-FAC9-4537-B99E-0A800A9326DF}") // InputSystemComponent
            || *iter == azrtti_typeid<AssetProcessor::ToolsAssetCatalogComponent>()
           )
        {
            // AP does not require the above components to be active
            iter = components.erase(iter);
        }
        else
        {
            ++iter;
        }
    }

    components.push_back(azrtti_typeid<AzToolsFramework::PerforceComponent>());
    components.push_back(azrtti_typeid<AzToolsFramework::Prefab::PrefabSystemComponent>());

    return components;
}

void AssetProcessorAZApplication::RegisterCoreComponents()
{
    AzToolsFramework::ToolsApplication::RegisterCoreComponents();

    RegisterComponentDescriptor(AzToolsFramework::EditorEntityFixupComponent::CreateDescriptor());
    RegisterComponentDescriptor(AzToolsFramework::AssetBrowser::AssetBrowserComponent::CreateDescriptor());
}

void AssetProcessorAZApplication::ResolveModulePath(AZ::OSString& modulePath)
{
   AssetProcessor::AssetProcessorStatusEntry entry(AssetProcessor::AssetProcessorStatus::Initializing_Gems, 0, QString(modulePath.c_str()));
   Q_EMIT AssetProcessorStatus(entry);

   AzFramework::Application::ResolveModulePath(modulePath);
}

void AssetProcessorAZApplication::SetSettingsRegistrySpecializations(AZ::SettingsRegistryInterface::Specializations& specializations)
{
    AzToolsFramework::ToolsApplication::SetSettingsRegistrySpecializations(specializations);
    specializations.Append("assetprocessor");
}

ApplicationManager::ApplicationManager(int* argc, char*** argv, QObject* parent)
    : QObject(parent)
    , m_frameworkApp(argc, argv)
{
    qInstallMessageHandler(&AssetProcessor::MessageHandler);
}

ApplicationManager::~ApplicationManager()
{
    // if any of the threads are running, destroy them
    // Any QObjects that have these ThreadWorker as parent will also be deleted
    if (m_runningThreads.size())
    {
        for (int idx = 0; idx < m_runningThreads.size(); idx++)
        {
            m_runningThreads.at(idx)->Destroy();
        }
    }
    for (int idx = 0; idx < m_appDependencies.size(); idx++)
    {
        delete m_appDependencies[idx];
    }

    // end stats capture (dump and shutdown)
    AssetProcessor::StatsCapture::Dump();
    AssetProcessor::StatsCapture::Shutdown();

    qInstallMessageHandler(nullptr);

    //deleting QCoreApplication/QApplication
    delete m_qApp;

    if (m_entity)
    {
        //Deactivate all the components
        m_entity->Deactivate();
        delete m_entity;
        m_entity = nullptr;
    }

    //Unregistering and deleting all the components
    EBUS_EVENT_ID(azrtti_typeid<AzFramework::LogComponent>(), AZ::ComponentDescriptorBus, ReleaseDescriptor);

    //Stop AZFramework
    m_frameworkApp.Stop();
    AZ::Debug::Trace::HandleExceptions(false);
}

bool ApplicationManager::InitiatedShutdown() const
{
    return m_duringShutdown;
}

void ApplicationManager::GetExternalBuilderFileList(QStringList& externalBuilderModules)
{
    externalBuilderModules.clear();

    static const char* builder_folder_name = "Builders";

    // LY_ASSET_BUILDERS is defined by the CMakeLists.txt. The asset builders add themselves to a variable that
    // is populated to allow selective building of those asset builder targets.
    // This allows left over Asset builders in the output directory to not be loaded by the AssetProcessor
#if !defined(LY_ASSET_BUILDERS)
    #error LY_ASSET_BUILDERS was not defined for ApplicationManager.cpp
#endif

    QDir builderDir = QDir::toNativeSeparators(QString(this->m_frameworkApp.GetExecutableFolder()));
    builderDir.cd(QString(builder_folder_name));
    if (builderDir.exists())
    {
        AZStd::vector<AZStd::string> tokens;
        AZ::StringFunc::Tokenize(AZStd::string_view(LY_ASSET_BUILDERS), tokens, ',');
        AZStd::string builderLibrary;
        for (const AZStd::string& token : tokens)
        {
            QString assetBuilderPath(token.c_str());
            if (builderDir.exists(assetBuilderPath))
            {
                externalBuilderModules.push_back(builderDir.absoluteFilePath(assetBuilderPath));
            }
        }
    }

    if (externalBuilderModules.empty())
    {
        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "AssetProcessor was unable to locate any external builders\n");
    }
}



QDir ApplicationManager::GetSystemRoot() const
{
    return m_systemRoot;
}
QString ApplicationManager::GetProjectPath() const
{
    auto projectPath = AZ::Utils::GetProjectPath();
    if (!projectPath.empty())
    {
        return QString::fromUtf8(projectPath.c_str(), aznumeric_cast<int>(projectPath.size()));
    }

    AZ_Warning("AssetUtils", false, "Unable to obtain the Project Path from the settings registry.");
    return {};
}

QCoreApplication* ApplicationManager::GetQtApplication()
{
    return m_qApp;
}

void ApplicationManager::RegisterObjectForQuit(QObject* source, bool insertInFront)
{
    Q_ASSERT(!m_duringShutdown);

    if (m_duringShutdown)
    {
        AZ_Warning(AssetProcessor::DebugChannel, false, "You may not register objects for quit during shutdown.\n");
        return;
    }

    QuitPair quitPair(source, false);
    if (!m_objectsToNotify.contains(quitPair))
    {
        if (insertInFront)
        {
            m_objectsToNotify.push_front(quitPair);
        }
        else
        {
            m_objectsToNotify.push_back(quitPair);
        }
        if (!connect(source, SIGNAL(ReadyToQuit(QObject*)), this, SLOT(ReadyToQuit(QObject*))))
        {
            AZ_Warning(AssetProcessor::DebugChannel, false, "ApplicationManager::RegisterObjectForQuit was passed an object of type %s which has no ReadyToQuit(QObject*) signal.\n", source->metaObject()->className());
        }
        connect(source, SIGNAL(destroyed(QObject*)), this, SLOT(ObjectDestroyed(QObject*)));
    }
}

void ApplicationManager::ObjectDestroyed(QObject* source)
{
    for (int notifyIdx = 0; notifyIdx < m_objectsToNotify.size(); ++notifyIdx)
    {
        if (m_objectsToNotify[notifyIdx].first == source)
        {
            m_objectsToNotify.erase(m_objectsToNotify.begin() + notifyIdx);
            if (m_duringShutdown)
            {
                if (!m_queuedCheckQuit)
                {
                    QTimer::singleShot(0, this, SLOT(CheckQuit()));
                    m_queuedCheckQuit = true;
                }
            }
            return;
        }
    }
}

void ApplicationManager::QuitRequested()
{
    if (m_duringShutdown)
    {
        AZ_TracePrintf(AssetProcessor::DebugChannel, "QuitRequested() - already during shutdown\n");
        return;
    }

    if (m_duringStartup)
    {
        AZ_TracePrintf(AssetProcessor::DebugChannel, "QuitRequested() - during startup - waiting\n");
        // if we're still starting up, spin until we're ready to shut down.
        QMetaObject::invokeMethod(this, "QuitRequested", Qt::QueuedConnection);
        return;
    }
    AZ_TracePrintf(AssetProcessor::DebugChannel, "QuitRequested() - ready!\n");
    m_duringShutdown = true;

    //Inform all the builders to shutdown
    EBUS_EVENT(AssetBuilderSDK::AssetBuilderCommandBus, ShutDown);

    // the following call will invoke on the main thread of the application, since its a direct bus call.
    EBUS_EVENT(AssetProcessor::ApplicationManagerNotifications::Bus, ApplicationShutdownRequested);

    // while it may be tempting to just collapse all of this to a bus call,  Qt Objects have the advantage of being
    // able to automatically queue calls onto their own thread, and a lot of these involved objects are in fact
    // on their own threads.  So even if we used a bus call we'd ultimately still have to invoke a queued
    // call there anyway.

    for (const QuitPair& quitter : m_objectsToNotify)
    {
        if (!quitter.second)
        {
            QMetaObject::invokeMethod(quitter.first, "QuitRequested", Qt::QueuedConnection);
        }
    }

    AZ_TracePrintf(AssetProcessor::ConsoleChannel, "App quit requested %d listeners notified.\n", m_objectsToNotify.size());

    if (!m_queuedCheckQuit)
    {
        QTimer::singleShot(0, this, SLOT(CheckQuit()));
        m_queuedCheckQuit = true;
    }
}

void ApplicationManager::CheckQuit()
{
    m_queuedCheckQuit = false;
    for (const QuitPair& quitter : m_objectsToNotify)
    {
        if (!quitter.second)
        {
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "App Quit: Object of type %s is not yet ready to quit.\n", quitter.first->metaObject()->className());
            return;
        }
    }
    AZ_TracePrintf(AssetProcessor::ConsoleChannel, "App quit requested, and all objects are ready.  Quitting app.\n");

    // We will now loop over all the running threads and destroy them
    // Any QObjects that have these ThreadWorker as parent will also be deleted
    for (int idx = 0; idx < m_runningThreads.size(); idx++)
    {
        m_runningThreads.at(idx)->Destroy();
    }

    m_runningThreads.clear();
    // all good.
    qApp->quit();
}

void ApplicationManager::CheckForUpdate()
{
    for (int idx = 0; idx < m_appDependencies.size(); ++idx)
    {
        ApplicationDependencyInfo* fileDependencyInfo = m_appDependencies[idx];
        QString fileName = fileDependencyInfo->FileName();
        QFileInfo fileInfo(fileName);
        if (fileInfo.exists())
        {
            QDateTime fileLastModifiedTime = fileInfo.lastModified();
            bool hasTimestampChanged = (fileDependencyInfo->Timestamp() != fileLastModifiedTime);
            if (hasTimestampChanged)
            {
                QuitRequested();
            }
        }
        else
        {
            // if one of the files is not present we construct a null datetime for it and
            // continue checking
            fileDependencyInfo->SetTimestamp(QDateTime());
        }
    }
}

void ApplicationManager::PopulateApplicationDependencies()
{
    connect(&m_updateTimer, SIGNAL(timeout()), this, SLOT(CheckForUpdate()));
    m_updateTimer.start(5000);

    QString currentDir(QCoreApplication::applicationDirPath());
    QDir dir(currentDir);
    QString applicationPath = QCoreApplication::applicationFilePath();

    m_filesOfInterest.push_back(applicationPath);

    // add some known-dependent files (this can be removed when they are no longer a dependency)
    // Note that its not necessary for any of these files to actually exist.  It is considered a "change" if they
    // change their file modtime, or if they go from existing to not existing, or if they go from not existing, to existing.
    // any of those should cause AP to drop.
    for (const QString& pathName : { "CrySystem",
                                     "SceneCore", "SceneData",
                                     "SceneBuilder", "AzQtComponents"
                                     })
    {
        QString pathWithPlatformExtension = pathName + QString(AZ_DYNAMIC_LIBRARY_EXTENSION);
        m_filesOfInterest.push_back(dir.absoluteFilePath(pathWithPlatformExtension));
    }

    // Get the external builder modules to add to the files of interest
    QStringList builderModuleFileList;
    GetExternalBuilderFileList(builderModuleFileList);
    for (const QString& builderModuleFile : builderModuleFileList)
    {
        m_filesOfInterest.push_back(builderModuleFile);
    }


    QDir assetRoot;
    AssetUtilities::ComputeAssetRoot(assetRoot);

    QString globalConfigPath = assetRoot.filePath("Registry/AssetProcessorPlatformConfig.setreg");
    m_filesOfInterest.push_back(globalConfigPath);

    QString gamePlatformConfigPath = QDir(AssetUtilities::ComputeProjectPath()).filePath("AssetProcessorGamePlatformConfig.setreg");
    m_filesOfInterest.push_back(gamePlatformConfigPath);

    // add app modules
    AZ::ModuleManagerRequestBus::Broadcast(&AZ::ModuleManagerRequestBus::Events::EnumerateModules,
        [this](const AZ::ModuleData& moduleData)
        {
            AZ::DynamicModuleHandle* handle = moduleData.GetDynamicModuleHandle();
            if (handle)
            {
                QFileInfo fi(handle->GetFilename().c_str());
                if (fi.exists())
                {
                    m_filesOfInterest.push_back(fi.absoluteFilePath());
                }
            }
            return true; // keep iterating.
        });

    //find timestamps of all the files
    for (int idx = 0; idx < m_filesOfInterest.size(); idx++)
    {
        QString fileName = m_filesOfInterest.at(idx);
        QFileInfo fileInfo(fileName);
        QDateTime fileLastModifiedTime = fileInfo.lastModified();
        ApplicationDependencyInfo* applicationDependencyInfo = new ApplicationDependencyInfo(fileName, fileLastModifiedTime);
        // if some file does not exist than null datetime will be stored
        m_appDependencies.push_back(applicationDependencyInfo);
    }
}

bool ApplicationManager::StartAZFramework()
{
    AzFramework::Application::Descriptor appDescriptor;
    AZ::ComponentApplication::StartupParameters params;

    QDir projectPath{ AssetUtilities::ComputeProjectPath() };
    if (!projectPath.exists("project.json"))
    {
        AZStd::string errorMsg = AZStd::string::format("Path '%s' is not a valid project path.", projectPath.path().toUtf8().constData());
        AssetProcessor::MessageInfoBus::Broadcast(&AssetProcessor::MessageInfoBus::Events::OnErrorMessage, errorMsg.c_str());
        return false;
    }

    QString projectName = AssetUtilities::ComputeProjectName();

    // Prevent loading of gems in the Create method of the ComponentApplication
    params.m_loadDynamicModules = false;

    // Prevent script reflection warnings from bringing down the AssetProcessor
    appDescriptor.m_enableScriptReflection = false;
    // start listening for exceptions occurring so if something goes wrong we have at least SOME output...
    AZ::Debug::Trace::HandleExceptions(true);

    m_frameworkApp.Start(appDescriptor, params);

    //Registering all the Components
    m_frameworkApp.RegisterComponentDescriptor(AzFramework::LogComponent::CreateDescriptor());

    Reflect();

    const AzFramework::CommandLine* commandLine = nullptr;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetCommandLine);
    if (commandLine && commandLine->HasSwitch("logDir"))
    {
        AZ::IO::FileIOBase::GetInstance()->SetAlias("@log@", commandLine->GetSwitchValue("logDir", 0).c_str());
    }
    else if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
    {
        AZ::IO::Path projectUserPath;
        settingsRegistry->Get(projectUserPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectUserPath);

        AZ::IO::Path logUserPath = projectUserPath / "log";
        auto fileIo = AZ::IO::FileIOBase::GetInstance();
        fileIo->SetAlias("@log@", logUserPath.c_str());
    }
    m_entity = aznew AZ::Entity("Application Entity");
    if (m_entity)
    {
        AssetProcessor::FilteredLogComponent* logger = aznew AssetProcessor::FilteredLogComponent();
        m_entity->AddComponent(logger);
        if (logger)
        {
            // Prevent files overwriting each other if you run batch at same time as GUI (unit tests, for example)
            logger->SetLogFileBaseName(GetLogBaseName());
        }

        //Activate all the components
        m_entity->Init();
        m_entity->Activate();

        return true;
    }
    else
    {
        //aznew failed
        return false;
    }
}

bool ApplicationManager::ActivateModules()
{
     AssetProcessor::StatsCapture::BeginCaptureStat("LoadingModules");

    // we load the editor xml for our modules since it contains the list of gems we need for tools to function (not just runtime)
    connect(&m_frameworkApp, &AssetProcessorAZApplication::AssetProcessorStatus, this,
        [this](AssetProcessor::AssetProcessorStatusEntry entry)
    {
        Q_EMIT AssetProcessorStatusChanged(entry);
        QCoreApplication::processEvents(QEventLoop::AllEvents);
    });

    QDir assetRoot;
    if (!AssetUtilities::ComputeAssetRoot(assetRoot))
    {
        AZ_Error(AssetProcessor::ConsoleChannel, false, "Cannot compute the asset root folder.  Is AssetProcessor being run from the appropriate folder?");
        return false;
    }

    m_frameworkApp.LoadDynamicModules();

    AssetProcessor::StatsCapture::EndCaptureStat("LoadingModules");
    return true;
}

void ApplicationManager::addRunningThread(AssetProcessor::ThreadWorker* thread)
{
    m_runningThreads.push_back(thread);
}


ApplicationManager::BeforeRunStatus ApplicationManager::BeforeRun()
{
    // Create the Qt Application
    CreateQtApplication();

    if (!StartAZFramework())
    {
        return ApplicationManager::BeforeRunStatus::Status_Failure;
    }

    if (!AssetUtilities::ComputeEngineRoot(m_systemRoot))
    {
        return ApplicationManager::BeforeRunStatus::Status_Failure;
    }

    if (!AssetUtilities::UpdateBranchToken())
    {
        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Asset Processor was unable to open  the bootstrap file and verify/update the branch token. \
            Please ensure that the bootstrap.cfg file is present and not locked by any other program.\n");
        return ApplicationManager::BeforeRunStatus::Status_Failure;
    }

    // enable stats capture from this point on
    AssetProcessor::StatsCapture::Initialize();

    return ApplicationManager::BeforeRunStatus::Status_Success;
}

bool ApplicationManager::Activate()
{
    if (!AssetUtilities::ComputeAssetRoot(m_systemRoot))
    {
        AZ_Error(AssetProcessor::ConsoleChannel, false, "Unable to compute the asset root for the project, this application cannot launch until this is fixed.");
        return false;
    }

    auto projectName = AssetUtilities::ComputeProjectName();
    if (projectName.isEmpty())
    {
        AZ_Error(AssetProcessor::ConsoleChannel, false, "Unable to detect name of current game project. Configure your game project name to launch this application.");
        return false;
    }

    // the following controls what registry keys (or on mac or linux what entries in home folder) are used
    // so they should not be translated!
    qApp->setOrganizationName(GetOrganizationName());
    qApp->setOrganizationDomain("amazon.com");
    qApp->setApplicationName(GetApplicationName());

    return true;
}

QString ApplicationManager::GetOrganizationName() const
{
    return "O3DE";
}

QString ApplicationManager::GetApplicationName() const
{
    return "O3DE Asset Processor";
}

bool ApplicationManager::PostActivate()
{
    return true;
}

bool ApplicationManager::NeedRestart() const
{
    return m_needRestart;
}

void ApplicationManager::Restart()
{
    if (m_needRestart)
    {
        AZ_TracePrintf(AssetProcessor::DebugChannel, "Restart() - already restarting\n");
        return;
    }
    AZ_TracePrintf(AssetProcessor::ConsoleChannel, "AssetProcessor is restarting.\n");
    m_needRestart = true;
    m_updateTimer.stop();
    QuitRequested();
}

void ApplicationManager::ReadyToQuit(QObject* source)
{
    if (!source)
    {
        return;
    }

    AZ_TracePrintf(AssetProcessor::ConsoleChannel, "App Quit Object of type %s indicates it is ready.\n", source->metaObject()->className());

    for (int notifyIdx = 0; notifyIdx < m_objectsToNotify.size(); ++notifyIdx)
    {
        if (m_objectsToNotify[notifyIdx].first == source)
        {
            // replace it.
            m_objectsToNotify[notifyIdx] = QuitPair(m_objectsToNotify[notifyIdx].first, true);
        }
    }
    if (!m_queuedCheckQuit)
    {
        QTimer::singleShot(0, this, SLOT(CheckQuit()));
        m_queuedCheckQuit = true;
    }
}

void ApplicationManager::RegisterComponentDescriptor(const AZ::ComponentDescriptor* descriptor)
{
    AZ_Assert(descriptor, "descriptor cannot be null");
    this->m_frameworkApp.RegisterComponentDescriptor(descriptor);
}

ApplicationManager::RegistryCheckInstructions ApplicationManager::CheckForRegistryProblems(QWidget* /*parentWidget*/, [[maybe_unused]] bool showPopupMessage)
{
#if defined(AZ_PLATFORM_WINDOWS)
    // There's a bug that prevents rc.exe from closing properly, making it appear
    // that jobs never complete. The issue is that Windows sometimes decides to put
    // an exe into a special compatibility mode and tells FreeLibrary calls to stop
    // doing anything. Once the registry entry for this is written, it never gets
    // removed unless the user goes and does it manually in RegEdit.
    // To prevent this from being a problem, we check for that registry key
    // and tell the user to remove it.
    // Here's a link with the same problem reported: https://social.msdn.microsoft.com/Forums/vstudio/en-US/3abe477b-ba6f-49d2-894f-efd42165e620/why-windows-generates-an-ignorefreelibrary-entry-in-appcompatflagslayers-registry-?forum=windowscompatibility
    // Here's a link to someone else with the same problem mentioning the problem registry key: https://software.intel.com/en-us/forums/intel-visual-fortran-compiler-for-windows/topic/606006

    QString compatibilityRegistryGroupName = "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers";
    QSettings settings(compatibilityRegistryGroupName, QSettings::NativeFormat);

    auto keys = settings.childKeys();
    for (auto key : keys)
    {
        if (key.contains("rc.exe", Qt::CaseInsensitive))
        {
            // Windows will allow us to see that there is an entry, but won't allow us to
            // read the entry or to modify it, so we'll have to warn the user instead

            // Qt displays the key with the slashes flipped; flip them back because we're on windows
            QString windowsFriendlyRegPath = key.replace('/', '\\');

            QString warningText = QObject::tr(
                "The AssetProcessor will not function correctly with certain registry settings. To correct the problem, please:\n"
                "1) Open RegEdit\n"
                "2) When Windows asks if you'd like to allow the app to make changes to your device, click \"Yes\"\n"
                "3) Open the registry group for the path %0\n"
                "4) Delete the key for %1\n"
                "5) %2"
            ).arg(compatibilityRegistryGroupName, windowsFriendlyRegPath);
            if (showPopupMessage)
            {
                return PopupRegistryProblemsMessage(warningText);
            }
            else
            {
                warningText = warningText.arg(tr("Restart the Asset Processor"));
                QByteArray warningUtf8 = warningText.toUtf8();
                AZ_TracePrintf(AssetProcessor::ConsoleChannel, warningUtf8.data());
            }

            return RegistryCheckInstructions::Exit;
        }
    }
#endif

    return RegistryCheckInstructions::Continue;
}


QDateTime ApplicationDependencyInfo::Timestamp() const
{
    return m_timestamp;
}

void ApplicationDependencyInfo::SetTimestamp(const QDateTime& timestamp)
{
    m_timestamp = timestamp;
}

QString ApplicationDependencyInfo::FileName() const
{
    return m_fileName;
}

void ApplicationDependencyInfo::SetFileName(QString fileName)
{
    m_fileName = fileName;
}


