/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/Debug/BudgetTracker.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Module/ModuleManager.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/ReflectionManager.h>
#include <AzCore/Settings/CommandLine.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryConsoleUtils.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/osstring.h>


namespace AZ
{
    class BehaviorContext;
    class IConsole;
    class Module;
    class ModuleManager;
    class TimeSystem;
}
namespace AZ::Debug
{
    class DrillerManager;
    class LocalFileEventLogger;
}

namespace AZ
{
    class ReflectionEnvironment
    {
    public:

        ReflectionEnvironment()
        {
            m_reflectionManager = AZStd::make_unique<ReflectionManager>();
        }

        static void Init();
        static void Reset();
        static ReflectionManager* GetReflectionManager();

        ReflectionManager* Get() { return m_reflectionManager.get(); }

    private:

        AZStd::unique_ptr<ReflectionManager> m_reflectionManager;

    };

    /**
     * A main class that can be used directly or as a base to start a
     * component based application. It will provide all the proper bootstrap
     * and entity bookkeeping functionality.
     *
     * IMPORTANT: If you use this as a base class you must follow one rule. You can't add
     * data that will allocate memory on construction. This is because the memory managers
     * are NOT yet ready. They will be initialized during the Create call.
     */
    class ComponentApplication
        : public ComponentApplicationBus::Handler
        , public TickRequestBus::Handler
    {
        // or try to use unordered set if we store the ID internally
        typedef AZStd::unordered_map<EntityId, Entity*>  EntitySetType;

    public:
        AZ_RTTI(ComponentApplication, "{1F3B070F-89F7-4C3D-B5A3-8832D5BC81D7}");
        AZ_CLASS_ALLOCATOR(ComponentApplication, SystemAllocator, 0);

        /**
         * Configures the component application.
         * \note This structure may be loaded from a file on disk. Values that
         *       must be set by a running application should go into StartupParameters.
         * \note It's important this structure not contain members that allocate from the system allocator.
         *       Use the OSAllocator only.
         */
        struct Descriptor
            : public SerializeContext::IObjectFactory
        {
            AZ_TYPE_INFO(ComponentApplication::Descriptor, "{70277A3E-2AF5-4309-9BBF-6161AFBDE792}");
            AZ_CLASS_ALLOCATOR(ComponentApplication::Descriptor, SystemAllocator, 0);

            struct AllocatorRemapping
            {
                AZ_TYPE_INFO(ComponentApplication::Descriptor::AllocatorRemapping, "{4C865590-4506-4B76-BF14-6CCB1B83019A}");
                AZ_CLASS_ALLOCATOR(ComponentApplication::Descriptor::AllocatorRemapping, OSAllocator, 0);

                static void Reflect(ReflectContext* context, ComponentApplication* app);

                OSString m_from;
                OSString m_to;
            };

            typedef AZStd::vector<AllocatorRemapping, OSStdAllocator> AllocatorRemappings;

            ///////////////////////////////////////////////
            // SerializeContext::IObjectFactory
            void* Create(const char* name) override;
            void  Destroy(void* data) override;
            ///////////////////////////////////////////////

            /// Reflect the descriptor data.
            static void     Reflect(ReflectContext* context, ComponentApplication* app);

            Descriptor();

            bool            m_useExistingAllocator;     //!< True if the user is creating the system allocation and setup tracking modes, if this is true all other parameters are IGNORED. (default: false)
            bool            m_grabAllMemory;            //!< True if we want to grab all available memory minus reserved fields. (default: false)
            bool            m_allocationRecords;        //!< True if we want to track memory allocations, otherwise false. (default: true)
            bool            m_allocationRecordsSaveNames; //!< True if we want to allocate space for saving the name/filename of each allocation so unloaded module memory leaks have valid names to read, otherwise false. (default: false, automatically true with recording mode FULL)
            bool            m_allocationRecordsAttemptDecodeImmediately; ///< True if we want to attempt decoding frames at time of allocation, otherwise false. Very expensive, used specifically for debugging allocations that fail to decode. (default: false)
            bool            m_autoIntegrityCheck;       //!< True to check the heap integrity on each allocation/deallocation. (default: false)
            bool            m_markUnallocatedMemory;    //!< True to mark all memory with 0xcd when it's freed. (default: true)
            bool            m_doNotUsePools;            //!< True of we want to pipe all allocation to a generic allocator (not pools), this can help debugging a memory stomp. (default: false)
            bool            m_enableScriptReflection;   //!< True if we want to enable reflection to the script context.

            unsigned int    m_pageSize;                 //!< Page allocation size must be 1024 bytes aligned. (default: SystemAllocator::Descriptor::Heap::m_defaultPageSize)
            unsigned int    m_poolPageSize;             //!< Page size used to small memory allocations. Must be less or equal to m_pageSize and a multiple of it. (default: SystemAllocator::Descriptor::Heap::m_defaultPoolPageSize)
            unsigned int    m_memoryBlockAlignment;     //!< Alignment of memory block. (default: SystemAllocator::Descriptor::Heap::m_memoryBlockAlignment)
            AZ::u64         m_memoryBlocksByteSize;     //!< Memory block size in bytes if. This parameter is ignored if m_grabAllMemory is set to true. (default: 0 - use memory on demand, no preallocation)
            AZ::u64         m_reservedOS;               //!< Reserved memory for the OS in bytes. Used only when m_grabAllMemory is set to true. (default: 0)
            AZ::u64         m_reservedDebug;            //!< Reserved memory for Debugging (allocation,etc.). Used only when m_grabAllMemory is set to true. (default: 0)
            Debug::AllocationRecords::Mode m_recordingMode; //!< When to record stack traces (default: AZ::Debug::AllocationRecords::RECORD_STACK_IF_NO_FILE_LINE)
            AZ::u64         m_stackRecordLevels;        //!< If stack recording is enabled, how many stack levels to record. (default: 5)
            bool            m_enableDrilling;           //!< True to enabled drilling support for the application. RegisterDrillers will be called. Ignored in release. (default: true)
            bool            m_useOverrunDetection;      //!< True to use the overrun detection memory management scheme. Only available on some platforms; greatly increases memory consumption.
            bool            m_useMalloc;                //!< True to use malloc instead of the internal memory manager. Intended for debugging purposes only.

            AllocatorRemappings m_allocatorRemappings;  //!< List of remappings of allocators to perform, so that they can alias each other.

            ModuleDescriptorList m_modules;             //!< Dynamic modules used by the application.
                                                        //!< These will be loaded on startup.
        };

        //! Application settings.
        //! Unlike the Descriptor, these values must be set in code and cannot be loaded from a file.
        struct StartupParameters
        {
            StartupParameters() {}

            //! If set, this allocator is used to allocate the temporary bootstrap memory, as well as the main \ref SystemAllocator heap.
            //! If it's left nullptr (default), the \ref OSAllocator will be used.
            IAllocatorAllocate* m_allocator = nullptr;

            //! Callback to create AZ::Modules for the static libraries linked by this application.
            //! Leave null if the application uses no static AZ::Modules.
            //! \note Dynamic AZ::Modules are specified in the ComponentApplication::Descriptor.
            CreateStaticModulesCallback m_createStaticModulesCallback = nullptr;

            //! Specifies which system components to create & activate. If no tags specified, all system components are used. Specify as comma separated list.
            const char* m_systemComponentTags = nullptr;

            //! Whether or not to load static modules associated with the application
            bool m_loadStaticModules = true;
            //! Whether or not to load dynamic modules described by \ref Descriptor::m_modules
            bool m_loadDynamicModules = true;
            //! Used by test fixtures to ensure reflection occurs to edit context.
            bool m_createEditContext = false;
            //! Indicates whether the AssetCatalog.xml should be loaded by default in Application::StartCommon
            bool m_loadAssetCatalog = true;
        };

        ComponentApplication();
        ComponentApplication(int argC, char** argV);
        virtual ~ComponentApplication();

        /**
         * Create function which accepts a variant which allows passing in either a ComponentApplication::Descriptor or a c-string path
         * to an object stream descriptor file.
         * The object stream descriptor path is deprecated and will removed when the gems are loaded from the settings registry
         * If descriptor type = const char*: Loads the application configuration and systemEntity from 'applicationDescriptorFile' (path relative to AppRoot).
         * It is expected that the first node in the file will be the descriptor, for memory manager creation.
         * If descriptor type = Descriptor: Create system allocator and system entity. No components are added to the system node.
         * You will need to setup all system components manually.
         * \returns pointer to the system entity.
         */
        virtual Entity* Create(const Descriptor& descriptor, const StartupParameters& startupParameters = StartupParameters());
        virtual void Destroy();
        virtual void DestroyAllocator(); // Called at the end of Destroy(). Applications can override to do tear down work right before allocator is destroyed.

        //////////////////////////////////////////////////////////////////////////
        // ComponentApplicationRequests
        void RegisterComponentDescriptor(const ComponentDescriptor* descriptor) final;
        void UnregisterComponentDescriptor(const ComponentDescriptor* descriptor) final;
        void RegisterEntityAddedEventHandler(EntityAddedEvent::Handler& handler) final;
        void RegisterEntityRemovedEventHandler(EntityRemovedEvent::Handler& handler) final;
        void RegisterEntityActivatedEventHandler(EntityActivatedEvent::Handler& handler) final;
        void RegisterEntityDeactivatedEventHandler(EntityDeactivatedEvent::Handler& handler) final;
        void SignalEntityActivated(Entity* entity) final;
        void SignalEntityDeactivated(Entity* entity) final;
        bool AddEntity(Entity* entity) override;
        bool RemoveEntity(Entity* entity) override;
        bool DeleteEntity(const EntityId& id) override;
        Entity* FindEntity(const EntityId& id) override;
        AZStd::string GetEntityName(const EntityId& id) override;
        bool SetEntityName(const EntityId& id, const AZStd::string_view name) override;
        void EnumerateEntities(const ComponentApplicationRequests::EntityCallback& callback) override;
        ComponentApplication* GetApplication() override { return this; }
        /// Returns the serialize context that has been registered with the app, if there is one.
        SerializeContext* GetSerializeContext() override;
        /// Returns the behavior context that has been registered with the app, if there is one.
        BehaviorContext* GetBehaviorContext() override;
        /// Returns the json registration context that has been registered with the app, if there is one.
        JsonRegistrationContext* GetJsonRegistrationContext() override;
        /// Returns the path to the engine.
        const char* GetEngineRoot() const override;
        /// Returns the path to the folder the executable is in.
        const char* GetExecutableFolder() const override;

        //////////////////////////////////////////////////////////////////////////
        /// TickRequestBus
        float GetTickDeltaTime() override;
        ScriptTimePoint GetTimeAtCurrentTick() override;
        //////////////////////////////////////////////////////////////////////////

        Descriptor& GetDescriptor() { return m_descriptor; }

        /**
         * Ticks all components using the \ref AZ::TickBus during simulation time. May not tick if the application is not active (i.e. not in focus)
         */
        virtual void Tick();

        /**
        * Ticks all using the \ref AZ::SystemTickBus at all times. Should always tick even if the application is not active.
        */
        virtual void TickSystem();

        /**
         * Application-overridable way to state required system components.
         * These components will be added to the system entity if they were
         * not already provided by the application descriptor.
         * \return the type-ids of required components.
         */
        virtual ComponentTypeList GetRequiredSystemComponents() const { return {}; }

        /**
         * ResolveModulePath is called whenever LoadDynamicModule wants to resolve a module in order to actually load it.
         * You can override this if you need to load modules from a different path or hijack module loading in some other way.
         * If you do, ensure that you use platform-specific conventions to do so, as this is called by multiple platforms.
         * The default implantation prepends the path to the executable to the module path, but you can override this behavior
         * (Call the base class if you want this behavior to persist in overrides)
         */
        void ResolveModulePath(AZ::OSString& modulePath) override;

        void QueryApplicationType(AZ::ApplicationTypeQuery& appType) const override;

        /**
         * Returns Parsed CommandLine structure which supports query command line options and positional parameters
        */
        AZ::CommandLine* GetAzCommandLine() override;

        /**
        * Retrieve the argc passed into the application class on startup, if any was passed in.
        * Note that this could return nullptr if the application was not initialized with any such parameter.
        * This is important to have because different operating systems have different level of access to the command line args
        * and on some operating systems (MacOS) its fairly difficult to reliably retrieve them without resorting to NS libraries
        * and making some assumptions. Instead, we allow you to pass your args in from the main(...) function.
        * Another thing to notice here is that these are non-const pointers to the argc and argv values
        * instead of int, char**, these are int*, char***.
        * This is because some application layers (such as Qt) actually require that the ArgC and ArgV are modifiable,
        * as they actually patch them to add/remove command line parameters during initialization.
        * but also to highlight the fact that they are pointers to static memory that must remain relevant throughout the existence
        * of the Application object.
        * For best results, simply pass in &argc and &argv from your void main(argc, argv) in here - that memory is
        * permanently tied to your process and is going to be available at all times during run.
        */
        int* GetArgC();

        /**
        * Retrieve the argv parameter passed into the application class on startup.  see the note on ArgC
        * Note that this could return nullptr if the application was not initialized with any such parameter.
        */
        char*** GetArgV();

        //! Perform loading of modules by appending the modules in the Descriptor
        //! to the list of modules in cmake_dependencies.*.setreg file for the active project
        void LoadModules();

        //! Loads only static modules which are populated via the CreateStaticModules member function
        void LoadStaticModules();

        //! Performs loading of dynamic modules made up of the list of modules in the cmake_dependencies.*.setreg
        //! loaded into the AZ::SettingsRegistry plus the list of modules stored in the Descriptor::m_modules array
        void LoadDynamicModules();

    protected:
        virtual void CreateReflectionManager();
        void DestroyReflectionManager();

        /// Perform any additional initialization needed before loading modules
        virtual void PreModuleLoad() {};

        virtual void CreateStaticModules(AZStd::vector<AZ::Module*>& outModules);

        /// Common logic shared between the multiple Create(...) functions.
        void        CreateCommon();

        /// Create the operating system allocator if not supplied in the StartupParameters
        void        CreateOSAllocator();

        /// Create the system allocator using the data in the m_descriptor
        void        CreateSystemAllocator();

        virtual void MergeSettingsToRegistry(SettingsRegistryInterface& registry);

        //! Sets the specializations that will be used when loading the Settings Registry. Extend this in derived
        //! application classes to specialize settings for those applications.
        virtual void SetSettingsRegistrySpecializations(SettingsRegistryInterface::Specializations& specializations);

        /**
         * This is the function that will be called instantly after the memory
         * manager is created. This is where we should register all core component
         * factories that will participate in the loading of the bootstrap file
         * or all factories in general.
         * When you create your own application this is where you should FIRST call
         * ComponentApplication::RegisterCoreComponents and then register the application
         * specific core components.
         */
        virtual void RegisterCoreComponents() {};

        /*
         * Reflect classes from this framework to the appropriate context.
         * Subclasses of AZ::Component should not be listed here, they are reflected through the ComponentDescriptorBus.
         */
        virtual void Reflect(ReflectContext* context);

        /// Check if a System Component should be created
        bool ShouldAddSystemComponent(AZ::ComponentDescriptor* descriptor);

        /// Adds system components requested by modules and the application to the system entity.
        void AddRequiredSystemComponents(AZ::Entity* systemEntity);

        template<typename Iterator>
        static void NormalizePath(Iterator begin, Iterator end, bool doLowercase = true)
        {
            AZStd::replace(begin, end, '\\', '/');
            if (doLowercase)
            {
                AZStd::to_lower(begin, end);
            }
        }

        AZStd::unique_ptr<ModuleManager>            m_moduleManager;
        AZStd::unique_ptr<SettingsRegistryInterface> m_settingsRegistry;
        EntityAddedEvent                            m_entityAddedEvent;
        EntityRemovedEvent                          m_entityRemovedEvent;
        EntityAddedEvent                            m_entityActivatedEvent;
        EntityRemovedEvent                          m_entityDeactivatedEvent;
        AZ::IConsole*                               m_console{};
        Descriptor                                  m_descriptor;
        bool                                        m_isStarted{ false };
        bool                                        m_isSystemAllocatorOwner{ false };
        bool                                        m_isOSAllocatorOwner{ false };
        bool                                        m_ownsConsole{};
        void*                                       m_fixedMemoryBlock{ nullptr }; //!< Pointer to the memory block allocator, so we can free it OnDestroy.
        IAllocatorAllocate*                         m_osAllocator{ nullptr };
        EntitySetType                               m_entities;

        AZ::SettingsRegistryInterface::NotifyEventHandler m_projectPathChangedHandler;
        AZ::SettingsRegistryInterface::NotifyEventHandler m_projectNameChangedHandler;
        AZ::SettingsRegistryInterface::NotifyEventHandler m_commandLineUpdatedHandler;

        AZStd::unique_ptr<AZ::TimeSystem> m_timeSystem;

        // ConsoleFunctorHandle is responsible for unregistering the Settings Registry Console
        // from the m_console member when it goes out of scope
        AZ::SettingsRegistryConsoleUtils::ConsoleFunctorHandle m_settingsRegistryConsoleFunctors;

#if !defined(_RELEASE)
        Debug::BudgetTracker m_budgetTracker;
#endif

        // this is used when no argV/ArgC is supplied.
        // in order to have the same memory semantics (writable, non-const)
        // we create a buffer that can be written to (up to AZ_MAX_PATH_LEN) and then
        // pack it with a single param.
        char                                        m_commandLineBuffer[AZ_MAX_PATH_LEN];
        char*                                       m_commandLineBufferAddress{ m_commandLineBuffer };

        StartupParameters                           m_startupParameters;

        char**                                      m_argV{ nullptr };
        int                                         m_argC{ 0 };
        AZ::CommandLine                             m_commandLine; // < Stores parsed command line supplied to the constructor

        AZStd::unique_ptr<AZ::Entity>               m_systemEntity; ///< Track the system entity to ensure we free it on shutdown.

        // Created early to allow events to be logged before anything else. These will be kept in memory until
        // a file is associated with the logger. The internal buffer is limited to 64kb and once full unexpected
        // behavior may happen. The LocalFileEventLogger will register itself automatically with AZ::Interface<IEventLogger>.

        struct EventLoggerDeleter
        {
            EventLoggerDeleter() noexcept;
            EventLoggerDeleter(bool skipDelete) noexcept;
            void operator()(AZ::Debug::LocalFileEventLogger* ptr);
            bool m_skipDelete{};
        };
        using EventLoggerPtr = AZStd::unique_ptr<AZ::Debug::LocalFileEventLogger, EventLoggerDeleter>;
        EventLoggerPtr m_eventLogger;
    };
}
