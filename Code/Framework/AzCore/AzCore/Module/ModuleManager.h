/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Module/ModuleManagerBus.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>
#include <AzCore/std/string/string_view.h>

namespace AZ
{
    /**
     * Checks if a component should be used as a System Component in a given context.
     *
     * \param descriptor    The descriptor for the component being checked
     * \param requiredTags  The tags the component must have one of for true to be returned
     * \param serialize     The serialize context to pull attributes for the component from
     *
     * \returns whether the component represented by descriptor has at least one tag that is present in required tags
     */
    bool ShouldUseSystemComponent(const ComponentDescriptor& descriptor, const AZStd::vector<Crc32>& requiredTags, const SerializeContext& serialize);

    /**
     * ModuleEntity is an Entity that carries a module class id along with it.
     * This we do so that when the System Entity Editor saves out an entity,
     * when it's loaded from the stream, we can use that id to associate the
     * entity with the appropriate module.
     */
    class ModuleEntity
        : public Entity
    {
        friend class ModuleManager;
    public:
        AZ_CLASS_ALLOCATOR(ModuleEntity, SystemAllocator);
        AZ_RTTI(ModuleEntity, "{C5950488-35E0-4B55-B664-29A691A6482F}", Entity);

        static void Reflect(ReflectContext* context);

        ModuleEntity() = default;
        ModuleEntity(const AZ::Uuid& moduleClassId, const char* name = nullptr)
            : Entity(name)
            , m_moduleClassId(moduleClassId)
        { }

        // The typeof of the module class associated with this module,
        // so it can be associated with the correct module after loading
        AZ::Uuid m_moduleClassId;

    protected:
        // Allow manual setting of state
        void SetState(Entity::State state);
    };

    /**
     * Contains a static or dynamic AZ::Module.
     */
    struct ModuleDataImpl
        : public ModuleData
    {
        AZ_CLASS_ALLOCATOR(ModuleDataImpl, SystemAllocator);

        ModuleDataImpl() = default;
        ~ModuleDataImpl() override;

        // no copy/move allowed
        ModuleDataImpl(const ModuleDataImpl&) = delete;
        ModuleDataImpl& operator=(const ModuleDataImpl&) = delete;
        ModuleDataImpl(ModuleDataImpl&& rhs) = delete;
        ModuleDataImpl& operator=(ModuleDataImpl&&) = delete;

        ////////////////////////////////////////////////////////////////////////
        // IModuleData
        DynamicModuleHandle* GetDynamicModuleHandle() const override { return m_dynamicHandle.get(); }
        Module* GetModule() const override { return m_module; }
        Entity* GetEntity() const override { return m_moduleEntity.get(); }
        const char* GetDebugName() const override;
        ////////////////////////////////////////////////////////////////////////

        /// Deals with loading and unloading the AZ::Module's DLL.
        /// This is null when the AZ::Module comes from a static LIB.
        AZStd::unique_ptr<DynamicModuleHandle> m_dynamicHandle;

        /// Handle to the module class within the module
        Module* m_module = nullptr;

        //! Entity that holds this module's provided system components
        AZStd::unique_ptr<ModuleEntity> m_moduleEntity;

        //! The last step this module completed
        ModuleInitializationSteps m_lastCompletedStep = ModuleInitializationSteps::None;
    };

    /*!
     * Handles reloading modules and their dependents at runtime
     */
    class ModuleManager
        : public ModuleManagerRequestBus::Handler
        , protected EntityBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ModuleManager, AZ::OSAllocator);
        static void Reflect(ReflectContext* context);

        ModuleManager();
        ~ModuleManager() override;

        // Destroy and unload all modules
        void UnloadModules();

        // To be called by the Component Application when it deserializes an entity
        void AddModuleEntity(ModuleEntity* moduleEntity);

        // Deactivates owned module entities without unloading the module
        void DeactivateEntities();

        // To be called by the Component Application on startup
        void SetSystemComponentTags(AZStd::string_view tags);

        // Get the split list of system component tags specified at startup
        const AZStd::vector<Crc32>& GetSystemComponentTags() { return m_systemComponentTags; }

        // Whether the user wants to quit the Application on errors rather than proceeding in a likely bad state
        bool m_quitRequested = false;

    protected:
        ////////////////////////////////////////////////////////////////////////
        // ModuleManagerRequestBus
        void EnumerateModules(EnumerateModulesCallback perModuleCallback) override;
        LoadModuleOutcome LoadDynamicModule(const char* modulePath, ModuleInitializationSteps lastStepToPerform, bool maintainReference) override;
        LoadModulesResult LoadDynamicModules(const ModuleDescriptorList& modules, ModuleInitializationSteps lastStepToPerform, bool maintainReferences) override;
        LoadModulesResult LoadStaticModules(CreateStaticModulesCallback staticModulesCb, ModuleInitializationSteps lastStepToPerform) override;
        bool IsModuleLoaded(const char* modulePath) override;

        //! Lookup module by name using the supplied modulePath parameter and return a ModuleData reference to it
        //! @param modulePath basename to a module to lookup within the executable directory
        //! if the modulePath has any path separators within it, the basename after the last separator is used
        //! @return shared ptr to an ModuleData structure if the module is loaded and managed by the ModuleManager
        AZStd::shared_ptr<ModuleDataImpl> GetLoadedModule(AZStd::string_view modulePath);


        //! On dependency sort errors, display error message with details.
        //! Additionally send the message to NativeUI (if available) and ask user what to do,
        void HandleDependencySortError(const Entity::DependencySortOutcome& outcome);

        ////////////////////////////////////////////////////////////////////////
        // EntityBus
        void OnEntityActivated(const AZ::EntityId& entityId) override;
        ////////////////////////////////////////////////////////////////////////

        // Helper function for initializing module entities
        void ActivateEntities(const AZStd::vector<AZStd::shared_ptr<ModuleDataImpl>>& modulesToInit);

        // Helper function to preprocess the module names to handle any special processing
        static AZ::OSString PreProcessModule(AZStd::string_view moduleName);

        // Tags to look for when activating system components
        AZStd::vector<Crc32> m_systemComponentTags;

        /// System components we own and are responsible for shutting down
        AZ::Entity::ComponentArrayType m_systemComponents;

        /// The modules we own
        AZStd::vector<AZStd::shared_ptr<ModuleDataImpl>> m_ownedModules;

        using ModuleNameNameToModuleDataMap = AZStd::unordered_map<AZ::OSString, AZStd::weak_ptr<ModuleDataImpl>>;
        //! Map from modules names to loaded ModuleData
        ModuleNameNameToModuleDataMap m_nameToModuleMap;
    };
} // namespace AZ
