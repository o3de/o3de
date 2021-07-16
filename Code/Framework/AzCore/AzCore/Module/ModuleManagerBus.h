/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/EBus/Event.h>
#include <AzCore/Outcome/Outcome.h>

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/osstring.h>

namespace AZ
{
    // Forward declares
    class DynamicModuleHandle;
    class Module;
    class Entity;
    class ReflectContext;

    /**
     * Public interface for accessing modules.
     */
    struct ModuleData
    {
        virtual ~ModuleData() = default;

        /// Get the handle to the actual dynamic module
        virtual DynamicModuleHandle* GetDynamicModuleHandle() const = 0;
        /// Get the handle to the module class
        virtual Module* GetModule() const = 0;
        /// Get the entity this module uses as a System Entity
        virtual Entity* GetEntity() const = 0;
        /// Get the debug name of the module
        virtual const char* GetDebugName() const = 0;
    };

    /**
     * Describes the steps taken when loading and initializing a module.
     * \note Not to be used as flags!
     */
    enum class ModuleInitializationSteps : u8
    {
        None,                           ///< The module hasn't been initialized
        Load,                           ///< Loads a dynamic module (populates GetDynamicModuleHandle())
        CreateClass,                    ///< Initializes the module class (populates GetModule())
        RegisterComponentDescriptors,   ///< Registers all of the component descriptors from a module
        ActivateEntity,                 ///< Instantiates system components and activates the entity
    };

    /**
     * Describes a dynamic module used by the application.
     */
    class DynamicModuleDescriptor
    {
    public:
        AZ_TYPE_INFO(DynamicModuleDescriptor, "{D2932FA3-9942-4FD2-A703-2E750F57C003}");
        static void Reflect(ReflectContext* context);

        OSString m_dynamicLibraryPath;          ///< Path to the module.
    };
    /**
     * Type for storing module references.
     */
    using ModuleDescriptorList = AZStd::vector<DynamicModuleDescriptor, OSStdAllocator>;

    /**
     * Function signature for static module registrars.
     */
    using CreateStaticModulesCallback = AZStd::function<void(AZStd::vector<AZ::Module*>&)>;

    //! Requests related to module reloading
    class ModuleManagerRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual ~ModuleManagerRequests() = default;

        using EnumerateModulesCallback = AZStd::function<bool(const ModuleData& moduleData)>;
        /// Calls cb on all loaded modules
        virtual void EnumerateModules(EnumerateModulesCallback perModuleCallback) = 0;

        using LoadModuleOutcome = AZ::Outcome<AZStd::shared_ptr<ModuleData>, AZStd::string>;
        /**
         * Load a module, instantiate its AZ::Module class, register it's components, and return a pointer to the data to the caller.
         * Module references are shared (shared_ptr), so subsequent Load calls for the same modules will share instances of the ModuleData.
         * \note Does NOT populate m_moduleEntity.
         *
         * \param modulePath        the path to the module to load.
         * \param lastStepToPerform the final step the procedure will complete, after completing all previous steps.
         * \param maintainReference if true, the ModuleManager will capture its own reference to the module, ensuring it isn't unloaded until application shutdown.
         *
         * \returns a pointer to the ModuleData on success, and error on failure.
         */
        virtual LoadModuleOutcome LoadDynamicModule(const char* modulePath, ModuleInitializationSteps lastStepToPerform, bool maintainReference) = 0;

        using LoadModulesResult = AZStd::vector<LoadModuleOutcome>;
        /**
         * Loads a list of dynamic modules, and initializes it up to the point specified.
         *
         * \param modules           the list of module references to load.
         * \param lastStepToPerform the final step of the procedure will complete, after completing all previous steps.
         * \param maintainReferences if true, the ModuleManager will capture its own references to the modules, ensuring they aren't unloaded until application shutdown.
         */
        virtual LoadModulesResult LoadDynamicModules(const ModuleDescriptorList& modules, ModuleInitializationSteps lastStepToPerform, bool maintainReferences) = 0;

        /**
         * Loads a static modules, and initializes it up to the point specified.
         *
         * \param staticModulesCb   the callback which creates the static modules.
         * \param lastStepToPerform the final step of the procedure will complete, after completing all previous steps.
         */
        virtual LoadModulesResult LoadStaticModules(CreateStaticModulesCallback staticModulesCb, ModuleInitializationSteps lastStepToPerform) = 0;

        /**
         * Determine if a module defined by its module path is loaded
         *
         * \param modulePath the absolute path to the module to determine if its loaded.
         *
         * \returns true if the given module is already loaded (performs module path resolving just like LoadDynamicModule does)
         */
        virtual bool IsModuleLoaded(const char* modulePath) = 0;

        using PreModuleLoadEvent = AZ::Event<AZStd::string_view>;
        PreModuleLoadEvent m_preModuleLoadEvent;
    };
    using ModuleManagerRequestBus = AZ::EBus<ModuleManagerRequests>;
} //namespace AZ
