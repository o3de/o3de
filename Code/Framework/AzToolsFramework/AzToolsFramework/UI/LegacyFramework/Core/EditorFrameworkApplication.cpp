/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include "EditorFrameworkApplication.h"

#include <time.h>

#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/Sfmt.h>
#include <AzCore/std/time.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Script/ScriptSystemComponent.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>

#include <AzFramework/CommandLine/CommandLine.h>

#include <AzToolsFramework/UI/LegacyFramework/UIFramework.hxx>
#include <AzToolsFramework/UI/LegacyFramework/CustomMenus/CustomMenusAPI.h>

#include <AzFramework/Asset/AssetCatalogComponent.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/TargetManagement/TargetManagementComponent.h>

#include <AzCore/Driller/Driller.h>

#ifdef AZ_PLATFORM_WINDOWS
#include "shlobj.h"
#endif

#if AZ_TRAIT_OS_PLATFORM_APPLE
#   include <mach-o/dyld.h>
#endif

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 'QFileInfo::d_ptr': class 'QSharedDataPointer<QFileInfoPrivate>' needs to have dll-interface to be used by clients of class 'QFileInfo'
#include <QFileInfo>
AZ_POP_DISABLE_WARNING
#include <QSharedMemory>
#include <QStandardPaths>
#include <QtWidgets/QApplication>

namespace LegacyFramework
{
    ApplicationDesc::ApplicationDesc(const char* name, int argc, char** argv)
        : m_applicationModule(nullptr)
        , m_enableGridmate(true)
        , m_enablePerforce(true)
        , m_enableGUI(true)
        , m_enableProjectManager(true)
        , m_shouldRunAssetProcessor(true)
        , m_saveUserSettings(true)
        , m_argc(argc)
        , m_argv(argv)
    {
        m_applicationName[0] = 0;
        if (name)
        {
            azstrcpy(m_applicationName, AZ_MAX_PATH_LEN, name);
        }
    }

    ApplicationDesc::ApplicationDesc(const ApplicationDesc& other)
    {
        this->operator=(other);
    }

    ApplicationDesc& ApplicationDesc::operator=(const ApplicationDesc& other)
    {
        if (this == &other)
        {
            return *this;
        }

        m_applicationModule = other.m_applicationModule;
        m_enableGUI = other.m_enableGUI;
        m_enableGridmate = other.m_enableGridmate;
        m_enablePerforce = other.m_enablePerforce;
        azstrcpy(m_applicationName, AZ_MAX_PATH_LEN, other.m_applicationName);
        m_enableProjectManager = other.m_enableProjectManager;
        m_shouldRunAssetProcessor = other.m_shouldRunAssetProcessor;
        m_saveUserSettings = other.m_saveUserSettings;
        m_argc = other.m_argc;
        m_argv = other.m_argv;
        return *this;
    }

    Application::Application()
    {
        m_isPrimary = true;
        m_desiredExitCode = 0;
        m_abortRequested = false;
        m_applicationEntity = nullptr;
        m_ptrSystemEntity = nullptr;
        m_applicationModule[0] = 0;
    }

    void* Application::GetMainModule()
    {
        return m_desc.m_applicationModule;
    }


    const char* Application::GetApplicationName()
    {
        return m_desc.m_applicationName;
    }

    const char* Application::GetApplicationModule()
    {
        return m_applicationModule;
    }


    const char* Application::GetApplicationDirectory()
    {
        return GetExecutableFolder();
    }

#ifdef AZ_PLATFORM_WINDOWS
    BOOL CTRL_BREAK_HandlerRoutine(DWORD /*dwCtrlType*/)
    {
        EBUS_EVENT(FrameworkApplicationMessages::Bus, SetAbortRequested);
        return TRUE;
    }
#endif

    AZStd::string Application::GetApplicationGlobalStoragePath()
    {
        return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).toUtf8().data();
    }

    void Application::SetSettingsRegistrySpecializations(AZ::SettingsRegistryInterface::Specializations& specializations)
    {
        ComponentApplication::SetSettingsRegistrySpecializations(specializations);
        specializations.Append("legacy");
        specializations.Append("tools");
    }

    void Application::CreateReflectionManager()
    {
        AZ::ComponentApplication::CreateReflectionManager();
        GetSerializeContext()->CreateEditContext();
    }

    int Application::Run(const ApplicationDesc& desc)
    {
        if (!AZ::AllocatorInstance<AZ::OSAllocator>::IsReady())
        {
            AZ::AllocatorInstance<AZ::OSAllocator>::Create();
        }

        QString appNameConcat = QStringLiteral("%1_GLOBALMUTEX").arg(desc.m_applicationName);
        {
            // If the application crashed before, it may have left behind shared memory
            // We can go ahead and do a quick attach/detach here to clean it up
            QSharedMemory fix(appNameConcat);
            fix.attach();
        }
        QSharedMemory* shared = new QSharedMemory(appNameConcat);
        if (qApp)
        {
            QObject::connect(qApp, &QCoreApplication::aboutToQuit, qApp, [shared]() { delete shared; });
        }
        m_isPrimary = shared->create(1);

        m_desc = desc;

        // Enable next line to load from the last state
        // if left commented, we will start from scratch
        // THE FOLLOWING LINE CREATES THE MEMORY MANAGER SUBSYSTEM, do not allocate memory before this call!
        m_ptrSystemEntity = Create({});

        AZ::Debug::Trace::HandleExceptions(false);
        FrameworkApplicationMessages::Handler::BusConnect();
        CoreMessageBus::Handler::BusConnect();

#ifdef AZ_PLATFORM_WINDOWS
        // if we're in console mode, listen for CTRL+C
        ::SetConsoleCtrlHandler(CTRL_BREAK_HandlerRoutine, true);
#endif

        m_ptrCommandLineParser = aznew AzFramework::CommandLine();
        m_ptrCommandLineParser->Parse(m_desc.m_argc, m_desc.m_argv);

        // If we don't have one create a serialize context
        if (GetSerializeContext() == nullptr)
        {
            CreateReflectionManager();
        }

        CreateSystemComponents();

        m_ptrSystemEntity->Init();
        m_ptrSystemEntity->Activate();

        // If we aren't the primary, RunAsAnotherInstance unless we are being forcestarted
        if (!m_isPrimary && !m_ptrCommandLineParser->HasSwitch("forcestart"))
        {
            // Required for the application component to handle RunAsAnotherInstance
            CreateApplicationComponent();

            // if we're not the primary instance, what exactly do we do?  This is a generic framework - not a specific app
            // and what we do might depend on implementation specifics for each app.
            EBUS_EVENT(LegacyFramework::CoreMessageBus, RunAsAnotherInstance);
        }
        else
        {
            //if (!RequiresGameProject())
            {
                // we won't be getting the project set message
                CreateApplicationComponent();
            }

            EBUS_EVENT(LegacyFramework::CoreMessageBus, Run);

            // as a precaution here, we save our app and system entities BEFORE we destroy anything
            // so that we have the highest chance of storing the user's precious application state and preferences
            // even if someone has done something bad on their destructor or shutdown
            SaveApplicationEntity();
        }

        if (m_applicationEntity)
        {
            m_applicationEntity->Deactivate();
            delete m_applicationEntity;
            m_applicationEntity = nullptr;
        }

        AZ::SystemTickBus::ExecuteQueuedEvents();
        AZ::TickBus::ExecuteQueuedEvents();

#ifdef AZ_PLATFORM_WINDOWS
        // clean up!
        ::SetConsoleCtrlHandler(CTRL_BREAK_HandlerRoutine, false);
#endif

        delete m_ptrCommandLineParser;
        m_ptrCommandLineParser = nullptr;

        CoreMessageBus::Handler::BusDisconnect();
        FrameworkApplicationMessages::Handler::BusDisconnect();

        m_ptrSystemEntity->Deactivate();

        Destroy();

        return GetDesiredExitCode();
    }

    void Application::TeardownApplicationComponent()
    {
        SaveApplicationEntity();

        if (m_applicationEntity)
        {
            m_applicationEntity->Deactivate();
            delete m_applicationEntity;
            m_applicationEntity = nullptr;
        }
    }

    const AzFramework::CommandLine* Application::GetCommandLineParser()
    {
        return m_ptrCommandLineParser;
    }

    // returns TRUE if the component already existed, FALSE if it had to create one.
    bool Application::EnsureComponentCreated(AZ::Uuid componentCRC)
    {
        if (m_applicationEntity)
        {
            // if the component already exists on the system entity, this is an error.
            if (auto comp = m_ptrSystemEntity->FindComponent(componentCRC))
            {
                AZ_Warning("EditorFramework", 0, "Attempt to add a component that already exists on the system entity: %s\n", comp->RTTI_GetTypeName());
                return true;
            }


            if (!m_applicationEntity->FindComponent(componentCRC))
            {
                if (m_applicationEntity->GetState() != AZ::Entity::State::Constructed)
                {
                    AZ_Warning("EditorFramework", 0, "Attempt to add a component 0x%08x to the application entity when the application entity has already been activated\n", componentCRC);
                    return false;
                }
                m_applicationEntity->CreateComponent(componentCRC);
            }

            return true;
        }

        if (m_ptrSystemEntity->GetState() != AZ::Entity::State::Constructed)
        {
            AZ_Warning("EditorFramework", 0, "Attempt to add a component 0x%08x to the system entity when the system entity has already been activated\n", componentCRC);
            return false;
        }

        if (!m_ptrSystemEntity->FindComponent(componentCRC))
        {
            m_ptrSystemEntity->CreateComponent(componentCRC);
            return false;
        }
        return true;
    }

    // returns TRUE if the component existed, FALSE if the component did not exist.
    bool Application::EnsureComponentRemoved(AZ::Uuid componentCRC)
    {
        if (m_applicationEntity)
        {
            if (auto comp = m_applicationEntity->FindComponent(componentCRC))
            {
                if (m_applicationEntity->GetState() != AZ::Entity::State::Constructed)
                {
                    AZ_Warning("EditorFramework", 0, "Attempt to remove a component %s (0x%08x) from the application entity when the application entity has already been activated\n", comp->RTTI_GetTypeName(), componentCRC);
                    return true;
                }
                m_applicationEntity->RemoveComponent(comp);
                delete comp;
                return true;
            }
            return false;
        }

        if (m_ptrSystemEntity)
        {
            if (auto comp = m_ptrSystemEntity->FindComponent(componentCRC))
            {
                if (m_ptrSystemEntity->GetState() != AZ::Entity::State::Constructed)
                {
                    AZ_Warning("EditorFramework", 0, "Attempt to remove a component %s (0x%08x) from the system entity when the entity entity has already been activated\n", comp->RTTI_GetTypeName(), componentCRC);
                    return true;
                }
                m_ptrSystemEntity->RemoveComponent(comp);
                delete comp;
                return true;
            }
        }

        return false;
    }

    void Application::CreateApplicationComponent()
    {
        // create the application entity:
        if (m_applicationEntity)
        {
            return;
        }
        AZ_TracePrintf("EditorFramework", "Application::OnProjectSet -- Creating Application Entity.\n");
        AZ_Assert(m_applicationEntity == nullptr, "Attempt to set a project while the project is still set.");

        AZStd::string applicationFilePath;
        AzFramework::StringFunc::Path::Join(GetExecutableFolder(), appName(), applicationFilePath);

        applicationFilePath.append("_app.xml");

        AZ_Assert(applicationFilePath.size() <= AZ_MAX_PATH_LEN, "Application path longer than expected");
        qstrcpy(m_applicationFilePath, applicationFilePath.c_str());

        // load all application entities, if present:
        AZ::IO::SystemFile cfg;

        if (cfg.Open(m_applicationFilePath, AZ::IO::SystemFile::SF_OPEN_READ_ONLY))
        {
            AZ::IO::SystemFileStream stream(&cfg, false);
            stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            AZ::ObjectStream::LoadBlocking(&stream, *GetSerializeContext(),
                [this](void* classPtr, const AZ::Uuid& classId, const AZ::SerializeContext* sc)
                {
                    AZ::Entity* entity = sc->Cast<AZ::Entity*>(classPtr, classId);
                    if (entity)
                    {
                        m_applicationEntity = entity;
                    }
                });
            cfg.Close();
        }

        if (!m_applicationEntity)
        {
            m_applicationEntity = aznew AZ::Entity("StandaloneToolsApplicationEntity");
        }

        CreateApplicationComponents();

        m_applicationEntity->InvalidateDependencies();
        m_applicationEntity->Init();
        m_applicationEntity->Activate();

        OnApplicationEntityActivated();
    }

    void Application::OnApplicationEntityActivated()
    {
    }

    bool Application::IsAppConfigWritable()
    {
        return !AZ::IO::SystemFile::Exists(m_applicationFilePath) || AZ::IO::SystemFile::IsWritable(m_applicationFilePath);
    }

    void Application::OnProjectSet(const char* projectPath)
    {
        (void)projectPath;
        CreateApplicationComponent();
    }

    void Application::RunAssetProcessor()
    {
        return;
    }

    void Application::SaveApplicationEntity()
    {
        if (!m_applicationEntity)
        {
            return;
        }

        // write the current applicaiton entity:
        AZStd::string applicationFilePath;
        AzFramework::StringFunc::Path::Join(GetExecutableFolder(), appName(), applicationFilePath);
        applicationFilePath += "_app.xml";

        QFileInfo fileAttribs(applicationFilePath.c_str());
        bool writeIt = true;
        if (fileAttribs.exists())
        {
            // file is found.
            if (fileAttribs.isHidden() || !fileAttribs.isWritable())
            {
                writeIt = false;
            }
        }

        if (writeIt)
        {
            using namespace AZ;
            AZStd::string tmpFileName(applicationFilePath);
            tmpFileName += ".tmp";

            IO::FileIOStream stream(tmpFileName.c_str(), IO::OpenMode::ModeWrite);
            if (!stream.IsOpen())
            {
                return;
            }
            AZ::SerializeContext* serializeContext = GetSerializeContext();
            AZ_Assert(serializeContext, "ComponentApplication::m_serializeContext is NULL!");
            ObjectStream* objStream = ObjectStream::Create(&stream, *serializeContext, ObjectStream::ST_XML);

            bool entityWriteOk = objStream->WriteClass(m_applicationEntity);
            AZ_Warning("ComponentApplication", entityWriteOk, "Failed to write application entity to application file %s!", applicationFilePath.c_str());
            bool flushOk = objStream->Finalize();
            AZ_Warning("ComponentApplication", flushOk, "Failed finalizing application file %s!", applicationFilePath.c_str());
            
            if (entityWriteOk && flushOk)
            {
                if (IO::SystemFile::Rename(tmpFileName.c_str(), applicationFilePath.c_str(), true))
                {
                    return;
                }
                AZ_Warning("ComponentApplication", false, "Failed to rename %s to %s.", tmpFileName.c_str(), applicationFilePath.c_str());
            }
        }
    }

    void Application::CreateApplicationComponents()
    {
        EnsureComponentCreated(AzFramework::TargetManagementComponent::RTTI_Type());
    }

    void Application::CreateSystemComponents()
    {
        EnsureComponentCreated(AZ::MemoryComponent::RTTI_Type());

        AZ_Assert(!m_desc.m_enableProjectManager || m_desc.m_enableGUI, "Enabling the project manager in the application settings requires enabling the GUI as well.");

        EnsureComponentCreated(AzToolsFramework::Framework::RTTI_Type());
    }

    //=========================================================================
    // RegisterCoreComponents
    // [6/18/2012]
    //=========================================================================
    void Application::RegisterCoreComponents()
    {
        ComponentApplication::RegisterCoreComponents();

        RegisterComponentDescriptor(AzFramework::TargetManagementComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::Framework::CreateDescriptor());
    }
}
