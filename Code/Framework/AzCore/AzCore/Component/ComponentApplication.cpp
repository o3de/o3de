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

#include <AzCore/Debug/LocalFileEventLogger.h>

#include <AzCore/Memory/AllocationRecords.h>

#include <AzCore/Memory/OverrunDetectionAllocator.h>
#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Memory/MallocSchema.h>

#include <AzCore/NativeUI/NativeUIRequests.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>

#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>

#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Settings/SettingsRegistryScriptUtils.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <AzCore/Module/Module.h>
#include <AzCore/Module/ModuleManager.h>

#include <AzCore/IO/SystemFile.h>

#include <AzCore/Driller/Driller.h>
#include <AzCore/Memory/MemoryDriller.h>
#include <AzCore/Debug/TraceMessagesDriller.h>
#include <AzCore/Debug/EventTraceDriller.h>
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

static void PrintEntityName(const AZ::ConsoleCommandContainer& arguments)
{
    if (arguments.empty())
    {
        return;
    }

    const auto entityIdStr = AZStd::string(arguments.front());
    const auto entityIdValue = AZStd::stoull(entityIdStr);

    AZStd::string entityName;
    AZ::ComponentApplicationBus::BroadcastResult(
        entityName, &AZ::ComponentApplicationBus::Events::GetEntityName, AZ::EntityId(entityIdValue));

    AZ_Printf("Entity Debug", "EntityId: %" PRIu64 ", Entity Name: %s", entityIdValue, entityName.c_str());
}

AZ_CONSOLEFREEFUNC(
    PrintEntityName, AZ::ConsoleFunctorFlags::Null, "Parameter: EntityId value, Prints the name of the entity to the console");

namespace AZ
{
    static EnvironmentVariable<OverrunDetectionSchema> s_overrunDetectionSchema;

    static EnvironmentVariable<MallocSchema> s_mallocSchema;

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

    ComponentApplication::EventLoggerDeleter::EventLoggerDeleter() noexcept= default;
    ComponentApplication::EventLoggerDeleter::EventLoggerDeleter(bool skipDelete) noexcept
        : m_skipDelete{skipDelete}
    {}
    void ComponentApplication::EventLoggerDeleter::operator()(AZ::Debug::LocalFileEventLogger* ptr)
    {
        if (!m_skipDelete)
        {
            delete ptr;
        }
    }

    //=========================================================================
    // ComponentApplication::Descriptor
    // [5/30/2012]
    //=========================================================================
    ComponentApplication::Descriptor::Descriptor()
    {
        m_useExistingAllocator = false;
        m_grabAllMemory = false;
        m_allocationRecords = true;
        m_allocationRecordsSaveNames = false;
        m_allocationRecordsAttemptDecodeImmediately = false;
        m_autoIntegrityCheck = false;
        m_markUnallocatedMemory = true;
        m_doNotUsePools = false;
        m_enableScriptReflection = true;

        m_pageSize = SystemAllocator::Descriptor::Heap::m_defaultPageSize;
        m_poolPageSize = SystemAllocator::Descriptor::Heap::m_defaultPoolPageSize;
        m_memoryBlockAlignment = SystemAllocator::Descriptor::Heap::m_memoryBlockAlignment;
        m_memoryBlocksByteSize = 0;
        m_reservedOS = 0;
        m_reservedDebug = 0;
        m_recordingMode = Debug::AllocationRecords::RECORD_STACK_IF_NO_FILE_LINE;
        m_stackRecordLevels = 5;
        m_enableDrilling = false;
        m_useOverrunDetection = false;
        m_useMalloc = false;
    }

    bool AppDescriptorConverter(SerializeContext& serialize, SerializeContext::DataElementNode& node)
    {
        if (node.GetVersion() < 2)
        {
            int nodeIdx = node.FindElement(AZ_CRC("recordsMode", 0x764c147a));
            if (nodeIdx != -1)
            {
                auto& subNode = node.GetSubElement(nodeIdx);

                char oldValue = 0;
                subNode.GetData(oldValue);
                subNode.Convert<Debug::AllocationRecords::Mode>(serialize);
                subNode.SetData<Debug::AllocationRecords::Mode>(serialize, aznumeric_caster(oldValue));
                subNode.SetName("recordingMode");
            }

            nodeIdx = node.FindElement(AZ_CRC("stackRecordLevels", 0xf8492566));
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

        void operator()(AZStd::string_view path, AZ::SettingsRegistryInterface::Type)
        {
            // Update the project settings when the project path is set
            using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
            const auto projectPathKey = FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";

            AZ::IO::FixedMaxPath newProjectPath;
            if (SettingsRegistryMergeUtils::IsPathAncestorDescendantOrEqual(projectPathKey, path)
                && m_registry.Get(newProjectPath.Native(), projectPathKey) && newProjectPath != m_oldProjectPath)
            {
                // Update old Project path before attempting to merge in new Settings Registry values in order to prevent recursive calls
                m_oldProjectPath = newProjectPath;

                // Merge the project.json file into settings registry under ProjectSettingsRootKey path.
                AZ::IO::FixedMaxPath projectMetadataFile{ AZ::SettingsRegistryMergeUtils::FindEngineRoot(m_registry) / newProjectPath };
                projectMetadataFile /= "project.json";
                m_registry.MergeSettingsFile(projectMetadataFile.Native(),
                    AZ::SettingsRegistryInterface::Format::JsonMergePatch, AZ::SettingsRegistryMergeUtils::ProjectSettingsRootKey);

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

        void operator()(AZStd::string_view path, AZ::SettingsRegistryInterface::Type)
        {
            // Update the project specialization when the project name is set
            using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
            const auto projectNameKey = FixedValueString(AZ::SettingsRegistryMergeUtils::ProjectSettingsRootKey) + "/project_name";

            FixedValueString newProjectName;
            if (SettingsRegistryMergeUtils::IsPathAncestorDescendantOrEqual(projectNameKey, path)
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

        void operator()(AZStd::string_view path, AZ::SettingsRegistryInterface::Type)
        {
            // Update the ComponentApplication CommandLine instance when the command line settings are merged into the Settings Registry
            if (path == AZ::SettingsRegistryMergeUtils::CommandLineValueChangedKey)
            {
                AZ::SettingsRegistryMergeUtils::GetCommandLineFromRegistry(m_registry, m_commandLine);
            }
        }

    private:
        AZ::SettingsRegistryInterface& m_registry;
        AZ::CommandLine& m_commandLine;
    };

    void ComponentApplication::Descriptor::AllocatorRemapping::Reflect(ReflectContext* context, ComponentApplication* app)
    {
        (void)app;

        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<AllocatorRemapping>()
                ->Field("from", &AllocatorRemapping::m_from)
                ->Field("to", &AllocatorRemapping::m_to)
                ;
        }
    }

    //=========================================================================
    // Reflect
    //=========================================================================
    void  ComponentApplication::Descriptor::Reflect(ReflectContext* context, ComponentApplication* app)
    {
        DynamicModuleDescriptor::Reflect(context);
        AllocatorRemapping::Reflect(context, app);

        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<Descriptor>(&app->GetDescriptor())
                ->Version(2, AppDescriptorConverter)
                ->Field("useExistingAllocator", &Descriptor::m_useExistingAllocator)
                ->Field("grabAllMemory", &Descriptor::m_grabAllMemory)
                ->Field("allocationRecords", &Descriptor::m_allocationRecords)
                ->Field("allocationRecordsSaveNames", &Descriptor::m_allocationRecordsSaveNames)
                ->Field("allocationRecordsAttemptDecodeImmediately", &Descriptor::m_allocationRecordsAttemptDecodeImmediately)
                ->Field("recordingMode", &Descriptor::m_recordingMode)
                ->Field("stackRecordLevels", &Descriptor::m_stackRecordLevels)
                ->Field("autoIntegrityCheck", &Descriptor::m_autoIntegrityCheck)
                ->Field("markUnallocatedMemory", &Descriptor::m_markUnallocatedMemory)
                ->Field("doNotUsePools", &Descriptor::m_doNotUsePools)
                ->Field("enableScriptReflection", &Descriptor::m_enableScriptReflection)
                ->Field("pageSize", &Descriptor::m_pageSize)
                ->Field("poolPageSize", &Descriptor::m_poolPageSize)
                ->Field("blockAlignment", &Descriptor::m_memoryBlockAlignment)
                ->Field("blockSize", &Descriptor::m_memoryBlocksByteSize)
                ->Field("reservedOS", &Descriptor::m_reservedOS)
                ->Field("reservedDebug", &Descriptor::m_reservedDebug)
                ->Field("enableDrilling", &Descriptor::m_enableDrilling)
                ->Field("useOverrunDetection", &Descriptor::m_useOverrunDetection)
                ->Field("useMalloc", &Descriptor::m_useMalloc)
                ->Field("allocatorRemappings", &Descriptor::m_allocatorRemappings)
                ->Field("modules", &Descriptor::m_modules)
                ;

            if (EditContext* ec = serializeContext->GetEditContext())
            {
                ec->Enum<Debug::AllocationRecords::Mode>("Debug::AllocationRecords::Mode", "Allocator recording mode")
                    ->Value("No records", Debug::AllocationRecords::RECORD_NO_RECORDS)
                    ->Value("No stack trace", Debug::AllocationRecords::RECORD_STACK_NEVER)
                    ->Value("Stack trace when file/line missing", Debug::AllocationRecords::RECORD_STACK_IF_NO_FILE_LINE)
                    ->Value("Stack trace always", Debug::AllocationRecords::RECORD_FULL);
                ec->Class<Descriptor>("System memory settings", "Settings for managing application memory usage")
                    ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                    ->DataElement(Edit::UIHandlers::CheckBox, &Descriptor::m_grabAllMemory, "Allocate all memory at startup", "Allocate all system memory at startup if enabled, or allocate as needed if disabled")
                    ->DataElement(Edit::UIHandlers::CheckBox, &Descriptor::m_allocationRecords, "Record allocations", "Collect information on each allocation made for debugging purposes (ignored in Release builds)")
                    ->DataElement(Edit::UIHandlers::CheckBox, &Descriptor::m_allocationRecordsSaveNames, "Record allocations with name saving", "Saves names/filenames information on each allocation made, useful for tracking down leaks in dynamic modules (ignored in Release builds)")
                    ->DataElement(Edit::UIHandlers::CheckBox, &Descriptor::m_allocationRecordsAttemptDecodeImmediately, "Record allocations and attempt immediate decode", "Decode callstacks for each allocation when they occur, used for tracking allocations that fail decoding. Very expensive. (ignored in Release builds)")
                    ->DataElement(Edit::UIHandlers::ComboBox, &Descriptor::m_recordingMode, "Stack recording mode", "Stack record mode. (Ignored in final builds)")
                    ->DataElement(Edit::UIHandlers::SpinBox, &Descriptor::m_stackRecordLevels, "Stack entries to record", "Number of stack levels to record for each allocation (ignored in Release builds)")
                        ->Attribute(Edit::Attributes::Step, 1)
                        ->Attribute(Edit::Attributes::Max, 1024)
                    ->DataElement(Edit::UIHandlers::CheckBox, &Descriptor::m_autoIntegrityCheck, "Validate allocations", "Check allocations for integrity on each allocation/free (ignored in Release builds)")
                    ->DataElement(Edit::UIHandlers::CheckBox, &Descriptor::m_markUnallocatedMemory, "Mark freed memory", "Set memory to 0xcd when a block is freed for debugging (ignored in Release builds)")
                    ->DataElement(Edit::UIHandlers::CheckBox, &Descriptor::m_doNotUsePools, "Don't pool allocations", "Pipe pool allocations in system/tree heap (ignored in Release builds)")
                    ->DataElement(Edit::UIHandlers::SpinBox, &Descriptor::m_pageSize, "Page size", "Memory page size in bytes (must be OS page size aligned)")
                        ->Attribute(Edit::Attributes::Step, 1024)
                    ->DataElement(Edit::UIHandlers::SpinBox, &Descriptor::m_poolPageSize, "Pool page size", "Memory pool page size in bytes (must be a multiple of page size)")
                        ->Attribute(Edit::Attributes::Max, &Descriptor::m_pageSize)
                        ->Attribute(Edit::Attributes::Step, 1024)
                    ->DataElement(Edit::UIHandlers::SpinBox, &Descriptor::m_memoryBlockAlignment, "Block alignment", "Memory block alignment in bytes (must be multiple of the page size)")
                        ->Attribute(Edit::Attributes::Step, &Descriptor::m_pageSize)
                    ->DataElement(Edit::UIHandlers::SpinBox, &Descriptor::m_memoryBlocksByteSize, "Block size", "Memory block size in bytes (must be multiple of the page size)")
                        ->Attribute(Edit::Attributes::Step, &Descriptor::m_pageSize)
                    ->DataElement(Edit::UIHandlers::SpinBox, &Descriptor::m_reservedOS, "OS reserved memory", "System memory reserved for OS (used only when 'Allocate all memory at startup' is true)")
                    ->DataElement(Edit::UIHandlers::SpinBox, &Descriptor::m_reservedDebug, "Memory reserved for debugger", "System memory reserved for Debug allocator, like memory tracking (used only when 'Allocate all memory at startup' is true)")
                    ->DataElement(Edit::UIHandlers::CheckBox, &Descriptor::m_enableDrilling, "Enable Driller", "Enable Drilling support for the application (ignored in Release builds)")
                    ->DataElement(Edit::UIHandlers::CheckBox, &Descriptor::m_useOverrunDetection, "Use Overrun Detection", "Use the overrun detection memory manager (only available on some platforms, ignored in Release builds)")
                    ->DataElement(Edit::UIHandlers::CheckBox, &Descriptor::m_useMalloc, "Use Malloc", "Use malloc for memory allocations (for memory debugging only, ignored in Release builds)")
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
        }
    }

    //=========================================================================
    // Create
    //=========================================================================
    void* ComponentApplication::Descriptor::Create(const char* name)
    {
        (void)name;
        return this; /// we the the factory and the object as we are part of the component application
    }

    //=========================================================================
    // Destroy
    //=========================================================================
    void ComponentApplication::Descriptor::Destroy(void* data)
    {
        // do nothing as descriptor is part of the component application
        (void)data;
    }

    //=========================================================================
    // ComponentApplication
    // [5/30/2012]
    //=========================================================================
    ComponentApplication::ComponentApplication()
        : ComponentApplication(0, nullptr)
    {
        if (Interface<ComponentApplicationRequests>::Get() == nullptr)
        {
            Interface<ComponentApplicationRequests>::Register(this);
        }
    }

    ComponentApplication::ComponentApplication(int argC, char** argV)
        : m_eventLogger{}
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

        // Create the Event logger if it doesn't exist, otherwise reuse the one registered
        // with the AZ::Interface
        if (AZ::Interface<AZ::Debug::IEventLogger>::Get() == nullptr)
        {
            m_eventLogger.reset(new AZ::Debug::LocalFileEventLogger);
        }
        else
        {
            m_eventLogger = EventLoggerPtr(static_cast<AZ::Debug::LocalFileEventLogger*>(AZ::Interface<AZ::Debug::IEventLogger>::Get()),
                EventLoggerDeleter{ true });
        }

        // Initializes the OSAllocator and SystemAllocator as soon as possible
        CreateOSAllocator();
        CreateSystemAllocator();

        // Now that the Allocators are initialized, the Command Line parameters can be parsed
        m_commandLine.Parse(m_argC, m_argV);
        SettingsRegistryMergeUtils::ParseCommandLine(m_commandLine);

        // Create the settings registry and register it with the AZ interface system
        // This is done after the AppRoot has been calculated so that the Bootstrap.cfg
        // can be read to determine the Game folder and the asset platform
        m_settingsRegistry = AZStd::make_unique<SettingsRegistryImpl>();

        // Register the Settings Registry with the AZ Interface if there isn't one registered already
        if (SettingsRegistry::Get() == nullptr)
        {
            SettingsRegistry::Register(m_settingsRegistry.get());
        }

        // Add the Command Line arguments into the SettingsRegistry
        SettingsRegistryMergeUtils::StoreCommandLineToRegistry(*m_settingsRegistry, m_commandLine);

        // Add a notifier to update the project_settings when
        // 1. The 'project_path' key changes
        // 2. The project specialization when the 'project-name' key changes
        // 3. The ComponentApplication command line when the command line is stored to the registry
        m_projectPathChangedHandler = m_settingsRegistry->RegisterNotifier(ProjectPathChangedEventHandler{
            *m_settingsRegistry });
        m_projectNameChangedHandler = m_settingsRegistry->RegisterNotifier(ProjectNameChangedEventHandler{
            *m_settingsRegistry });
        m_commandLineUpdatedHandler = m_settingsRegistry->RegisterNotifier(UpdateCommandLineEventHandler{
            *m_settingsRegistry, m_commandLine });

        // Merge Command Line arguments
        constexpr bool executeRegDumpCommands = false;
        SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(*m_settingsRegistry, m_commandLine, executeRegDumpCommands);

        // Query for the Executable Path using OS specific functions
        CalculateExecutablePath();

        // Determine the path to the engine
        CalculateEngineRoot();

        // If the current platform returns an engaged optional from Utils::GetDefaultAppRootPath(), that is used
        // for the application root.
        CalculateAppRoot();

        SettingsRegistryMergeUtils::MergeSettingsToRegistry_O3deUserRegistry(*m_settingsRegistry, AZ_TRAIT_OS_PLATFORM_CODENAME, {});
        SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(*m_settingsRegistry, m_commandLine, executeRegDumpCommands);
        SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*m_settingsRegistry);

        // The /O3DE/Application/LifecycleEvents array contains a valid set of lifecycle events
        // Those lifecycle events are normally read from the <engine-root>/Registry
        // which isn't merged until ComponentApplication::Create invokes MergeSettingsToRegistry
        // So pre-populate the valid lifecycle even entries
        ComponentApplicationLifecycle::RegisterEvent(*m_settingsRegistry, "SystemAllocatorCreated");
        ComponentApplicationLifecycle::RegisterEvent(*m_settingsRegistry, "SettingsRegistryAvailable");
        ComponentApplicationLifecycle::RegisterEvent(*m_settingsRegistry, "ConsoleAvailable");
        ComponentApplicationLifecycle::SignalEvent(*m_settingsRegistry, "SystemAllocatorCreated", R"({})");
        ComponentApplicationLifecycle::SignalEvent(*m_settingsRegistry, "SettingsRegistryAvailable", R"({})");

        // Create the Module Manager
        m_moduleManager = AZStd::make_unique<ModuleManager>();

        // Az Console initialization..
        // note that tests destroy and construct the application over and over, which is not a desirable pattern
        // so we allow the console to construct once and skip destruction / construction on consecutive runs
        m_console = AZ::Interface<AZ::IConsole>::Get();
        if (m_console == nullptr)
        {
            m_console = aznew AZ::Console(*m_settingsRegistry);
            AZ::Interface<AZ::IConsole>::Register(m_console);
            m_ownsConsole = true;
            m_console->LinkDeferredFunctors(AZ::ConsoleFunctorBase::GetDeferredHead());
            m_settingsRegistryConsoleFunctors = AZ::SettingsRegistryConsoleUtils::RegisterAzConsoleCommands(*m_settingsRegistry, *m_console);
            ComponentApplicationLifecycle::SignalEvent(*m_settingsRegistry, "ConsoleAvailable", R"({})");
        }
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
        if (m_ownsConsole)
        {
            AZ::Interface<AZ::IConsole>::Unregister(m_console);
            delete m_console;
            ComponentApplicationLifecycle::SignalEvent(*m_settingsRegistry, "ConsoleUnavailable", R"({})");
        }

        m_moduleManager.reset();
        // Unregister the global Settings Registry if it is owned by this application instance
        if (AZ::SettingsRegistry::Get() == m_settingsRegistry.get())
        {
            SettingsRegistry::Unregister(m_settingsRegistry.get());
            ComponentApplicationLifecycle::SignalEvent(*m_settingsRegistry, "SettingsRegistryUnavailable", R"({})");
            ComponentApplicationLifecycle::SignalEvent(*m_settingsRegistry, "SystemAllocatorPendingDestruction", R"({})");
        }
        m_settingsRegistry.reset();

        // Set AZ::CommandLine to an empty object to clear out allocated memory before the allocators
        // are destroyed
        m_commandLine = {};

        m_entityAddedEvent.DisconnectAllHandlers();
        m_entityRemovedEvent.DisconnectAllHandlers();
        m_entityActivatedEvent.DisconnectAllHandlers();
        m_entityDeactivatedEvent.DisconnectAllHandlers();

#if !defined(_RELEASE)
        m_budgetTracker.Reset();
#endif

        DestroyAllocator();
    }


    void ReportBadEngineRoot()
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

        if (m_engineRoot.empty())
        {
            ReportBadEngineRoot();
            return nullptr;
        }

        m_startupParameters = startupParameters;

        m_descriptor = descriptor;

        // Re-invokes CreateOSAllocator and CreateSystemAllocator function to allow the component application
        // to use supplied startupParameters and descriptor parameters this time
        CreateOSAllocator();
        CreateSystemAllocator();

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
        {
            AZ::IO::FixedMaxPath outputPath;
            m_settingsRegistry->Get(outputPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_DevWriteStorage);
            outputPath /= "eventlogger";

            AZ::IO::FixedMaxPathString baseFileName{ "EventLog" }; // default name
            m_settingsRegistry->Get(baseFileName, AZ::SettingsRegistryMergeUtils::BuildTargetNameKey);

            m_eventLogger->Start(outputPath.Native(), baseFileName);
        }

        Sfmt::Create();

        CreateReflectionManager();

        if (m_startupParameters.m_createEditContext)
        {
            GetSerializeContext()->CreateEditContext();
        }

        NameDictionary::Create();

        // Call this and child class's reflects
        ReflectionEnvironment::GetReflectionManager()->Reflect(azrtti_typeid(this), [this](ReflectContext* context) {Reflect(context); });

        RegisterCoreComponents();
        ComponentApplicationLifecycle::SignalEvent(*m_settingsRegistry, "ReflectionManagerAvailable", R"({})");

        TickBus::AllowFunctionQueuing(true);
        SystemTickBus::AllowFunctionQueuing(true);

        ComponentApplicationBus::Handler::BusConnect();

        m_currentTime = AZStd::chrono::system_clock::now();
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
        AZ::IO::FixedMaxPath platformCachePath;
        m_settingsRegistry->Get(platformCachePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder);
        m_console->ExecuteConfigFile((platformCachePath / "user.cfg").Native());

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

        // Uninit and unload any dynamic modules.
        m_moduleManager->UnloadModules();
        ComponentApplicationLifecycle::SignalEvent(*m_settingsRegistry, "GemsUnloaded", R"({})");

        NameDictionary::Destroy();

        m_systemEntity.reset();

        Sfmt::Destroy();

        // delete all descriptors left for application clean up
        EBUS_EVENT(ComponentDescriptorBus, ReleaseDescriptor);

        // Disconnect from application and tick request buses
        ComponentApplicationBus::Handler::BusDisconnect();
        TickRequestBus::Handler::BusDisconnect();

        m_eventLogger->Stop();

        // Clear the descriptor to deallocate all strings (owned by ModuleDescriptor)
        m_descriptor = Descriptor();

        m_isStarted = false;

#if defined(AZ_ENABLE_DEBUG_TOOLS)
        // Unregister module listeners after allocators are destroyed
        // so that symbol/stack trace information is available at shutdown
        Debug::SymbolStorage::UnregisterModuleListeners();
#endif // defined(AZ_ENABLE_DEBUG_TOOLS)
    }

    void ComponentApplication::DestroyAllocator()
    {
        // kill the system allocator if we created it
        if (m_isSystemAllocatorOwner)
        {
            AZ::Debug::Trace::Instance().Destroy();
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();

            if (m_fixedMemoryBlock)
            {
                m_osAllocator->DeAllocate(m_fixedMemoryBlock);
            }
            m_fixedMemoryBlock = nullptr;
            m_isSystemAllocatorOwner = false;
        }

        s_overrunDetectionSchema.Reset();
        s_mallocSchema.Reset();
        if (m_isOSAllocatorOwner)
        {
            AZ::AllocatorInstance<AZ::OSAllocator>::Destroy();
            m_isOSAllocatorOwner = false;
        }

        m_osAllocator = nullptr;
    }

    void ComponentApplication::CreateOSAllocator()
    {
        if (!m_startupParameters.m_allocator)
        {
            if (!AZ::AllocatorInstance<AZ::OSAllocator>::IsReady())
            {
                AZ::AllocatorInstance<AZ::OSAllocator>::Create();
                m_isOSAllocatorOwner = true;
            }
            m_osAllocator = &AZ::AllocatorInstance<AZ::OSAllocator>::Get();
        }
        else
        {
            m_osAllocator = m_startupParameters.m_allocator;
        }
    }

    //=========================================================================
    // CreateSystemAllocator
    // [5/30/2012]
    //=========================================================================
    void ComponentApplication::CreateSystemAllocator()
    {
        if (m_descriptor.m_useExistingAllocator || AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
        {
            AZ_Assert(AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady(), "You must setup AZ::SystemAllocator instance, before you can call Create application with m_useExistingAllocator flag set to true");
            return;
        }
        else
        {
            // Create the system allocator
            AZ::SystemAllocator::Descriptor desc;
            desc.m_heap.m_pageSize = m_descriptor.m_pageSize;
            desc.m_heap.m_poolPageSize = m_descriptor.m_poolPageSize;
            if (m_descriptor.m_grabAllMemory)
            {
                // grab all available memory
                AZ::u64 availableOS = AZ_CORE_MAX_ALLOCATOR_SIZE;
                AZ::u64 reservedOS = m_descriptor.m_reservedOS;
                AZ::u64 reservedDbg = m_descriptor.m_reservedDebug;
                AZ_Warning("Memory", false, "This platform is not supported for grabAllMemory flag! Provide a valid allocation size and set the m_grabAllMemory flag to false! Using default max memory size %llu!", availableOS);
                AZ_Assert(availableOS > 0, "OS doesn't have any memory available!");
                // compute total memory to grab
                desc.m_heap.m_fixedMemoryBlocksByteSize[0] = static_cast<size_t>(availableOS - reservedOS - reservedDbg);
                // memory block MUST be a multiple of pages
                desc.m_heap.m_fixedMemoryBlocksByteSize[0] = AZ::SizeAlignDown(desc.m_heap.m_fixedMemoryBlocksByteSize[0], m_descriptor.m_pageSize);
            }
            else
            {
                desc.m_heap.m_fixedMemoryBlocksByteSize[0] = static_cast<size_t>(m_descriptor.m_memoryBlocksByteSize);
            }

            if (desc.m_heap.m_fixedMemoryBlocksByteSize[0] > 0) // 0 means one demand memory which we support
            {
                m_fixedMemoryBlock = m_osAllocator->Allocate(desc.m_heap.m_fixedMemoryBlocksByteSize[0], m_descriptor.m_memoryBlockAlignment);
                desc.m_heap.m_fixedMemoryBlocks[0] = m_fixedMemoryBlock;
                desc.m_heap.m_numFixedMemoryBlocks = 1;
            }
            desc.m_allocationRecords = m_descriptor.m_allocationRecords;
            desc.m_stackRecordLevels = aznumeric_caster(m_descriptor.m_stackRecordLevels);
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create(desc);
            AZ::Debug::Trace::Instance().Init();

            AZ::Debug::AllocationRecords* records = AllocatorInstance<SystemAllocator>::GetAllocator().GetRecords();
            if (records)
            {
                records->SetMode(m_descriptor.m_recordingMode);
                records->SetSaveNames(m_descriptor.m_allocationRecordsSaveNames);
                records->SetDecodeImmediately(m_descriptor.m_allocationRecordsAttemptDecodeImmediately);
                records->AutoIntegrityCheck(m_descriptor.m_autoIntegrityCheck);
                records->MarkUallocatedMemory(m_descriptor.m_markUnallocatedMemory);
            }

            m_isSystemAllocatorOwner = true;
        }

#ifndef RELEASE
        if (m_descriptor.m_useOverrunDetection)
        {
            OverrunDetectionSchema::Descriptor overrunDesc(false);
            s_overrunDetectionSchema = Environment::CreateVariable<OverrunDetectionSchema>(AzTypeInfo<OverrunDetectionSchema>::Name(), overrunDesc);
            OverrunDetectionSchema* schemaPtr = &s_overrunDetectionSchema.Get();

            AZ::AllocatorManager::Instance().SetOverrideAllocatorSource(schemaPtr);
        }

        if (m_descriptor.m_useMalloc)
        {
            AZ_Printf("Malloc", "WARNING: Malloc override is enabled. Registered allocators will use malloc instead of their normal allocation schemas.");
            s_mallocSchema = Environment::CreateVariable<MallocSchema>(AzTypeInfo<MallocSchema>::Name());
            MallocSchema* schemaPtr = &s_mallocSchema.Get();

            AZ::AllocatorManager::Instance().SetOverrideAllocatorSource(schemaPtr);
        }
#endif

        AllocatorManager& allocatorManager = AZ::AllocatorManager::Instance();

        for (const auto& remapping : m_descriptor.m_allocatorRemappings)
        {
            allocatorManager.AddAllocatorRemapping(remapping.m_from.c_str(), remapping.m_to.c_str());
        }

        allocatorManager.FinalizeConfiguration();
    }

    void ComponentApplication::MergeSettingsToRegistry(SettingsRegistryInterface& registry)
    {
        SettingsRegistryInterface::Specializations specializations;
        SetSettingsRegistrySpecializations(specializations);

        AZStd::vector<char> scratchBuffer;
#if defined(AZ_DEBUG_BUILD) || defined(AZ_PROFILE_BUILD)
        // In development builds apply the o3de registry and the command line to allow early overrides. This will
        // allow developers to override things like default paths or Asset Processor connection settings. Any additional
        // values will be replaced by later loads, so this step will happen again at the end of loading.
        SettingsRegistryMergeUtils::MergeSettingsToRegistry_O3deUserRegistry(registry, AZ_TRAIT_OS_PLATFORM_CODENAME, specializations, &scratchBuffer);
        SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(registry, m_commandLine, false);
        // Project User Registry is merged after the command line here to allow make sure the any command line override of the project path
        // is used for merging the project's user registry
        SettingsRegistryMergeUtils::MergeSettingsToRegistry_ProjectUserRegistry(registry, AZ_TRAIT_OS_PLATFORM_CODENAME, specializations, &scratchBuffer);
        SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(registry, m_commandLine, false);
        SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(registry);
#endif
        //! Retrieves the list gem targets that the project has load dependencies on
        //! This populates the /Amazon/Gems/<GemName>/SourcePaths array entries which is required
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

        SettingsRegistryMergeUtils::MergeSettingsToRegistry_TargetBuildDependencyRegistry(registry,
            AZ_TRAIT_OS_PLATFORM_CODENAME, specializations, &scratchBuffer);
        SettingsRegistryMergeUtils::MergeSettingsToRegistry_EngineRegistry(registry, AZ_TRAIT_OS_PLATFORM_CODENAME, specializations, &scratchBuffer);
        SettingsRegistryMergeUtils::MergeSettingsToRegistry_GemRegistries(registry, AZ_TRAIT_OS_PLATFORM_CODENAME, specializations, &scratchBuffer);
        SettingsRegistryMergeUtils::MergeSettingsToRegistry_ProjectRegistry(registry, AZ_TRAIT_OS_PLATFORM_CODENAME, specializations, &scratchBuffer);
#if defined(AZ_DEBUG_BUILD) || defined(AZ_PROFILE_BUILD)
        SettingsRegistryMergeUtils::MergeSettingsToRegistry_O3deUserRegistry(registry, AZ_TRAIT_OS_PLATFORM_CODENAME, specializations, &scratchBuffer);
        SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(registry, m_commandLine, false);
        SettingsRegistryMergeUtils::MergeSettingsToRegistry_ProjectUserRegistry(registry, AZ_TRAIT_OS_PLATFORM_CODENAME, specializations, &scratchBuffer);
        SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(registry, m_commandLine, true);
#endif
        // Update the Runtime file paths in case the "{BootstrapSettingsRootKey}/assets" key was overriden by a setting registry
        SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(registry);
    }

    void ComponentApplication::SetSettingsRegistrySpecializations(SettingsRegistryInterface::Specializations& specializations)
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
            AZ::OSString m_gemName;
            AZStd::vector<AZ::OSString> m_dynamicLibraryPaths;
            bool m_autoLoad{ true };
        };

        struct GemModuleVisitor
            : AZ::SettingsRegistryInterface::Visitor
        {
            GemModuleVisitor()
                : m_gemModuleKey{ AZ::SettingsRegistryInterface::FixedValueString::format("%s/Gems", AZ::SettingsRegistryMergeUtils::OrganizationRootKey) }
            {}

            AZ::SettingsRegistryInterface::VisitResponse Traverse(AZStd::string_view path, AZStd::string_view,
                AZ::SettingsRegistryInterface::VisitAction action, AZ::SettingsRegistryInterface::Type) override
            {
                if (action == AZ::SettingsRegistryInterface::VisitAction::Begin)
                {
                    // Strip off the last JSON pointer key from the path and if it matches the gem module key then add an entry
                    // to the ModulesLoadData array
                    AZStd::optional<AZStd::string_view> gemNameKey = AZ::StringFunc::TokenizeLast(path, "/");
                    if (path == m_gemModuleKey && gemNameKey && !gemNameKey->empty())
                    {
                        AZStd::string_view gemName = gemNameKey.value();
                        auto FindGemModuleLoadEntry = [gemName](const GemModuleLoadData& moduleLoadData)
                        {
                            return gemName == moduleLoadData.m_gemName;
                        };

                        if (auto foundIt = AZStd::find_if(m_modulesLoadData.begin(), m_modulesLoadData.end(), FindGemModuleLoadEntry);
                            foundIt == m_modulesLoadData.end())
                        {
                            m_modulesLoadData.emplace_back(GemModuleLoadData{ gemName });
                        }
                    }
                }

                return AZ::SettingsRegistryInterface::VisitResponse::Continue;
            }

            using SettingsRegistryInterface::Visitor::Visit;
            void Visit(AZStd::string_view path, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, bool value) override
            {
                // By default the auto load option is true
                // So auto load is turned off if option "AutoLoad" key is bool that is false
                if (valueName == "AutoLoad" && !value)
                {
                    // Strip off the AutoLoad entry from the path
                    auto autoLoadKey = AZ::StringFunc::TokenizeLast(path, "/");
                    if (!autoLoadKey)
                    {
                        return;
                    }
                    if (auto moduleLoadData = FindGemModuleEntry(path); moduleLoadData != nullptr)
                    {
                        moduleLoadData->m_autoLoad = value;
                    }
                }
            }

            void Visit(AZStd::string_view path, AZStd::string_view, AZ::SettingsRegistryInterface::Type, AZStd::string_view value) override
            {
                // Remove last path segment and check if the key corresponds to the Modules array
                AZ::StringFunc::TokenizeLast(path, "/");
                if (path.ends_with("/Modules"))
                {
                    // Remove the "Modules" path segment to be at the GemName key
                    AZ::StringFunc::TokenizeLast(path, "/");
                    if (auto moduleLoadData = FindGemModuleEntry(path); moduleLoadData != nullptr)
                    {
                        // Just use Json Serialization to load all the array elements
                        moduleLoadData->m_dynamicLibraryPaths.emplace_back(value);
                    }
                }
            }

            AZStd::vector<GemModuleLoadData> m_modulesLoadData;
            const AZ::SettingsRegistryInterface::FixedValueString m_gemModuleKey;

        private:
            // Looks upwards one key to locate the GemModuleLoadData
            GemModuleLoadData* FindGemModuleEntry(AZStd::string_view path)
            {
                // Now retrieve the GemName token
                AZStd::optional<AZStd::string_view> gemNameKey = AZ::StringFunc::TokenizeLast(path, "/");
                if (gemNameKey && !gemNameKey->empty())
                {
                    AZStd::string_view gemName = gemNameKey.value();
                    auto FindGemModuleLoadEntry = [gemName](const GemModuleLoadData& moduleLoadData)
                    {
                        return gemName == moduleLoadData.m_gemName;
                    };

                    auto foundIt = AZStd::find_if(m_modulesLoadData.begin(), m_modulesLoadData.end(), FindGemModuleLoadEntry);
                    return foundIt != m_modulesLoadData.end() ? AZStd::addressof(*foundIt) : nullptr;
                }

                return nullptr;
            }
        };

        auto gemModuleKey = AZ::SettingsRegistryInterface::FixedValueString::format("%s/Gems", AZ::SettingsRegistryMergeUtils::OrganizationRootKey);
        ModuleDescriptorList gemModules;
        {
            GemModuleVisitor moduleVisitor;
            m_settingsRegistry->Visit(moduleVisitor, gemModuleKey);
            for (GemModuleLoadData& moduleLoadData : moduleVisitor.m_modulesLoadData)
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

        // All dynamic modules have been gathered at this point
        AZ::ModuleManagerRequests::LoadModulesResult loadModuleOutcomes;
        ModuleManagerRequestBus::BroadcastResult(loadModuleOutcomes, &ModuleManagerRequests::LoadDynamicModules, gemModules, ModuleInitializationSteps::RegisterComponentDescriptors, true);

#if defined(AZ_ENABLE_TRACING)
        for (const auto& loadModuleOutcome : loadModuleOutcomes)
        {
            AZ_Error("ComponentApplication", loadModuleOutcome.IsSuccess(), "%s", loadModuleOutcome.GetError().c_str());
        }
#endif
    }

    void ComponentApplication::Tick(float deltaOverride /*= -1.f*/)
    {
        {
            AZ_PROFILE_SCOPE(System, "Component application simulation tick");

            AZStd::chrono::system_clock::time_point now = AZStd::chrono::system_clock::now();

            m_deltaTime = 0.0f;

            if (now >= m_currentTime)
            {
                AZStd::chrono::duration<float> delta = now - m_currentTime;
                m_deltaTime = deltaOverride >= 0.f ? deltaOverride : delta.count();
            }

            {
                AZ_PROFILE_SCOPE(AzCore, "ComponentApplication::Tick:ExecuteQueuedEvents");
                TickBus::ExecuteQueuedEvents();
            }
            m_currentTime = now;
            {
                AZ_PROFILE_SCOPE(AzCore, "ComponentApplication::Tick:OnTick");
                EBUS_EVENT(TickBus, OnTick, m_deltaTime, ScriptTimePoint(now));
            }
        }
    }

    void ComponentApplication::TickSystem()
    {
        AZ_PROFILE_SCOPE(System, "Component application tick");

        SystemTickBus::ExecuteQueuedEvents();
        EBUS_EVENT(SystemTickBus, OnSystemTick);
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
            EBUS_EVENT_ID_RESULT(componentDescriptor, componentId, ComponentDescriptorBus, GetDescriptor);
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

    //=========================================================================
    // CalculateExecutablePath
    //=========================================================================
    void ComponentApplication::CalculateExecutablePath()
    {
        m_exeDirectory = Utils::GetExecutableDirectory();
    }

    void ComponentApplication::CalculateAppRoot()
    {
        if (AZStd::optional<AZ::StringFunc::Path::FixedString> appRootPath = Utils::GetDefaultAppRootPath(); appRootPath)
        {
            m_appRoot = AZStd::move(*appRootPath);
        }
    }

    void ComponentApplication::CalculateEngineRoot()
    {
        m_engineRoot = AZ::SettingsRegistryMergeUtils::FindEngineRoot(*m_settingsRegistry).Native();
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

    //=========================================================================
    // GetFrameTime
    // [1/22/2016]
    //=========================================================================
    float ComponentApplication::GetTickDeltaTime()
    {
        return m_deltaTime;
    }

    //=========================================================================
    // GetTime
    // [1/22/2016]
    //=========================================================================
    ScriptTimePoint ComponentApplication::GetTimeAtCurrentTick()
    {
        return ScriptTimePoint(m_currentTime);
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
        IO::PathReflection::Reflect(context);

        // reflect the SettingsRegistryInterface, SettignsRegistryImpl and the global Settings Registry
        // instance (AZ::SettingsRegistry::Get()) into the Behavior Context
        if (auto behaviorContext{ azrtti_cast<AZ::BehaviorContext*>(context) }; behaviorContext != nullptr)
        {
            AZ::SettingsRegistryScriptUtils::ReflectSettingsRegistryToBehaviorContext(*behaviorContext);
        }
    }
} // namespace AZ
