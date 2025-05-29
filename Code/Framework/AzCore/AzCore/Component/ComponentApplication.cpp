/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <cctype>

#include <AzCore/AzCoreModule.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Casting/lossy_cast.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/ComponentApplicationLifecycle.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Date/DateFormat.h>

#include <AzCore/Memory/AllocationRecords.h>

#include <AzCore/Memory/AllocatorManager.h>

#include <AzCore/Metrics/EventLoggerFactoryImpl.h>
#include <AzCore/Metrics/JsonTraceEventLogger.h>
#include <AzCore/Metrics/EventLoggerUtils.h>

#include <AzCore/NativeUI/NativeUIRequests.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>

#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>

#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <AzCore/Settings/CommandLineParser.h>
#include <AzCore/Settings/ConfigParser.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Settings/SettingsRegistryScriptUtils.h>
#include <AzCore/Settings/SettingsRegistryVisitorUtils.h>
#include <AzCore/Settings/SettingsRegistryOriginTracker.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <AzCore/Module/Module.h>
#include <AzCore/Module/ModuleManager.h>

#include <AzCore/IO/Path/PathReflect.h>
#include <AzCore/IO/SystemFile.h>

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Script/ScriptSystemBus.h>

#include <AzCore/Math/PolygonPrism.h>
#include <AzCore/Math/Spline.h>
#include <AzCore/Math/VertexContainer.h>

#include <AzCore/Name/NameDictionary.h>

#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/Utils/Utils.h>

#include <AzCore/XML/rapidxml.h>
#include <AzCore/Math/Sfmt.h>

#if defined(AZ_ENABLE_DEBUG_TOOLS)
#include <AzCore/Debug/StackTracer.h>
#endif // defined(AZ_ENABLE_DEBUG_TOOLS)

#include <AzCore/Module/Environment.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/utility/charconv.h>
#include <AzCore/std/ranges/ranges_algorithm.h>
#include <AzCore/Time/TimeSystem.h>

#include <AzCore/Outcome/Outcome.h> // for unexpect_t

namespace AZ::Metrics
{
    const EventLoggerId CoreEventLoggerId{ static_cast<AZ::u32>(AZStd::hash<AZStd::string_view>{}("Core")) };
    constexpr const char* CoreMetricsFilenameStem = "Metrics/core_metrics";
    // Settings key used which indicates the rate in microseconds seconds to record core metrics in the Tick() function
    constexpr AZStd::string_view CoreMetricsRecordRateMicrosecondsKey = "/O3DE/Metrics/Core/RecordRateMicroseconds";
    // Settings key which determines if the core_metrics.*.json file is created
    // By default it is not created in release configuration and is for non-release configurations
    constexpr auto CoreMetricsCreateLoggerKey = AZ::Metrics::SettingsKey("Core/CreateLogger");
}

namespace AZ::Internal
{
    static bool ShouldCreateCoreMetricsLogger(SettingsRegistryInterface& settingsRegistry)
    {
#if !defined(AZ_RELEASE_BUILD)
        bool createCoreMetricsFile{ true };
#else
        bool createCoreMetricsFile{ false };
#endif

        settingsRegistry.Get(createCoreMetricsFile, AZ::Metrics::CoreMetricsCreateLoggerKey);

        return createCoreMetricsFile;
    }

    enum class DevelopmentSettingsOverrides
    {
        None, // 0 = no overrides are allowed
        CommandLineOnly, // 1 = registry overrides are allowed from the command line
        ProjectOnly, // 2 = registry overrides are allowed from the engine, gem and project registry files
        CommandLineAndProject, // 3 = registry overrides are allowed from the command line, engine, gem, and project files
        CommandLineProjectAndUser // 4 = registry overrides are allowed from the command line, engine, gem, project, and user files
    };

    //! Determines which development settings (user registry files, project registry files, etc) should
    //! be allowed to override the default settings.
    //! @return the level of development settings overrides that are allowed
    static constexpr DevelopmentSettingsOverrides GetDevelopmentSettingsOverrides()
    {
#if defined(ALLOW_SETTINGS_REGISTRY_DEVELOPMENT_OVERRIDES)
        // If this compile setting has been set, then allow whatever subset of settings overrides the user requested, whether this
        // is a debug, profile, or release build.
        return static_cast<DevelopmentSettingsOverrides>(ALLOW_SETTINGS_REGISTRY_DEVELOPMENT_OVERRIDES);
#elif AZ_RELEASE_BUILD
        // By default, if no compile setting was provided, allow overrides from the engine, gem and project registry files in release builds.
        return DevelopmentSettingsOverrides::ProjectOnly;
#else
        // By default, if no compile setting was provided, turn on all overrides in non-release builds.
        return DevelopmentSettingsOverrides::CommandLineProjectAndUser;
#endif
    }

    static constexpr bool CanUseStartupCfgFile()
    {
#if defined(O3DE_STARTUP_CFG_FILE_CHECK_OVERRIDE)
        return bool{ O3DE_STARTUP_CFG_FILE_CHECK_OVERRIDE };
#elif AZ_RELEASE_BUILD
        return false;
#else
        return true;
#endif
    }
} // namespace AZ::Internal

namespace AZ
{
    // explicit instantiation of the template defined in ComponentApplicationBus.h
    static void PrintEntityName(const AZ::ConsoleCommandContainer& arguments)
    {
        if (arguments.empty())
        {
            return;
        }

        AZStd::string_view entityIdStr(arguments.front());
        AZ::u64 entityIdValue;
        AZStd::from_chars(entityIdStr.begin(), entityIdStr.end(), entityIdValue);

        AZStd::string entityName;
        if (auto componentApplicationRequests = AZ::Interface<ComponentApplicationRequests>::Get();
            componentApplicationRequests != nullptr)
        {
            entityName = componentApplicationRequests->GetEntityName(AZ::EntityId(entityIdValue));
        }

        AZ_Printf("Entity Debug", "EntityId: %llu, Entity Name: %s", entityIdValue, entityName.c_str());
    }

    AZ_CONSOLEFREEFUNC(PrintEntityName, AZ::ConsoleFunctorFlags::Null,
        "Parameter: EntityId value, Prints the name of the entity to the console");

    static EnvironmentVariable<ReflectionEnvironment> s_reflectionEnvironment;
    static const char* s_reflectionEnvironmentName = "ReflectionEnvironment";

    void ReflectionEnvironment::Init()
    {
        s_reflectionEnvironment = AZ::Environment::CreateVariable<ReflectionEnvironment>(s_reflectionEnvironmentName);
    }

    void ReflectionEnvironment::Reset()
    {
        s_reflectionEnvironment.Reset();
    }

    ReflectionManager* ReflectionEnvironment::GetReflectionManager()
    {
        AZ::EnvironmentVariable<ReflectionEnvironment> environment = AZ::Environment::FindVariable<ReflectionEnvironment>(s_reflectionEnvironmentName);
        return environment ? environment->Get() : nullptr;
    }

    //=========================================================================
    // ComponentApplication::Descriptor
    // [5/30/2012]
    //=========================================================================
    ComponentApplication::Descriptor::Descriptor()
    {
        m_useExistingAllocator = false;
        m_allocationRecordsSaveNames = false;
        m_allocationRecordsAttemptDecodeImmediately = false;
        m_autoIntegrityCheck = false;
        m_markUnallocatedMemory = true;
        m_doNotUsePools = false;
        m_enableScriptReflection = true;

        m_memoryBlocksByteSize = 0;
        m_recordingMode = Debug::AllocationRecords::Mode::RECORD_STACK_IF_NO_FILE_LINE;
    }

    bool AppDescriptorConverter(SerializeContext& serialize, SerializeContext::DataElementNode& node)
    {
        if (node.GetVersion() < 2)
        {
            int nodeIdx = node.FindElement(AZ_CRC_CE("recordsMode"));
            if (nodeIdx != -1)
            {
                auto& subNode = node.GetSubElement(nodeIdx);

                char oldValue = 0;
                subNode.GetData(oldValue);
                subNode.Convert<Debug::AllocationRecords::Mode>(serialize);
                subNode.SetData<Debug::AllocationRecords::Mode>(serialize, aznumeric_caster(oldValue));
                subNode.SetName("recordingMode");
            }

            nodeIdx = node.FindElement(AZ_CRC_CE("stackRecordLevels"));
            if (nodeIdx != -1)
            {
                auto& subNode = node.GetSubElement(nodeIdx);

                unsigned char oldValue = 0;
                subNode.GetData(oldValue);
                subNode.Convert<AZ::u64>(serialize);
                subNode.SetData<AZ::u64>(serialize, aznumeric_caster(oldValue));
            }
        }

        return true;
    };


    //! SettingsRegistry notifier handler which is responsible for loading
    //! the project.json file at the new project path
    //! if an update to '<BootstrapSettingsRootKey>/project_path' key occurs.
    struct ProjectPathChangedEventHandler
    {
        ProjectPathChangedEventHandler(AZ::SettingsRegistryInterface& registry)
            : m_registry{ registry }
        {
        }

        void operator()(const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
        {
            // Update the project settings when the project path is set
            using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
            const auto projectPathKey = FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";

            AZ::IO::FixedMaxPath newProjectPath;
            if (SettingsRegistryMergeUtils::IsPathAncestorDescendantOrEqual(projectPathKey, notifyEventArgs.m_jsonKeyPath)
                && m_registry.Get(newProjectPath.Native(), projectPathKey) && newProjectPath != m_oldProjectPath)
            {
                // Update old Project path before attempting to merge in new Settings Registry values in order to prevent recursive calls
                m_oldProjectPath = newProjectPath;

                // Update all the runtime file paths based on the new "project_path" value.
                AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(m_registry);
            }
        }

    private:
        AZ::IO::FixedMaxPath m_oldProjectPath;
        AZ::SettingsRegistryInterface& m_registry;
    };

    //! SettingsRegistry notifier handler which adds the project name as a specialization tag
    //! to the registry
    //! if an update to '<ProjectSettingsRootKey>/project_name' key occurs.
    struct ProjectNameChangedEventHandler
    {
        ProjectNameChangedEventHandler(AZ::SettingsRegistryInterface& registry)
            : m_registry{ registry }
        {
        }

        void operator()(const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
        {
            // Update the project specialization when the project name is set
            using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
            const auto projectNameKey = FixedValueString(AZ::SettingsRegistryMergeUtils::ProjectSettingsRootKey) + "/project_name";

            FixedValueString newProjectName;
            if (SettingsRegistryMergeUtils::IsPathAncestorDescendantOrEqual(projectNameKey, notifyEventArgs.m_jsonKeyPath)
                && m_registry.Get(newProjectName, projectNameKey) && newProjectName != m_oldProjectName)
            {
                // Add the project_name as a specialization for loading the build system dependency .setreg files
                auto newProjectNameSpecialization = FixedValueString::format("%s/%.*s", AZ::SettingsRegistryMergeUtils::SpecializationsRootKey,
                    aznumeric_cast<int>(newProjectName.size()), newProjectName.data());
                auto oldProjectNameSpecialization = FixedValueString::format("%s/%s", AZ::SettingsRegistryMergeUtils::SpecializationsRootKey,
                    m_oldProjectName.c_str());
                m_registry.Remove(oldProjectNameSpecialization);
                m_oldProjectName = newProjectName;
                m_registry.Set(newProjectNameSpecialization, true);
            }
        }

    private:
        AZ::SettingsRegistryInterface::FixedValueString m_oldProjectName;
        AZ::SettingsRegistryInterface& m_registry;
    };

    //! SettingsRegistry notifier handler which updates relevant registry settings based
    //! on an update to '/Amazon/AzCore/Bootstrap/project_path' key.
    struct UpdateCommandLineEventHandler
    {
        UpdateCommandLineEventHandler(AZ::SettingsRegistryInterface& registry, AZ::CommandLine& commandLine)
            : m_registry{ registry }
            , m_commandLine{ commandLine }
        {
        }

        void operator()(const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
        {
            // Update the ComponentApplication CommandLine instance when the command line settings are merged into the Settings Registry
            if (notifyEventArgs.m_jsonKeyPath == AZ::SettingsRegistryMergeUtils::CommandLineValueChangedKey)
            {
                AZ::SettingsRegistryMergeUtils::GetCommandLineFromRegistry(m_registry, m_commandLine);
            }
        }

    private:
        AZ::SettingsRegistryInterface& m_registry;
        AZ::CommandLine& m_commandLine;
    };

    //=========================================================================
    // Reflect
    //=========================================================================
    void ComponentApplication::Descriptor::Reflect(ReflectContext* context, ComponentApplication* app)
    {
        // Create a local ObjectFactory that passes the component application descriptor
        // to the SerializeContext
        struct DescriptorFactory
            : public SerializeContext::IObjectFactory
        {
            DescriptorFactory(Descriptor* descriptor)
                : m_descriptor(descriptor)
            {}

            void* Create(const char*) override
            {
                // Return a pointer to the component application descriptor
                return m_descriptor;
            }
            void  Destroy(void*) override
            {
                // do nothing as descriptor is part of the component application
            }

        private:
            Descriptor* m_descriptor;
        };

        static DescriptorFactory descriptorFactory(&app->GetDescriptor());

        DynamicModuleDescriptor::Reflect(context);

        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<Descriptor>(&descriptorFactory)
                ->Version(2, AppDescriptorConverter)
                ->Field("useExistingAllocator", &Descriptor::m_useExistingAllocator)
                ->Field("allocationRecordsSaveNames", &Descriptor::m_allocationRecordsSaveNames)
                ->Field("allocationRecordsAttemptDecodeImmediately", &Descriptor::m_allocationRecordsAttemptDecodeImmediately)
                ->Field("recordingMode", &Descriptor::m_recordingMode)
                ->Field("autoIntegrityCheck", &Descriptor::m_autoIntegrityCheck)
                ->Field("markUnallocatedMemory", &Descriptor::m_markUnallocatedMemory)
                ->Field("doNotUsePools", &Descriptor::m_doNotUsePools)
                ->Field("enableScriptReflection", &Descriptor::m_enableScriptReflection)
                ->Field("blockSize", &Descriptor::m_memoryBlocksByteSize)
                ->Field("modules", &Descriptor::m_modules)
                ;

            if (EditContext* ec = serializeContext->GetEditContext())
            {
                ec->Enum<Debug::AllocationRecords::Mode>("Debug::AllocationRecords::Mode", "Allocator recording mode")
                    ->Value("No records", Debug::AllocationRecords::Mode::RECORD_NO_RECORDS)
                    ->Value("No stack trace", Debug::AllocationRecords::Mode::RECORD_STACK_NEVER)
                    ->Value("Stack trace when file/line missing", Debug::AllocationRecords::Mode::RECORD_STACK_IF_NO_FILE_LINE)
                    ->Value("Stack trace always", Debug::AllocationRecords::Mode::RECORD_FULL);
                ec->Class<Descriptor>("System memory settings", "Settings for managing application memory usage")
                    ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                    ->DataElement(Edit::UIHandlers::CheckBox, &Descriptor::m_allocationRecordsSaveNames, "Record allocations with name saving", "Saves names/filenames information on each allocation made, useful for tracking down leaks in dynamic modules (ignored in Release builds)")
                    ->DataElement(Edit::UIHandlers::CheckBox, &Descriptor::m_allocationRecordsAttemptDecodeImmediately, "Record allocations and attempt immediate decode", "Decode callstacks for each allocation when they occur, used for tracking allocations that fail decoding. Very expensive. (ignored in Release builds)")
                    ->DataElement(Edit::UIHandlers::ComboBox, &Descriptor::m_recordingMode, "Stack recording mode", "Stack record mode. (Ignored in final builds)")
                    ->DataElement(Edit::UIHandlers::CheckBox, &Descriptor::m_autoIntegrityCheck, "Validate allocations", "Check allocations for integrity on each allocation/free (ignored in Release builds)")
                    ->DataElement(Edit::UIHandlers::CheckBox, &Descriptor::m_markUnallocatedMemory, "Mark freed memory", "Set memory to 0xcd when a block is freed for debugging (ignored in Release builds)")
                    ->DataElement(Edit::UIHandlers::CheckBox, &Descriptor::m_doNotUsePools, "Don't pool allocations", "Pipe pool allocations in system/tree heap (ignored in Release builds)")
                    ->DataElement(Edit::UIHandlers::SpinBox, &Descriptor::m_memoryBlocksByteSize, "Block size", "Memory block size in bytes (must be multiple of the page size)")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<ComponentApplicationBus>("ComponentApplicationBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Components")

                ->Event("GetEntityName", &ComponentApplicationBus::Events::GetEntityName)
                ->Event("SetEntityName", &ComponentApplicationBus::Events::SetEntityName);

            behaviorContext->Class<AZStd::unexpect_t>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "std")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All);
        }
    }

    //=========================================================================
    // ComponentApplication
    // [5/30/2012]
    //=========================================================================
    ComponentApplication::ComponentApplication()
        : ComponentApplication(0, nullptr, {})
    {
    }

    ComponentApplication::ComponentApplication(ComponentApplicationSettings componentAppSettings)
        : ComponentApplication(0, nullptr, AZStd::move(componentAppSettings))
    {
    }
    ComponentApplication::ComponentApplication(int argC, char** argV)
        : ComponentApplication(argC, argV, {})
    {
    }

    ComponentApplication::ComponentApplication(int argC, char** argV, ComponentApplicationSettings componentAppSettings)
        : m_timeSystem(AZStd::make_unique<TimeSystem>())
    {
        if (Interface<ComponentApplicationRequests>::Get() == nullptr)
        {
            Interface<ComponentApplicationRequests>::Register(this);
        }

        if (argV)
        {
            m_argC = argC;
            m_argV = argV;
        }
        else
        {
            azstrcpy(m_commandLineBuffer, AZ_ARRAY_SIZE(m_commandLineBuffer), "no_argv_supplied");
            // use a "valid" value here.  This is because Qt and potentially other third party libraries require
            // that ArgC be 'at least 1' and that (*argV)[0] be a valid pointer to a real null terminated string.
            m_argC = 1;
            m_argV = &m_commandLineBufferAddress;
        }

        // Check for a bootstrap config file that contains settings for configuring the allocator tracking settings
        InitializeAllocatorSettings(m_argC, m_argV);

        // we are about to create allocators, so make sure that
        // the descriptor is filled with at least the defaults:
        m_descriptor.m_recordingMode = AllocatorManager::Instance().GetDefaultTrackingMode();

        // Initializes the OSAllocator and SystemAllocator as soon as possible
        AZ::Debug::Trace::Instance().Init();
        ConfigureSystemAllocatorTracking();

        // Now that the Allocators are initialized, the Command Line parameters can be parsed
        m_commandLine.Parse(m_argC, m_argV);

        m_nameDictionary = AZStd::make_unique<NameDictionary>();

        // Register the Name Dictionary with the AZ Interface system
        if (AZ::Interface<AZ::NameDictionary>::Get() == nullptr)
        {
            AZ::Interface<AZ::NameDictionary>::Register(m_nameDictionary.get());
            // Link the deferred names into this Name Dictionary
            m_nameDictionary->LoadDeferredNames(AZ::Name::GetDeferredHead());
        }

        InitializeSettingsRegistry(componentAppSettings);

        InitializeEventLoggerFactory();

        InitializeLifecyleEvents(*m_settingsRegistry);

        // Create the Module Manager
        m_moduleManager = AZStd::make_unique<ModuleManager>();

        InitializeConsole(*m_settingsRegistry);
    }

    //=========================================================================
    // ~ComponentApplication
    // [5/30/2012]
    //=========================================================================
    ComponentApplication::~ComponentApplication()
    {
        if (Interface<ComponentApplicationRequests>::Get() == this)
        {
            Interface<ComponentApplicationRequests>::Unregister(this);
        }

        if (m_isStarted)
        {
            Destroy();
        }

        // The SettingsRegistry Notify handlers stores an AZStd::function internally
        // which may allocates using the AZ SystemAllocator(if the functor > 16 bytes)
        // The handlers are being default value initialized to clear out the AZStd::function
        m_commandLineUpdatedHandler = {};
        m_projectNameChangedHandler = {};
        m_projectPathChangedHandler = {};

        // Delete the AZ::IConsole if it was created by this application instance
        if (AZ::Interface<AZ::IConsole>::Get() == m_console.get())
        {
            AZ::Interface<AZ::IConsole>::Unregister(m_console.get());
            ComponentApplicationLifecycle::SignalEvent(*m_settingsRegistry, "ConsoleUnavailable", R"({})");
        }
        m_console.reset();

        m_moduleManager.reset();

        // Unregister the global settings registry origin tracker if this application owns is
        if (AZ::Interface<AZ::SettingsRegistryOriginTracker>::Get() == m_settingsRegistryOriginTracker.get())
        {
            AZ::Interface<AZ::SettingsRegistryOriginTracker>::Unregister(m_settingsRegistryOriginTracker.get());
        }
        m_settingsRegistryOriginTracker.reset();
        // Unregister the global Settings Registry if it is owned by this application instance
        if (AZ::SettingsRegistry::Get() == m_settingsRegistry.get())
        {
            SettingsRegistry::Unregister(m_settingsRegistry.get());
            ComponentApplicationLifecycle::SignalEvent(*m_settingsRegistry, "SettingsRegistryUnavailable", R"({})");
            ComponentApplicationLifecycle::SignalEvent(*m_settingsRegistry, "SystemAllocatorPendingDestruction", R"({})");
        }
        m_settingsRegistry.reset();

        // Unregister the Name Dictionary with the AZ Interface system and reset it
        if (AZ::Interface<AZ::NameDictionary>::Get() == m_nameDictionary.get())
        {
            AZ::Interface<AZ::NameDictionary>::Unregister(m_nameDictionary.get());
        }
        m_nameDictionary.reset();

        // Unregister the Event Factory with th AZ Interface if it is registered
        if (AZ::Metrics::EventLoggerFactory::Get() == m_eventLoggerFactory.get())
        {
            AZ::Metrics::EventLoggerFactory::Unregister(m_eventLoggerFactory.get());
        }

        m_eventLoggerFactory.reset();

        // Set AZ::CommandLine to an empty object to clear out allocated memory before the allocators
        // are destroyed
        m_commandLine = {};

        m_entityAddedEvent.DisconnectAllHandlers();
        m_entityRemovedEvent.DisconnectAllHandlers();
        m_entityActivatedEvent.DisconnectAllHandlers();
        m_entityDeactivatedEvent.DisconnectAllHandlers();

        AZ::Debug::Trace::Instance().Destroy();
    }

    void ComponentApplication::InitializeSettingsRegistry(const ComponentApplicationSettings& componentAppSettings)
    {
        SettingsRegistryMergeUtils::ParseCommandLine(m_commandLine);

        // Create the settings registry and register it with the AZ interface system
        // This is done after the AppRoot has been calculated so that the Bootstrap.cfg
        // can be read to determine the Game folder and the asset platform
        m_settingsRegistry = AZStd::make_unique<SettingsRegistryImpl>();
        // Merge the bootstrap settings to the root of the Settings Registry
        m_settingsRegistry->MergeSettings(componentAppSettings.m_setregBootstrapJson, componentAppSettings.m_setregFormat);

        // Register the Settings Registry with the AZ Interface if there isn't one registered already
        if (SettingsRegistry::Get() == nullptr)
        {
            SettingsRegistry::Register(m_settingsRegistry.get());
        }

        m_settingsRegistryOriginTracker = AZStd::make_unique<SettingsRegistryOriginTracker>(*m_settingsRegistry);

        // Register the Settings Registry Origin Tracker with the AZ Interface system
        if (AZ::Interface<AZ::SettingsRegistryOriginTracker>::Get() == nullptr)
        {
            AZ::Interface<AZ::SettingsRegistryOriginTracker>::Register(m_settingsRegistryOriginTracker.get());
        }

        // Add the Command Line arguments into the SettingsRegistry
        SettingsRegistryMergeUtils::StoreCommandLineToRegistry(*m_settingsRegistry, m_commandLine);

        // Add a notifier to update the project_settings when
        // 1. The 'project_path' key changes
        // 2. The project specialization when the 'project-name' key changes
        // 3. The ComponentApplication command line when the command line is stored to the registry
        m_projectPathChangedHandler = m_settingsRegistry->RegisterNotifier(ProjectPathChangedEventHandler{ *m_settingsRegistry });
        m_projectNameChangedHandler = m_settingsRegistry->RegisterNotifier(ProjectNameChangedEventHandler{ *m_settingsRegistry });
        m_commandLineUpdatedHandler =
            m_settingsRegistry->RegisterNotifier(UpdateCommandLineEventHandler{ *m_settingsRegistry, m_commandLine });

        if constexpr (
            AZ::Internal::GetDevelopmentSettingsOverrides() == AZ::Internal::DevelopmentSettingsOverrides::CommandLineProjectAndUser)
        {
            // Only merge the Global User Registry (~/.o3de/Registry) if that override type is allowed by our compile settings.
            SettingsRegistryMergeUtils::MergeSettingsToRegistry_O3deUserRegistry(
                *m_settingsRegistry, AZ_TRAIT_OS_PLATFORM_CODENAME, {});
        }
        // Merge Command Line arguments using the default CommandToParse instance
        SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(*m_settingsRegistry, m_commandLine, {});
        SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*m_settingsRegistry);
    }

    void ComponentApplication::InitializeEventLoggerFactory()
    {
        // Create the EventLoggerFactory as soon as the Allocators are available
        m_eventLoggerFactory = AZStd::make_unique<AZ::Metrics::EventLoggerFactoryImpl>();
        if (AZ::Metrics::EventLoggerFactory::Get() == nullptr)
        {
            AZ::Metrics::EventLoggerFactory::Register(m_eventLoggerFactory.get());
        }
    }

    void ComponentApplication::InitializeLifecyleEvents(AZ::SettingsRegistryInterface& settingsRegistry)
    {
        // The /O3DE/Application/LifecycleEvents array contains a valid set of lifecycle events
        // Those lifecycle events are normally read from the <engine-root>/Registry
        // which isn't merged until ComponentApplication::Create invokes MergeSettingsToRegistry
        // So pre-populate the valid lifecycle even entries
        ComponentApplicationLifecycle::RegisterEvent(settingsRegistry, "SystemAllocatorCreated");
        ComponentApplicationLifecycle::RegisterEvent(settingsRegistry, "SettingsRegistryAvailable");
        ComponentApplicationLifecycle::RegisterEvent(settingsRegistry, "ConsoleAvailable");
        ComponentApplicationLifecycle::SignalEvent(settingsRegistry, "SystemAllocatorCreated", R"({})");
        ComponentApplicationLifecycle::SignalEvent(settingsRegistry, "SettingsRegistryAvailable", R"({})");
    }

    void ComponentApplication::InitializeConsole(SettingsRegistryInterface& settingsRegistry)
    {
        // Az Console initialization.
        // note that tests destroy and construct the application over and over, which is not a desirable pattern
        m_console = AZStd::make_unique<AZ::Console>(settingsRegistry);
        if (AZ::Interface<AZ::IConsole>::Get() == nullptr)
        {
            AZ::Interface<AZ::IConsole>::Register(m_console.get());
            m_console->LinkDeferredFunctors(AZ::ConsoleFunctorBase::GetDeferredHead());
            m_settingsRegistryConsoleFunctors = AZ::SettingsRegistryConsoleUtils::RegisterAzConsoleCommands(settingsRegistry, *m_console);
            m_settingsRegistryOriginTrackerConsoleFunctors = AZ::SettingsRegistryConsoleUtils::RegisterAzConsoleCommands(*m_settingsRegistryOriginTracker, *m_console);
            ComponentApplicationLifecycle::SignalEvent(settingsRegistry, "ConsoleAvailable", R"({})");
        }
    }

    void ComponentApplication::InitializeAllocatorSettings(int argc, char** argv)
    {
        if constexpr (Internal::CanUseStartupCfgFile())
        {
            AZ::IO::FixedMaxPath startupCfgPath;

            // First check if there is an "O3DE_STARTUP_CFG_FILE" environment variable set
            // in the OS and use that as the startup cfg path
            auto QueryStartupEnv = [](char* buffer, size_t size) -> size_t
            {
                constexpr const char* StartupCfgEnvKey = "O3DE_STARTUP_CFG_FILE";
                auto getEnvOutcome = AZ::Utils::GetEnv(AZStd::span(buffer, size), StartupCfgEnvKey);
                return getEnvOutcome ? getEnvOutcome.GetValue().size() : 0;
            };
            startupCfgPath.Native().resize_and_overwrite(startupCfgPath.Native().max_size(), QueryStartupEnv);

            if (startupCfgPath.empty())
            {
                // Default to looking for startup config file at "<exe-directory>/startup.cfg" if the O3DE_STARTUP_CFG_FILE env is not set
                startupCfgPath =
                    AZ::IO::FixedMaxPath(AZ::Utils::GetExecutableDirectory()) / SettingsRegistryInterface::RegistryFolder / "startup.cfg";
                if (!AZ::IO::SystemFile::Exists(startupCfgPath.c_str()))
                {
                    // If a <exe-directory>/startup.cfg doesn't exist, try to locate the startup config file at "~/.o3de/Registry/startup.cfg"
                    startupCfgPath =
                        AZ::IO::FixedMaxPath(AZ::Utils::GetO3deManifestDirectory()) / SettingsRegistryInterface::RegistryFolder / "startup.cfg";
                }
            }

            Settings::CommandLineParserSettings commandLineParserSettings;
            commandLineParserSettings.m_parseCommandLineEntryFunc = [&startupCfgPath](const Settings::CommandLineArgument& argument) -> bool
            {
                constexpr AZStd::string_view StartupFileOption = "startup-cfg-file";
                if (argument.m_option == StartupFileOption)
                {
                    // Remove any double quotes surrounding the startup cfg path
                    // and store it in the path variable
                    startupCfgPath = Settings::UnquoteArgument(argument.m_value);
                }

                return true;
            };

            // Parses the command line arguments without using any heap allocations
            // This attempts to read the startup config file from the command line
            Settings::ParseCommandLine(argc, argv, commandLineParserSettings);

            // Try to the open up the Startup config file
            constexpr auto openMode = AZ::IO::OpenMode::ModeRead;
            AZ::IO::SystemFileStream startupCfgStream(startupCfgPath.c_str(), openMode);
            if (!startupCfgStream.IsOpen())
            {
                return;
            }

            auto& allocatorManager = AZ::AllocatorManager::Instance();
            // Now parse the config file settings without using heap allocations as well
            Settings::ConfigParserSettings configParserSettings;
            configParserSettings.m_parseConfigEntryFunc =
                [&allocatorManager](const Settings::ConfigParserSettings::ConfigEntry& configEntry) -> bool
            {
                // If a key in the config file is of the form with "allocator_tracking_<allocator_name>,
                // try to convert the value into a boolean. If the value is true
                // An entry of the allocator name is registered with the AllocatorManager
                // which is used to turn on Allocator Tracking when an allocator with the specified
                // name is registered
                constexpr AZStd::string_view AllocatorTrackingPrefix = "allocator_tracking_";
                if (AZ::StringFunc::StartsWith(configEntry.m_keyValuePair.m_key, AllocatorTrackingPrefix))
                {
                    AZStd::string_view allocatorName = configEntry.m_keyValuePair.m_key.substr(AllocatorTrackingPrefix.size());
                    if (!allocatorName.empty())
                    {
                        // Attempt to convert the string directly to an AllocationRecords::Mode enum
                        // This occurs in the case when the string matches the name of an enum option
                        AZ::Debug::AllocationRecords::Mode mode = allocatorManager.GetDefaultTrackingMode();
                        if (!AZ::ConsoleTypeHelpers::ToValue(mode, configEntry.m_keyValuePair.m_value))
                        {
                            // The tracking value was unable to be converted to an enum, so return true
                            // to parse the next command line entry
                            return true;
                        }

                        allocatorManager.SetTrackingForAllocator(allocatorName, mode);
                    }
                }

                return true;
            };

            // Now uses the ConfigParser to parse the allocator settings
            AZ::Settings::ParseConfigFile(startupCfgStream, configParserSettings);
        }
    }

    void ComponentApplication::RegisterCoreEventLogger()
    {
        // Check the Core metrics "CreateLogger" setting to determine if it should be created at all
        if (!Internal::ShouldCreateCoreMetricsLogger(*m_settingsRegistry))
        {
            return;
        }

        // Register Core Event logger with Component Application
        // The registration occurs on the first tick
        auto RegisterOnFirstTick = [this]()
        {
            // Use the name of the running build target as part of the event logger name
            // If it is not available, then an event logger will not be created
            AZ::IO::FixedMaxPath uniqueFilenameSuffix = AZ::Metrics::CoreMetricsFilenameStem;
            if (AZ::IO::FixedMaxPathString buildTargetName;
                m_settingsRegistry->Get(buildTargetName, AZ::SettingsRegistryMergeUtils::BuildTargetNameKey))
            {
                // append the build target name as injected from CMake if known
                uniqueFilenameSuffix.Native() += AZ::IO::FixedMaxPathString::format(".%s", buildTargetName.c_str());
            }
            else
            {
                return;
            }

            // Append the build configuration(debug, release, profile) to the metrics filename
            if (AZStd::string_view buildConfig{ AZ_BUILD_CONFIGURATION_TYPE }; !buildConfig.empty())
            {
                uniqueFilenameSuffix.Native() += AZ::IO::FixedMaxPathString::format(".%.*s", AZ_STRING_ARG(buildConfig));
            }

            // Use a ISO8601 timestamp + the process ID to provide uniqueness to the metrics json files
            AZ::Date::Iso8601TimestampString utcTimestampString;
            if (AZ::Date::GetFilenameCompatibleFormatNow(utcTimestampString))
            {
                uniqueFilenameSuffix.Native() += AZ::IO::FixedMaxPathString::format(".%s", utcTimestampString.c_str());
            }
            {
                // append process id
                AZStd::fixed_string<32> processIdString;
                AZStd::to_string(processIdString, AZ::Platform::GetCurrentProcessId());
                uniqueFilenameSuffix.Native() += AZ::IO::FixedMaxPathString::format(".%s", processIdString.c_str());
            }
            // Append .json extension
            uniqueFilenameSuffix.Native() += ".json";

            // Append the relative file name portion to the <project-root>/user directory
            auto metricsFilePath = (AZ::IO::FixedMaxPath{ AZ::Utils::GetProjectUserPath(m_settingsRegistry.get()) }
            / uniqueFilenameSuffix).LexicallyNormal();

            // Open up the metrics file in write mode and truncate the contents if it exist(it shouldn't since a millisecond
            // is being used
            constexpr AZ::IO::OpenMode openMode = AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath;
            if (auto fileStream = AZStd::make_unique<AZ::IO::SystemFileStream>(metricsFilePath.c_str(), openMode);
                fileStream != nullptr && fileStream->IsOpen())
            {
                // Configure core event logger with the name of "Core"
                AZ::Metrics::JsonTraceEventLoggerConfig config{ "Core" };
                auto coreEventLogger = AZStd::make_unique<AZ::Metrics::JsonTraceEventLogger>(AZStd::move(fileStream), config);
                m_eventLoggerFactory->RegisterEventLogger(AZ::Metrics::CoreEventLoggerId, AZStd::move(coreEventLogger));
            }
            else
            {
                AZ_Error("ComponentApplication", false, R"(unable to open core metrics with with path "%s")",
                    metricsFilePath.c_str());
            }

            // Record metrics every X microseconds based on the /O3DE/Metrics/Core/RecordRateMicroseconds setting
            // or every 10 secondsif not supplied
            m_recordMetricsOnTickCallback = [&registry = *m_settingsRegistry.get(),
                lastRecordTime = AZStd::chrono::steady_clock::now()]
                (AZStd::chrono::steady_clock::time_point monotonicTime) mutable -> bool
            {
                // Retrieve the record rate setting from the Setting Registry
                using namespace AZStd::chrono_literals;
                AZStd::chrono::microseconds recordTickMicroseconds = 10s;
                if (AZ::s64 recordRateMicrosecondValue;
                    registry.Get(recordRateMicrosecondValue, AZ::Metrics::CoreMetricsRecordRateMicrosecondsKey))
                {
                    recordTickMicroseconds = AZStd::chrono::microseconds(recordRateMicrosecondValue);
                }

                if ((monotonicTime - lastRecordTime) >= recordTickMicroseconds)
                {
                    // Reset the recordTime to the current steady clock time and return true to record the metrics
                    lastRecordTime = monotonicTime;
                    return true;
                }

                return false;
            };
        };

        AZ::TickBus::QueueFunction(AZStd::move(RegisterOnFirstTick));
    }

    void ComponentApplication::ReportBadEngineRoot()
    {
        AZStd::string errorMessage = {"Unable to determine a valid path to the engine.\n"
                                      "Check parameters such as --project-path and --engine-path and make sure they are valid.\n"};
        if (auto registry = AZ::SettingsRegistry::Get(); registry != nullptr)
        {
            AZ::SettingsRegistryInterface::FixedValueString filePathErrorStr;
            if (registry->Get(filePathErrorStr, AZ::SettingsRegistryMergeUtils::FilePathKey_ErrorText); !filePathErrorStr.empty())
            {
                errorMessage += "Additional Info:\n";
                errorMessage += filePathErrorStr.c_str();
            }
        }

        if (auto nativeUI = AZ::Interface<AZ::NativeUI::NativeUIRequests>::Get(); nativeUI != nullptr)
        {
            nativeUI->DisplayOkDialog("O3DE Fatal Error", errorMessage.c_str(), false);
        }
        else
        {
            AZ_Error("ComponentApplication", false, "O3DE Fatal Error: %s\n", errorMessage.c_str());
        }
    }


    Entity* ComponentApplication::Create(const Descriptor& descriptor, const StartupParameters& startupParameters)
    {
        AZ_Assert(!m_isStarted, "Component application already started!");

        using Type = AZ::SettingsRegistryInterface::Type;
        if (m_startupParameters.m_loadSettingsRegistry &&
            m_settingsRegistry->GetType(SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder) == Type::NoType)
        {
            ReportBadEngineRoot();
            return nullptr;
        }

        m_startupParameters = startupParameters;

        m_descriptor = descriptor;

        ConfigureSystemAllocatorTracking();

#if !defined(_RELEASE)
        m_budgetTracker.Init();
#endif

        // This can be moved to the ComponentApplication constructor if need be
        // This is reading the *.setreg files using SystemFile and merging the settings
        // to the settings registry.

        MergeSettingsToRegistry(*m_settingsRegistry);

        m_systemEntity = AZStd::make_unique<AZ::Entity>(SystemEntityId, "SystemEntity");
        CreateCommon();
        AZ_Assert(m_systemEntity, "SystemEntity failed to initialize!");

        AddRequiredSystemComponents(m_systemEntity.get());
        m_isStarted = true;
        return m_systemEntity.get();
    }

    //=========================================================================
    // CreateCommon
    //=========================================================================
    void ComponentApplication::CreateCommon()
    {
        Sfmt::Create();

        CreateReflectionManager();

        if (m_startupParameters.m_createEditContext)
        {
            GetSerializeContext()->CreateEditContext();
        }

        // Call this and child class's reflects
        ReflectionEnvironment::GetReflectionManager()->Reflect(azrtti_typeid(this), [this](ReflectContext* context) {Reflect(context); });

        RegisterCoreComponents();
        ComponentApplicationLifecycle::SignalEvent(*m_settingsRegistry, "ReflectionManagerAvailable", R"({})");

        TickBus::AllowFunctionQueuing(true);
        SystemTickBus::AllowFunctionQueuing(true);

        // Register the Core metrics Event logger with the IEventLoggerFactory
        // after the TickBus has enabled function queuing
        RegisterCoreEventLogger();

        ComponentApplicationBus::Handler::BusConnect();

        TickRequestBus::Handler::BusConnect();

#if defined(AZ_ENABLE_DEBUG_TOOLS)
        // Prior to loading more modules, we make sure SymbolStorage
        // is listening for the loads so it can keep track of which
        // modules we may eventually need symbols for
        Debug::SymbolStorage::RegisterModuleListeners();
#endif // defined(AZ_ENABLE_DEBUG_TOOLS)

        PreModuleLoad();

        // Load the actual modules
        LoadModules();
        ComponentApplicationLifecycle::SignalEvent(*m_settingsRegistry, "GemsLoaded", R"({})");

        // Execute user.cfg after modules have been loaded but before processing any command-line overrides
        if (m_startupParameters.m_loadSettingsRegistry)
        {
            AZ::IO::FixedMaxPath platformCachePath;
            m_settingsRegistry->Get(platformCachePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder);
            m_console->ExecuteConfigFile((platformCachePath / "user.cfg").Native());
        }

        // Parse the command line parameters for console commands after modules have loaded
        m_console->ExecuteCommandLine(m_commandLine);
    }

    //=========================================================================
    // Destroy
    // [5/30/2012]
    //=========================================================================
    void ComponentApplication::Destroy()
    {
        // Finish all queued work
        AZ::SystemTickBus::Broadcast(&AZ::SystemTickBus::Events::OnSystemTick);

        TickBus::ExecuteQueuedEvents();
        TickBus::AllowFunctionQueuing(false);

        SystemTickBus::ExecuteQueuedEvents();
        SystemTickBus::AllowFunctionQueuing(false);

        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::Finalize);

        // deactivate all entities
        while (!m_entities.empty())
        {
            Entity* entity = m_entities.begin()->second;
            m_entities.erase(m_entities.begin());

            if (entity->GetId() == SystemEntityId)
            {
                AZ_Assert(m_systemEntity.get() == entity, "Activated system entity does not match the system entity created in Create().");
            }
            else
            {
                delete entity;
            }
        }

        // Force full garbage collect after all game entities destroyed, but before modules are unloaded.
        // This is to ensure that all references to reflected classes/ebuses are cleaned up before the data is deleted.
        // This problem could also be solved by using ref-counting for reflected data.
        ScriptSystemRequestBus::Broadcast(&ScriptSystemRequests::GarbageCollect);

        // Deactivate all module entities before the System Entity is deactivated, but do not unload the modules as
        // components on the SystemEntity are allowed to reference module data at this point.
        m_moduleManager->DeactivateEntities();

        // deactivate all system components
        if (m_systemEntity)
        {
            if (m_systemEntity->GetState() == Entity::State::Active)
            {
                m_systemEntity->Deactivate();
            }
        }

        m_entities.clear();
        m_entities.rehash(0); // force free all memory

        DestroyReflectionManager();
        ComponentApplicationLifecycle::SignalEvent(*m_settingsRegistry, "ReflectionManagerUnavailable", R"({})");

        static_cast<SettingsRegistryImpl*>(m_settingsRegistry.get())->ClearNotifiers();
        static_cast<SettingsRegistryImpl*>(m_settingsRegistry.get())->ClearMergeEvents();

#if !defined(_RELEASE)
        // the budget tracker must be cleaned up prior to module unloading to ensure
        // budgets initialized cross boundary are freed properly
        m_budgetTracker.Reset();
#endif

        // Uninit and unload any dynamic modules.
        m_moduleManager->UnloadModules();
        ComponentApplicationLifecycle::SignalEvent(*m_settingsRegistry, "GemsUnloaded", R"({})");

        m_systemEntity.reset();

        Sfmt::Destroy();

        // delete all descriptors left for application clean up
        ComponentDescriptorBus::Broadcast(&ComponentDescriptorBus::Events::ReleaseDescriptor);

        // Disconnect from application and tick request buses
        ComponentApplicationBus::Handler::BusDisconnect();
        TickRequestBus::Handler::BusDisconnect();

        // Clear the descriptor to deallocate all strings (owned by ModuleDescriptor)
        m_descriptor = Descriptor();

        m_isStarted = false;

#if defined(AZ_ENABLE_DEBUG_TOOLS)
        // Unregister module listeners after allocators are destroyed
        // so that symbol/stack trace information is available at shutdown
        Debug::SymbolStorage::UnregisterModuleListeners();
#endif // defined(AZ_ENABLE_DEBUG_TOOLS)
    }

    //=========================================================================
    // ConfigureSystemAllocatorTracking
    // [5/30/2012]
    //=========================================================================
    void ComponentApplication::ConfigureSystemAllocatorTracking()
    {
        AZ::Debug::AllocationRecords* records = AllocatorInstance<SystemAllocator>::Get().GetRecords();
        if (records)
        {
            records->SetMode(m_descriptor.m_recordingMode);
            records->SetSaveNames(m_descriptor.m_allocationRecordsSaveNames);
            records->SetDecodeImmediately(m_descriptor.m_allocationRecordsAttemptDecodeImmediately);
            records->AutoIntegrityCheck(m_descriptor.m_autoIntegrityCheck);
            records->MarkUallocatedMemory(m_descriptor.m_markUnallocatedMemory);
        }
    }

    void ComponentApplication::MergeSharedSettings(
        SettingsRegistryInterface& registry,
        const AZ::SettingsRegistryInterface::Specializations& specializations,
        AZStd::vector<char>& scratchBuffer)
    {
        constexpr bool overridesAllowedFromCommandLine =
            AZ::Internal::GetDevelopmentSettingsOverrides() == AZ::Internal::DevelopmentSettingsOverrides::CommandLineOnly ||
            AZ::Internal::GetDevelopmentSettingsOverrides() == AZ::Internal::DevelopmentSettingsOverrides::CommandLineAndProject ||
            AZ::Internal::GetDevelopmentSettingsOverrides() == AZ::Internal::DevelopmentSettingsOverrides::CommandLineProjectAndUser;
        if constexpr (overridesAllowedFromCommandLine)
        {
            if constexpr (
                AZ::Internal::GetDevelopmentSettingsOverrides() == AZ::Internal::DevelopmentSettingsOverrides::CommandLineProjectAndUser)
            {
                // In development builds apply the o3de registry and the command line to allow early overrides. This will
                // allow developers to override things like default paths or Asset Processor connection settings. Any additional
                // values will be replaced by later loads, so this step will happen again at the end of loading.
                SettingsRegistryMergeUtils::MergeSettingsToRegistry_O3deUserRegistry(
                    registry, AZ_TRAIT_OS_PLATFORM_CODENAME, specializations, &scratchBuffer);
                SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(registry, m_commandLine, {});
                // Project User Registry is merged after the command line here to allow make sure the any command line override of the
                // project path is used for merging the project's user registry
                SettingsRegistryMergeUtils::MergeSettingsToRegistry_ProjectUserRegistry(
                    registry, AZ_TRAIT_OS_PLATFORM_CODENAME, specializations, &scratchBuffer);
            }

            // Make sure the command line is merged at least once, before updating the runtime filepaths
            SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(registry, m_commandLine, {});
            SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(registry);
        }

        //! Retrieves the list gem targets that the project has load dependencies on
        //! This populates the /Amazon/Gems/<GemName> field entries which is required
        //! by the MergeSettingsToRegistry_GemRegistry() function below to locate the gem's root folder
        //! and merge in the gem's registry files.
        //! But when running from a pre-built app from the O3DE SDK(Editor/AssetProcessor), the projects binary
        //! directory is needed in order to located the load dependency registry files
        //! That project binary folder is generated with the <ProjectRoot>/user/Registry when CMake is configured
        //! for the project
        //! Therefore the order of merging must be as follows
        //! 1. MergeSettingsToRegistry_ProjectUserRegistry - Populates the /Amazon/Project/Settings/Build/project_build_path
        //!    which contains the path to the project binary directory
        //! 2. MergeSettingsToRegistry_TargetBuildDependencyRegistry - Loads the cmake_dependencies.<project_name>.<application_name>.setreg
        //!    file from the locations in order of
        //!    1. <executable_directory>/Registry
        //!    2. <cache_root>/Registry
        //!    3. <project_build_path>/bin/$<CONFIG>/Registry
        //! 3. MergeSettingsToRegistry_GemRegistries - Merges the settings registry files from each gem's <GemRoot>/Registry directory

        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_TargetBuildDependencyRegistry(
            registry, AZ_TRAIT_OS_PLATFORM_CODENAME, specializations, &scratchBuffer);

#if AZ_TRAIT_OS_IS_HOST_OS_PLATFORM
        constexpr bool overridesAllowedFromProjectRegistries =
            AZ::Internal::GetDevelopmentSettingsOverrides() == AZ::Internal::DevelopmentSettingsOverrides::ProjectOnly ||
            AZ::Internal::GetDevelopmentSettingsOverrides() == AZ::Internal::DevelopmentSettingsOverrides::CommandLineAndProject ||
            AZ::Internal::GetDevelopmentSettingsOverrides() == AZ::Internal::DevelopmentSettingsOverrides::CommandLineProjectAndUser;
        if constexpr (overridesAllowedFromProjectRegistries)
        {
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_EngineRegistry(
                registry, AZ_TRAIT_OS_PLATFORM_CODENAME, specializations, &scratchBuffer);
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_GemRegistries(
                registry, AZ_TRAIT_OS_PLATFORM_CODENAME, specializations, &scratchBuffer);
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_ProjectRegistry(
                registry, AZ_TRAIT_OS_PLATFORM_CODENAME, specializations, &scratchBuffer);
        }
#endif
    }

    void ComponentApplication::MergeUserSettings(
        SettingsRegistryInterface& registry,
        const AZ::SettingsRegistryInterface::Specializations& specializations,
        AZStd::vector<char>& scratchBuffer)
    {
        constexpr bool overridesAllowedFromCommandLine =
            AZ::Internal::GetDevelopmentSettingsOverrides() == AZ::Internal::DevelopmentSettingsOverrides::CommandLineOnly ||
            AZ::Internal::GetDevelopmentSettingsOverrides() == AZ::Internal::DevelopmentSettingsOverrides::CommandLineAndProject ||
            AZ::Internal::GetDevelopmentSettingsOverrides() == AZ::Internal::DevelopmentSettingsOverrides::CommandLineProjectAndUser;
        if constexpr (overridesAllowedFromCommandLine)
        {
            if constexpr (
                AZ::Internal::GetDevelopmentSettingsOverrides() == AZ::Internal::DevelopmentSettingsOverrides::CommandLineProjectAndUser)
            {
                AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_O3deUserRegistry(
                    registry, AZ_TRAIT_OS_PLATFORM_CODENAME, specializations, &scratchBuffer);
                AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(registry, m_commandLine, {});
                AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_ProjectUserRegistry(
                    registry, AZ_TRAIT_OS_PLATFORM_CODENAME, specializations, &scratchBuffer);
            }

            // The final merge of the command line should also execute any command-line commands.
            AZ::SettingsRegistryMergeUtils::CommandsToParse commandsToParse;
            // The regdump and regset-file arguments are parsed in the final merge
            commandsToParse.m_parseRegdumpCommands = true;
            commandsToParse.m_parseRegsetFileCommands = true;
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(registry, m_commandLine, commandsToParse);
        }

        // Update the Runtime file paths in case the "{BootstrapSettingsRootKey}/assets" key was overriden by a setting registry
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(registry);
    }

    void ComponentApplication::MergeSettingsToRegistry(SettingsRegistryInterface& registry)
    {
        if (m_startupParameters.m_loadSettingsRegistry)
        {
            SettingsRegistryInterface::Specializations specializations;
            SetSettingsRegistrySpecializations(specializations);

            AZStd::vector<char> scratchBuffer;

            MergeSharedSettings(registry, specializations, scratchBuffer);
            MergeUserSettings(registry, specializations, scratchBuffer);
        }
    }

    void ComponentApplication::SetSettingsRegistrySpecializations(SettingsRegistryInterface::Specializations& specializations)
    {
        if (m_startupParameters.m_loadSettingsRegistry)
        {
#if defined(AZ_DEBUG_BUILD)
            specializations.Append("debug");
#elif defined(AZ_PROFILE_BUILD)
            specializations.Append("profile");
#else
            specializations.Append("release");
#endif

            SettingsRegistryMergeUtils::QuerySpecializationsFromRegistry(*m_settingsRegistry, specializations);
        }
    }

    //=========================================================================
    // RegisterComponentDescriptor
    // [5/30/2012]
    //=========================================================================
    void ComponentApplication::RegisterComponentDescriptor(const ComponentDescriptor* descriptor)
    {
        if (ReflectionEnvironment::GetReflectionManager())
        {
            ReflectionEnvironment::GetReflectionManager()->Reflect(
                descriptor->GetUuid(),
                [descriptor](ReflectContext* context)
                {
                    descriptor->Reflect(context);
                });
        }
    }

    //=========================================================================
    // RegisterComponentDescriptor
    //=========================================================================
    void ComponentApplication::UnregisterComponentDescriptor(const ComponentDescriptor* descriptor)
    {
        if (ReflectionEnvironment::GetReflectionManager())
        {
            ReflectionEnvironment::GetReflectionManager()->Unreflect(descriptor->GetUuid());
        }
    }

    void ComponentApplication::RegisterEntityAddedEventHandler(EntityAddedEvent::Handler& handler)
    {
        handler.Connect(m_entityAddedEvent);
    }

    void ComponentApplication::RegisterEntityRemovedEventHandler(EntityRemovedEvent::Handler& handler)
    {
        handler.Connect(m_entityRemovedEvent);
    }

    void ComponentApplication::RegisterEntityActivatedEventHandler(EntityActivatedEvent::Handler& handler)
    {
        handler.Connect(m_entityActivatedEvent);
    }

    void ComponentApplication::RegisterEntityDeactivatedEventHandler(EntityDeactivatedEvent::Handler& handler)
    {
        handler.Connect(m_entityDeactivatedEvent);
    }

    void ComponentApplication::SignalEntityActivated(AZ::Entity* entity)
    {
        m_entityActivatedEvent.Signal(entity);
    }

    void ComponentApplication::SignalEntityDeactivated(AZ::Entity* entity)
    {
        m_entityDeactivatedEvent.Signal(entity);
    }

    //=========================================================================
    // AddEntity
    // [5/30/2012]
    //=========================================================================
    bool ComponentApplication::AddEntity(Entity* entity)
    {
        AZ_Error("ComponentApplication", entity, "Input entity is null, cannot add entity");
        if (!entity)
        {
            return false;
        }
        m_entityAddedEvent.Signal(entity);
        return m_entities.insert(AZStd::make_pair(entity->GetId(), entity)).second;
    }

    //=========================================================================
    // RemoveEntity
    // [5/31/2012]
    //=========================================================================
    bool ComponentApplication::RemoveEntity(Entity* entity)
    {
        AZ_Error("ComponentApplication", entity, "Input entity is null, cannot remove entity");
        if (!entity)
        {
            return false;
        }
        m_entityRemovedEvent.Signal(entity);
        return (m_entities.erase(entity->GetId()) == 1);
    }

    //=========================================================================
    // DeleteEntity
    // [5/31/2012]
    //=========================================================================
    bool ComponentApplication::DeleteEntity(const EntityId& id)
    {
        Entity* entity = FindEntity(id);
        if (entity)
        {
            m_entityRemovedEvent.Signal(entity);
            delete entity;
            return true;
        }
        return false;
    }

    //=========================================================================
    // FindEntity
    // [5/30/2012]
    //=========================================================================
    Entity* ComponentApplication::FindEntity(const EntityId& id)
    {
        EntitySetType::const_iterator it = m_entities.find(id);
        if (it != m_entities.end())
        {
            return it->second;
        }
        return nullptr;
    }

    //=========================================================================
    // GetEntityName
    // [10/17/2016]
    //=========================================================================
    AZStd::string ComponentApplication::GetEntityName(const EntityId& id)
    {
        Entity* entity = FindEntity(id);
        if (entity)
        {
            return entity->GetName();
        }
        return AZStd::string();
    }

    //=========================================================================
    // SetEntityName
    //=========================================================================
    bool ComponentApplication::SetEntityName(const EntityId& id, const AZStd::string_view name)
    {
        Entity* entity = FindEntity(id);
        if (entity)
        {
            entity->SetName(name);
            return true;
        }
        return false;
    }

    //=========================================================================
    // EnumerateEntities
    //=========================================================================
    void ComponentApplication::EnumerateEntities(const ComponentApplicationRequests::EntityCallback& callback)
    {
        for (const auto& entityIter : m_entities)
        {
            callback(entityIter.second);
        }
    }

    //=========================================================================
    // GetSerializeContext
    //=========================================================================
    AZ::SerializeContext* ComponentApplication::GetSerializeContext()
    {
        return ReflectionEnvironment::GetReflectionManager() ? ReflectionEnvironment::GetReflectionManager()->GetReflectContext<SerializeContext>() : nullptr;
    }

    //=========================================================================
    // GetBehaviorContext
    //=========================================================================
    AZ::BehaviorContext* ComponentApplication::GetBehaviorContext()
    {
        return ReflectionEnvironment::GetReflectionManager() ? ReflectionEnvironment::GetReflectionManager()->GetReflectContext<BehaviorContext>() : nullptr;
    }

    //=========================================================================
    // GetJsonRegistrationContext
    //=========================================================================
    AZ::JsonRegistrationContext* ComponentApplication::GetJsonRegistrationContext()
    {
        return ReflectionEnvironment::GetReflectionManager() ? ReflectionEnvironment::GetReflectionManager()->GetReflectContext<JsonRegistrationContext>() : nullptr;
    }

    /// Returns the path to the engine.

    const char* ComponentApplication::GetEngineRoot() const
    {
        static IO::FixedMaxPathString engineRoot;
        engineRoot.clear();
        m_settingsRegistry->Get(engineRoot, SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        return engineRoot.c_str();
    }

    const char* ComponentApplication::GetExecutableFolder() const
    {
        static IO::FixedMaxPathString exeFolder;
        exeFolder.clear();
        m_settingsRegistry->Get(exeFolder, SettingsRegistryMergeUtils::FilePathKey_BinaryFolder);
        return exeFolder.c_str();
    }

    //=========================================================================
    // CreateReflectionManager
    //=========================================================================
    void ComponentApplication::CreateReflectionManager()
    {
        ReflectionEnvironment::Init();

        ReflectionEnvironment::GetReflectionManager()->AddReflectContext<SerializeContext>();
        ReflectionEnvironment::GetReflectionManager()->AddReflectContext<BehaviorContext>();
        ReflectionEnvironment::GetReflectionManager()->AddReflectContext<JsonRegistrationContext>();
    }

    //=========================================================================
    // DestroyReflectionManager
    //=========================================================================
    void ComponentApplication::DestroyReflectionManager()
    {
        // Must clear before resetting so that calls to GetSerializeContext et al will succeed will unreflecting

        ReflectionEnvironment::GetReflectionManager()->Clear();
        ReflectionEnvironment::Reset();
    }

    //=========================================================================
    // CreateStaticModules
    //=========================================================================
    void ComponentApplication::CreateStaticModules(AZStd::vector<AZ::Module*>& outModules)
    {
        if (m_startupParameters.m_createStaticModulesCallback)
        {
            m_startupParameters.m_createStaticModulesCallback(outModules);
        }

        outModules.emplace_back(aznew AzCoreModule());
    }

    void ComponentApplication::LoadModules()
    {
        // Load Static Modules which is populated by CreateStaticModules()
        if (m_startupParameters.m_loadStaticModules)
        {
            LoadStaticModules();
        }

        // Load dynamic modules if appropriate for the platform
        if (m_startupParameters.m_loadDynamicModules)
        {
            LoadDynamicModules();
        }
    }

    void ComponentApplication::LoadStaticModules()
    {
        // Load static modules
        auto LoadStaticModules = [this](ModuleManagerRequests* moduleManager)
        {
            auto PopulateStaticModules = [this](AZStd::vector<AZ::Module*>& outModules) { this->CreateStaticModules(outModules); };
            moduleManager->LoadStaticModules(PopulateStaticModules, ModuleInitializationSteps::RegisterComponentDescriptors);
        };

        ModuleManagerRequestBus::Broadcast(LoadStaticModules);
    }

    void ComponentApplication::LoadDynamicModules()
    {
        // Queries the settings registry to get the list of gem modules to load
        struct GemModuleLoadData
        {
            AZ::OSString m_gemModuleName;
            AZStd::vector<AZ::OSString> m_dynamicLibraryPaths;
            bool m_autoLoad{ true };
        };

        using FixedValueString = SettingsRegistryInterface::FixedValueString;
        AZStd::vector<GemModuleLoadData> modulesLoadData;
        auto GemModuleVisitor = [&settingsRegistry = *m_settingsRegistry, &modulesLoadData]
        (const AZ::SettingsRegistryInterface::VisitArgs& activeGemArgs)
        {
            auto VisitGemObjectFields = [&settingsRegistry, &modulesLoadData](
                const AZ::SettingsRegistryInterface::VisitArgs& gemTargetArgs)
            {
                AZStd::string_view gemModuleName = gemTargetArgs.m_fieldName;
                auto FindGemModuleLoadEntry = [gemModuleName](const GemModuleLoadData& moduleLoadData)
                {
                    return gemModuleName == moduleLoadData.m_gemModuleName;
                };

                auto foundIt = AZStd::ranges::find_if(modulesLoadData, FindGemModuleLoadEntry);
                GemModuleLoadData& moduleLoadData = foundIt != modulesLoadData.end()
                    ? *foundIt
                    : modulesLoadData.emplace_back(GemModuleLoadData{ gemModuleName });

                // By default the auto load option is true
                // So auto load is turned off if option "AutoLoad" key exist and has a value of false
                auto autoLoadJsonPath = FixedValueString::format("%.*s/AutoLoad",
                    AZ_STRING_ARG(gemTargetArgs.m_jsonKeyPath));
                if (bool autoLoadModule{}; settingsRegistry.Get(autoLoadModule, autoLoadJsonPath) && !autoLoadModule)
                {
                    moduleLoadData.m_autoLoad = false;
                }

                // Locate the Module paths within the Gem Target Name object
                auto AppendDynamicModulePaths = [&settingsRegistry, &moduleLoadData]
                (const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
                {
                    if (AZ::IO::Path modulePath; settingsRegistry.Get(modulePath.Native(), visitArgs.m_jsonKeyPath))
                    {
                        moduleLoadData.m_dynamicLibraryPaths.emplace_back(AZStd::move(modulePath.Native()));
                    }

                    return AZ::SettingsRegistryInterface::VisitResponse::Skip;
                };
                auto gemModulesJsonPath = FixedValueString::format("%.*s/Modules",
                    AZ_STRING_ARG(gemTargetArgs.m_jsonKeyPath));
                AZ::SettingsRegistryVisitorUtils::VisitArray(settingsRegistry, AppendDynamicModulePaths, gemModulesJsonPath);

                return AZ::SettingsRegistryInterface::VisitResponse::Skip;
            };

            AZ::SettingsRegistryVisitorUtils::VisitField(settingsRegistry, VisitGemObjectFields,
                FixedValueString::format("%.*s/Targets", AZ_STRING_ARG(activeGemArgs.m_jsonKeyPath)));

            return AZ::SettingsRegistryInterface::VisitResponse::Skip;
        };

        ModuleDescriptorList gemModules;

        // Visit each ActiveGemsRootKey entry to retrieve the module file names and autoload state
        AZ::SettingsRegistryVisitorUtils::VisitField(*m_settingsRegistry, GemModuleVisitor,
            AZ::SettingsRegistryMergeUtils::ActiveGemsRootKey);
        for (GemModuleLoadData& moduleLoadData : modulesLoadData)
        {
            // Add all auto loadable non-asset gems to the list of gem modules to load
            if (!moduleLoadData.m_autoLoad)
            {
                continue;
            }
            for (AZ::OSString& dynamicLibraryPath : moduleLoadData.m_dynamicLibraryPaths)
            {
                auto CompareDynamicModuleDescriptor = [&dynamicLibraryPath](const DynamicModuleDescriptor& entry)
                {
                    return AZ::IO::PathView(entry.m_dynamicLibraryPath).Stem() == AZ::IO::PathView(dynamicLibraryPath).Stem();
                };
                if (auto moduleIter = AZStd::find_if(gemModules.begin(), gemModules.end(), CompareDynamicModuleDescriptor);
                    moduleIter == gemModules.end())
                {
                    gemModules.emplace_back(DynamicModuleDescriptor{ AZStd::move(dynamicLibraryPath) });
                }
            }
        }

        // The modules in the settings registry are prioritized to load before the modules in the ComponentApplication descriptor
        // in the order in which they were found
        for (auto&& moduleDescriptor : m_descriptor.m_modules)
        {
            // Append new dynamic library modules to the descriptor array
            auto CompareDynamicModuleDescriptor = [&moduleDescriptor](const DynamicModuleDescriptor& entry)
            {
                return entry.m_dynamicLibraryPath.contains(moduleDescriptor.m_dynamicLibraryPath);
            };
            if (auto foundModuleIter = AZStd::find_if(gemModules.begin(), gemModules.end(), CompareDynamicModuleDescriptor);
                foundModuleIter == gemModules.end())
            {
                gemModules.emplace_back(AZStd::move(moduleDescriptor));
            }

        }

        // All dynamic modules have been gathered at this point, and each dynamic module will be up until follow three phases:
        // 1. Load - the first call is to ensure all dynamic modules are loaded
        // 2. CreateClass - the second call is to create specific AZ::Module class instances for each dynamic module after its loaded
        // 3. RegisterComponentDescriptors - the third call is to perform a horizontal register step for each module component descriptos after
        //    module has been loaded and created
        for (ModuleInitializationSteps lastStepToPerform : { ModuleInitializationSteps::Load,
            ModuleInitializationSteps::CreateClass, ModuleInitializationSteps::RegisterComponentDescriptors })
        {
            AZ::ModuleManagerRequests::LoadModulesResult loadModuleOutcomes;
            ModuleManagerRequestBus::BroadcastResult(
                loadModuleOutcomes, &ModuleManagerRequests::LoadDynamicModules, gemModules, lastStepToPerform, true);

#if defined(AZ_ENABLE_TRACING)
            for (const auto& loadModuleOutcome : loadModuleOutcomes)
            {
                AZ_Error("ComponentApplication", loadModuleOutcome.IsSuccess(), "%s", loadModuleOutcome.GetError().c_str());
            }
#endif
        }
    }

    void ComponentApplication::Tick()
    {
        AZ_PROFILE_SCOPE(System, "Component application simulation tick");

        // Only record when the record metrics on tick callback is set
        if (m_recordMetricsOnTickCallback)
        {
            auto currentMonotonicTime = AZStd::chrono::steady_clock::now();

            if (m_recordMetricsOnTickCallback(currentMonotonicTime))
            {
                AZ::Metrics::EventObjectStorage argsContainer;
                argsContainer.emplace_back("frameTimeMicroseconds", static_cast<AZ::u64>(
                    AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(currentMonotonicTime - m_lastTickTime).count()));
                AZ::Metrics::AsyncArgs asyncArgs;
                asyncArgs.m_name = "FrameTime";
                asyncArgs.m_cat = "Core";
                asyncArgs.m_args = argsContainer;
                asyncArgs.m_id = "Simulation";
                asyncArgs.m_scope = "Engine";

                [[maybe_unused]] auto metricsOutcome = AZ::Metrics::RecordAsyncEventInstant(AZ::Metrics::CoreEventLoggerId, asyncArgs, m_eventLoggerFactory.get());

                AZ_ErrorOnce("ComponentApplication", metricsOutcome.IsSuccess(),
                    "Failed to record frame time metrics. Error %s", metricsOutcome.GetError().c_str());
            }

            // Update the m_lastTickTime to the current monotonic time
            m_lastTickTime = currentMonotonicTime;
        }

        {
            AZ_PROFILE_SCOPE(AzCore, "ComponentApplication::Tick:ExecuteQueuedEvents");
            TickBus::ExecuteQueuedEvents();
        }

        {
            AZ_PROFILE_SCOPE(AzCore, "ComponentApplication::Tick:OnTick");
            const AZ::TimeUs deltaTimeUs = m_timeSystem->AdvanceTickDeltaTimes();
            const float deltaTimeSeconds = AZ::TimeUsToSeconds(deltaTimeUs);
            AZ::TickBus::Broadcast(&TickEvents::OnTick, deltaTimeSeconds, GetTimeAtCurrentTick());
        }

        m_timeSystem->ApplyTickRateLimiterIfNeeded();
    }

    void ComponentApplication::TickSystem()
    {
        AZ_PROFILE_SCOPE(System, "Component application tick");

        SystemTickBus::ExecuteQueuedEvents();
        SystemTickBus::Broadcast(&SystemTickBus::Events::OnSystemTick);
    }

    bool ComponentApplication::ShouldAddSystemComponent(AZ::ComponentDescriptor* descriptor)
    {
        // NOTE: This is different than modules! All system components must be listed in GetRequiredSystemComponents, and then AZ::Edit::Attributes::SystemComponentTags may be used to filter down from there
        if (m_moduleManager->GetSystemComponentTags().empty())
        {
            return true;
        }

        const SerializeContext::ClassData* classData = GetSerializeContext()->FindClassData(descriptor->GetUuid());
        AZ_Warning("ComponentApplication", classData, "Component type %s not reflected to SerializeContext!", descriptor->GetName());

        // Note, if there are no SystemComponentTags on the classData, we will return true
        // in order to maintain backwards compatibility with legacy non-tagged components
        return Edit::SystemComponentTagsMatchesAtLeastOneTag(classData, m_moduleManager->GetSystemComponentTags(), true);
    }

    //=========================================================================
    // AddRequiredSystemComponents
    //=========================================================================
    void ComponentApplication::AddRequiredSystemComponents(AZ::Entity* systemEntity)
    {
        //
        // Gather required system components from all modules and the application.
        //
        for (const Uuid& componentId : GetRequiredSystemComponents())
        {
            ComponentDescriptor* componentDescriptor = nullptr;
            ComponentDescriptorBus::EventResult(componentDescriptor, componentId, &ComponentDescriptorBus::Events::GetDescriptor);
            if (!componentDescriptor)
            {
                AZ_Error("Module", false, "Failed to add system component required by application. No component descriptor found for: %s",
                    componentId.ToString<AZStd::string>().c_str());
                continue;
            }

            if (ShouldAddSystemComponent(componentDescriptor))
            {
                // add component if it's not already present
                if (!systemEntity->FindComponent(componentId))
                {
                    systemEntity->AddComponent(componentDescriptor->CreateComponent());
                }
            }
        }
    }

    void ComponentApplication::ResolveModulePath([[maybe_unused]] AZ::OSString& modulePath)
    {
        // No special parsing of the Module Path is done by the Component Application anymore
    }

    AZ::CommandLine* ComponentApplication::GetAzCommandLine()
    {
        return &m_commandLine;
    }

    int* ComponentApplication::GetArgC()
    {
        return &m_argC;
    }

    char*** ComponentApplication::GetArgV()
    {
        return &m_argV;
    }

    void ComponentApplication::QueryApplicationType(AZ::ApplicationTypeQuery& appType) const
    {
        appType.m_maskValue = AZ::ApplicationTypeQuery::Masks::Invalid;
    }

    float ComponentApplication::GetTickDeltaTime()
    {
        const AZ::TimeUs gameTickTime = m_timeSystem->GetSimulationTickDeltaTimeUs();
        return AZ::TimeUsToSeconds(gameTickTime);
    }

    //=========================================================================
    // GetTime
    // [1/22/2016]
    //=========================================================================
    ScriptTimePoint ComponentApplication::GetTimeAtCurrentTick()
    {
        const AZ::TimeUs lastGameTickTime = m_timeSystem->GetLastSimulationTickTime();
        return ScriptTimePoint(AZ::TimeUsToChrono(lastGameTickTime));
    }

    //=========================================================================
    // Reflect
    //=========================================================================
    void ComponentApplication::Reflect(ReflectContext* context)
    {
        // reflect default entity
        Entity::Reflect(context);
        // reflect module manager
        ModuleManager::Reflect(context);
        // reflect descriptor
        Descriptor::Reflect(context, this);
        // reflect vertex container
        VertexContainerReflect(context);
        // reflect spline and associated data
        SplineReflect(context);
        // reflect polygon prism
        PolygonPrismReflect(context);
        // reflect name dictionary.
        Name::Reflect(context);
        // reflect path
        IO::PathReflect(context);
        // reflect the SettingsRegistryInterface and SettingsRegistryImpl
        AZ::SettingsRegistryScriptUtils::ReflectSettingsRegistry(context);
    }
} // namespace AZ
