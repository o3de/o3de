/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Module/ModuleManager.h>
#include <AzCore/Module/Internal/ModuleManagerSearchPathTool.h>

#include <AzCore/Module/Module.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/ComponentApplicationLifecycle.h>
#include <AzCore/NativeUI/NativeUIRequests.h>

#include <AzCore/std/algorithm.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace
{
    static const char* s_moduleLoggingScope = "Module Manager";
}

namespace AZ
{
    bool ShouldUseSystemComponent(const ComponentDescriptor& descriptor, const AZStd::vector<Crc32>& requiredTags, const SerializeContext& serialize)
    {
        const SerializeContext::ClassData* classData = serialize.FindClassData(descriptor.GetUuid());
        AZ_Warning(s_moduleLoggingScope, classData, "Component type %s not reflected to SerializeContext!", descriptor.GetName());
        return Edit::SystemComponentTagsMatchesAtLeastOneTag(classData, requiredTags, false);
    }

    //=========================================================================
    // Reflect
    //=========================================================================
    void DynamicModuleDescriptor::Reflect(ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            // Bail early to avoid double reflection
            bool isReflected = serializeContext->FindClassData(azrtti_typeid<DynamicModuleDescriptor>()) != nullptr;
            if (isReflected != context->IsRemovingReflection())
            {
                return;
            }

            serializeContext->Class<DynamicModuleDescriptor>()
                ->Field("dynamicLibraryPath", &DynamicModuleDescriptor::m_dynamicLibraryPath)
                ;

            if (EditContext* ec = serializeContext->GetEditContext())
            {
                ec->Class<DynamicModuleDescriptor>(
                    "Dynamic Module descriptor", "Describes a dynamic module (DLL) used by the application")
                    ->DataElement(Edit::UIHandlers::Default, &DynamicModuleDescriptor::m_dynamicLibraryPath, "Dynamic library path", "Path to DLL.")
                    ;
            }
        }
    }

    //=========================================================================
    // ModuleEntity
    //=========================================================================
    void ModuleEntity::Reflect(ReflectContext* context)
    {
        if (SerializeContext* serialize = azrtti_cast<SerializeContext*>(context))
        {
            serialize->Class<ModuleEntity, Entity>()
                ->Field("moduleClassId", &ModuleEntity::m_moduleClassId)
                ;
        }
    }

    //=========================================================================
    // SetState
    //=========================================================================
    void ModuleEntity::SetState(Entity::State state)
    {
        if (state == Entity::State::Deactivating)
        {
            EntityBus::Event(m_id, &EntityBus::Events::OnEntityDeactivated, m_id);
            EntitySystemBus::Broadcast(&EntitySystemBus::Events::OnEntityDeactivated, m_id);
        }

        m_state = state;

        if (state == Entity::State::Active)
        {
            EntityBus::Event(m_id, &EntityBus::Events::OnEntityActivated, m_id);
            EntitySystemBus::Broadcast(&EntitySystemBus::Events::OnEntityActivated, m_id);
        }
    }

    //=========================================================================
    // GetDebugName
    //=========================================================================
    const char* ModuleDataImpl::GetDebugName() const
    {
        // If module is from DLL, return DLL name.
        if (m_dynamicHandle)
        {
            AZStd::string_view handleFilename(m_dynamicHandle->GetFilename());
            AZStd::string_view::size_type lastSlash = handleFilename.find_last_of("\\/");
            if (lastSlash != AZStd::string_view::npos)
            {
                handleFilename.remove_prefix(lastSlash + 1);
            }
            return handleFilename.data();
        }
        // If Module has its own RTTI info, return that name
        else if (m_module && !azrtti_istypeof<Module>(m_module))
        {
            return m_module->RTTI_GetTypeName();
        }
        else
        {
            return "module";
        }
    }

    //=========================================================================
    // ~ModuleDataImpl
    //=========================================================================
    ModuleDataImpl::~ModuleDataImpl()
    {
        m_moduleEntity.reset();

        // If the AZ::Module came from a DLL, destroy it
        // using the DLL's \ref DestroyModuleClassFunction.
        if (m_module && m_dynamicHandle)
        {
            auto destroyFunc = m_dynamicHandle->GetFunction<DestroyModuleClassFunction>(DestroyModuleClassFunctionName);
            AZ_Assert(destroyFunc, "Unable to locate '%s' entry point in module at \"%s\".",
                DestroyModuleClassFunctionName, m_dynamicHandle->GetFilename());
            if (destroyFunc)
            {
                destroyFunc(m_module);
                m_module = nullptr;
            }
        }

        // If the AZ::Module came from a static LIB, delete it directly.
        if (m_module)
        {
            delete m_module;
            m_module = nullptr;
        }

        // If dynamic, unload DLL and destroy handle.
        if (m_dynamicHandle)
        {
            m_dynamicHandle->Unload();
            m_dynamicHandle.reset();
        }
    }

    static_assert(!AZStd::is_trivially_copyable_v<ModuleDataImpl>, "Compiler believes ModuleData is trivially copyable, despite having non-trivially copyable members.\n");

    //=========================================================================
    // Reflect
    //=========================================================================
    void ModuleManager::Reflect(ReflectContext* context)
    {
        DynamicModuleDescriptor::Reflect(context);
        ModuleEntity::Reflect(context);
    }

    //=========================================================================
    // ModuleManager
    //=========================================================================
    ModuleManager::ModuleManager()
    {
        ModuleManagerRequestBus::Handler::BusConnect();

        // Once the system entity activates, we can activate our entities
        EntityBus::Handler::BusConnect(SystemEntityId);
    }

    //=========================================================================
    // ~ModuleManager
    //=========================================================================
    ModuleManager::~ModuleManager()
    {
        ModuleManagerRequestBus::Handler::BusDisconnect();

        UnloadModules();

#if defined(AZ_ENABLE_TRACING)
        AZStd::string moduleHandlesOpen;
        for (const auto& weakModulePair : m_nameToModuleMap)
        {
            AZ_Assert(!weakModulePair.second.expired(), "Internal error: Module was unloaded, but not removed from weak modules list!");
            auto modulePtr = weakModulePair.second.lock();
            moduleHandlesOpen += modulePtr->GetDebugName();
            moduleHandlesOpen += "\n";
        }

        if (!m_nameToModuleMap.empty())
        {
            AZ_TracePrintf(s_moduleLoggingScope, "ModuleManager being destroyed, but non-owned module handles are still open:\n%s", moduleHandlesOpen.c_str());
        }
#endif // AZ_ENABLE_TRACING
    }

    void ModuleManager::DeactivateEntities()
    {
        // For all modules that we created an entity for, set them to "Deactivating"
        for (auto& moduleData : m_ownedModules)
        {
            if (moduleData->m_moduleEntity && moduleData->m_lastCompletedStep == ModuleInitializationSteps::ActivateEntity)
            {
                moduleData->m_moduleEntity->SetState(Entity::State::Deactivating);
            }
        }

        AZStd::string componentNamesArray = R"({ "SystemComponents":[)";
        const char* comma = "";
        // For all system components, deactivate
        for (auto componentIt = m_systemComponents.rbegin(); componentIt != m_systemComponents.rend(); ++componentIt)
        {
            ModuleEntity::DeactivateComponent(**componentIt);
            componentNamesArray += AZStd::string::format(R"(%s"%s")", comma, (*componentIt)->RTTI_GetTypeName());
            comma = ", ";
        }
        componentNamesArray += R"(]})";

        // For all modules that we created an entity for, set them to "Init" (meaning not Activated)
        for (auto& moduleData : m_ownedModules)
        {
            if (moduleData->m_moduleEntity && moduleData->m_lastCompletedStep == ModuleInitializationSteps::ActivateEntity)
            {
                moduleData->m_moduleEntity->SetState(Entity::State::Init);
                moduleData->m_lastCompletedStep = ModuleInitializationSteps::RegisterComponentDescriptors;
            }
        }

        // Since the system components have been deactivated clear out the vector.
        m_systemComponents.clear();

        // Signal that the System Components have deactivated
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            AZ::ComponentApplicationLifecycle::SignalEvent(*settingsRegistry, "SystemComponentsDeactivated", componentNamesArray);
        }

    }

    //=========================================================================
    // UnloadModules
    //=========================================================================
    void ModuleManager::UnloadModules()
    {
        DeactivateEntities();
        // Because everything is unique_ptr, we don't need to explicitly delete anything
        // Shutdown in reverse order of initialization, just in case the order matters.
        while (!m_ownedModules.empty())
        {
            m_ownedModules.pop_back(); // ~ModuleData() handles shutdown logic
        }
        m_ownedModules.set_capacity(0);

        // Clear the weak modules list
        {
            m_nameToModuleMap = {};
        };
    }

    //=========================================================================
    // AddModuleEntity
    //=========================================================================
    void ModuleManager::AddModuleEntity(ModuleEntity* moduleEntity)
    {
        for (auto& moduleData : m_ownedModules)
        {
            if (moduleEntity->m_moduleClassId == azrtti_typeid(moduleData->m_module))
            {
                AZ_Assert(!moduleData->m_moduleEntity, "Adding module entity twice!");
                moduleData->m_moduleEntity.reset(moduleEntity);
                return;
            }
        }
    }

    //=========================================================================
    // SetSystemComponentTags
    //=========================================================================
    void ModuleManager::SetSystemComponentTags(AZStd::string_view tags)
    {
        // Split the tag list
        AZStd::vector<AZStd::string_view> tagList;
        auto TokenizeTags = [&tagList](AZStd::string_view token)
        {
            tagList.push_back(token);
        };
        AZ::StringFunc::TokenizeVisitor(tags, TokenizeTags, ',');

        m_systemComponentTags.resize(tagList.size());
        AZStd::transform(tagList.begin(), tagList.end(), m_systemComponentTags.begin(), [](const AZStd::string_view& tag)
        {
            return AZ::Crc32(tag.data(), tag.length(), true);
        });
    }

    //=========================================================================
    // EnumerateModules
    //=========================================================================
    void ModuleManager::EnumerateModules(EnumerateModulesCallback perModuleCallback)
    {
        for (auto& moduleData : m_ownedModules)
        {
            if (!perModuleCallback(*moduleData))
            {
                return;
            }
        }
    }

    //=========================================================================
    // PreProcessModule
    // Preprocess a dynamic module name.
    //=========================================================================
    AZ::OSString ModuleManager::PreProcessModule(AZStd::string_view moduleName)
    {
        // Let the application process the path
        AZ::OSString finalPath{ moduleName };
        ComponentApplicationBus::Broadcast(&ComponentApplicationBus::Events::ResolveModulePath, finalPath);

        return finalPath;
    }

    //=========================================================================
    // LoadDynamicModule
    //=========================================================================
    ModuleManager::LoadModuleOutcome ModuleManager::LoadDynamicModule(const char* modulePath, ModuleInitializationSteps lastStepToPerform, bool maintainReference)
    {
        AZ::OSString preprocessedModulePath = PreProcessModule(modulePath);
        // Check if the module is already loaded
        AZStd::shared_ptr<ModuleDataImpl> moduleDataPtr = GetLoadedModule(modulePath);

        if (!moduleDataPtr)
        {
            // Create shared pointer, with a deleter that will remove the module reference from m_nameToModuleMap
            moduleDataPtr = AZStd::shared_ptr<ModuleDataImpl>(
                aznew ModuleDataImpl(),
                [preprocessedModulePath, this](ModuleDataImpl* toDelete)
                {
                    this->m_nameToModuleMap.erase(preprocessedModulePath);
                    delete toDelete;
                }
            );

            // Create weak pointer to the module data
            m_nameToModuleMap.emplace(preprocessedModulePath, moduleDataPtr);
        }

        // Cache an iterator to the module in the owned modules list. This will be used later to either:
        // 1. Remove the module if the load fails
        // 2. If the module was not found and it should be owned, add it.
        auto ownedModuleIt = AZStd::find(m_ownedModules.begin(), m_ownedModules.end(), moduleDataPtr);

        // If we need to hold a reference and we don't already have one, save it
        if (maintainReference && ownedModuleIt == m_ownedModules.end())
        {
            m_ownedModules.emplace_back(moduleDataPtr);
            ownedModuleIt = AZStd::prev(m_ownedModules.end());
        }

        // If the module is already "loaded enough," just return it
        if (moduleDataPtr->m_lastCompletedStep >= lastStepToPerform)
        {
            return AZ::Success(AZStd::static_pointer_cast<ModuleData>(moduleDataPtr));
        }

        using PhaseOutcome = AZ::Outcome<void, AZStd::string>;
        // This will serve as the list of steps to perform when loading a module
        AZStd::vector<AZStd::pair<ModuleInitializationSteps, AZStd::function<PhaseOutcome()>>> phases = {
            // N/A
            {
                ModuleInitializationSteps::None,
                []() -> PhaseOutcome
                {
                    return AZ::Success();
                }
            },

            // Load the dynamic module
            {
                ModuleInitializationSteps::Load,
                [&moduleDataPtr, &preprocessedModulePath, this]() -> PhaseOutcome
                {
                    m_preModuleLoadEvent.Signal(preprocessedModulePath);
                    // Create handle
                    moduleDataPtr->m_dynamicHandle = DynamicModuleHandle::Create(preprocessedModulePath.c_str());
                    if (!moduleDataPtr->m_dynamicHandle)
                    {
                        return AZ::Failure(AZStd::string::format("Failed to create AZ::DynamicModuleHandle at path \"%s\".", preprocessedModulePath.c_str()));
                    }

                    // Load DLL from disk
                    if (!moduleDataPtr->m_dynamicHandle->Load(AZ::DynamicModuleHandle::LoadFlags::InitFuncRequired))
                    {
                        return AZ::Failure(AZStd::string::format("Failed to load dynamic library at path \"%s\".",
                            moduleDataPtr->m_dynamicHandle->GetFilename()));
                    }

                    return AZ::Success();
                }
            },

            // Create the module class
            {
                ModuleInitializationSteps::CreateClass,
                [&moduleDataPtr]() -> PhaseOutcome
                {
                    const char* moduleName = moduleDataPtr->GetDebugName();
                    // Find function that creates AZ::Module class.
                    // It's acceptable for a library not to have this function.
                    auto createModuleFunction = moduleDataPtr->m_dynamicHandle->GetFunction<CreateModuleClassFunction>(CreateModuleClassFunctionName);
                    if (!createModuleFunction)
                    {
                        // It's an error if the library is missing a CreateModuleClass() function.
                        return AZ::Failure(AZStd::string::format("'%s' entry point in module \"%s\" failed to create AZ::Module.",
                            CreateModuleClassFunctionName,
                            moduleName));
                    }

                    // Create AZ::Module class
                    moduleDataPtr->m_module = createModuleFunction();
                    if (!moduleDataPtr->m_module)
                    {
                        // It's an error if the library has a CreateModuleClass() function that returns nothing.
                        return AZ::Failure(AZStd::string::format("'%s' entry point in module \"%s\" failed to create AZ::Module.",
                            CreateModuleClassFunctionName,
                            moduleName));
                    }

                    return AZ::Success();
                }
            },

            // Register the module descriptors
            {
                ModuleInitializationSteps::RegisterComponentDescriptors,
                [&moduleDataPtr]() -> PhaseOutcome
                {
                    moduleDataPtr->m_module->RegisterComponentDescriptors();
                    return AZ::Success();
                }
            },

            // Activate the module's entity
            {
                ModuleInitializationSteps::ActivateEntity,
                [this, &moduleDataPtr]() -> PhaseOutcome
                {
                    Entity* systemEntity = nullptr;
                    ComponentApplicationBus::BroadcastResult(systemEntity, &ComponentApplicationBus::Events::FindEntity, SystemEntityId);

                    // Only initialize components if the system entity is up and activated. Otherwise, it'll be inited later.
                    if (systemEntity && systemEntity->GetState() == Entity::State::Active)
                    {
                        // Initialize the entity (explicitly do not sort, because the dependencies are probably already met.
                        AZStd::vector<AZStd::shared_ptr<ModuleDataImpl>> modulesToInit = { moduleDataPtr };
                        ActivateEntities(modulesToInit);
                    }

                    return AZ::Success();
                }
            },
        };

        // Iterate over each pair and execute it
        for (const auto& phasePair : phases)
        {
            // If this step is already complete, skip it
            if (moduleDataPtr->m_lastCompletedStep >= phasePair.first)
            {
                continue;
            }

            PhaseOutcome phaseResult = phasePair.second();
            if (!phaseResult.IsSuccess())
            {
                // Remove all references to the module from the owned and unowned list
                if (ownedModuleIt != m_ownedModules.end())
                {
                    m_ownedModules.erase(ownedModuleIt);
                }
                moduleDataPtr.reset();
                m_nameToModuleMap.erase(preprocessedModulePath);

                return AZ::Failure(AZStd::move(phaseResult.GetError()));
            }

            // Update the state of the module
            moduleDataPtr->m_lastCompletedStep = phasePair.first;

            // If this was the last step to perform, then we're done.
            if (phasePair.first == lastStepToPerform)
            {
                break;
            }
        }

        return AZ::Success(AZStd::static_pointer_cast<ModuleData>(moduleDataPtr));
    }

    //=========================================================================
    // LoadDynamicModules
    //=========================================================================
    ModuleManager::LoadModulesResult ModuleManager::LoadDynamicModules(const ModuleDescriptorList& modules, ModuleInitializationSteps lastStepToPerform, bool maintainReferences)
    {
        LoadModulesResult results;

        Internal::ModuleManagerSearchPathTool moduleSearchPathHelper;

        // Load DLLs specified in the application descriptor
        for (const auto& moduleDescriptor : modules)
        {
            // For each module that is loaded, attempt to set the module's folder as a path for dependent module resolution
            moduleSearchPathHelper.SetModuleSearchPath(moduleDescriptor);

            LoadModuleOutcome result = LoadDynamicModule(moduleDescriptor.m_dynamicLibraryPath.c_str(), lastStepToPerform, maintainReferences);
            results.emplace_back(AZStd::move(result));
        }

        return results;
    }

    //=========================================================================
    // LoadStaticModules
    //=========================================================================
    ModuleManager::LoadModulesResult ModuleManager::LoadStaticModules(CreateStaticModulesCallback staticModulesCb, ModuleInitializationSteps lastStepToPerform)
    {
        ModuleManager::LoadModulesResult results;

        // Load static modules
        if (staticModulesCb)
        {
            // Get the System Entity
            Entity* systemEntity = nullptr;
            ComponentApplicationBus::BroadcastResult(systemEntity, &ComponentApplicationBus::Events::FindEntity, SystemEntityId);

            AZStd::vector<Module*> staticModules;
            staticModulesCb(staticModules);

            for (Module* module : staticModules)
            {
                if (!module)
                {
                    AZ_Warning(s_moduleLoggingScope, false, "Nullptr module somehow inserted during call to static module creation function. Ignoring...");
                    continue;
                }

                auto moduleData = AZStd::make_shared<ModuleDataImpl>();
                moduleData->m_module = module;
                moduleData->m_lastCompletedStep = ModuleInitializationSteps::CreateClass;

                if (lastStepToPerform >= ModuleInitializationSteps::RegisterComponentDescriptors)
                {
                    moduleData->m_module->RegisterComponentDescriptors();
                    moduleData->m_lastCompletedStep = ModuleInitializationSteps::RegisterComponentDescriptors;
                }

                if (lastStepToPerform >= ModuleInitializationSteps::ActivateEntity)
                {
                    // Only initialize components if the system entity is up and activated. Otherwise, it'll be inited later.
                    if (systemEntity && systemEntity->GetState() == Entity::State::Active)
                    {
                        // Initialize the entity (explicitly do not sort, because the dependencies are probably already met.
                        AZStd::vector<AZStd::shared_ptr<ModuleDataImpl>> modulesToInit = { moduleData };
                        ActivateEntities(modulesToInit);
                    }

                    moduleData->m_lastCompletedStep = ModuleInitializationSteps::ActivateEntity;
                }

                // Success! Move ModuleData into a permanent location
                m_ownedModules.emplace_back(moduleData);

                // Store the result to be returned
                results.emplace_back(AZ::Success(AZStd::static_pointer_cast<ModuleData>(moduleData)));
            }
        }

        return results;
    }

    //=========================================================================
    // IsModuleLoaded
    //=========================================================================
    bool ModuleManager::IsModuleLoaded(const char* modulePath)
    {
        return GetLoadedModule(modulePath) != nullptr;
    }

    AZStd::shared_ptr<AZ::ModuleDataImpl> ModuleManager::GetLoadedModule(AZStd::string_view modulePath)
    {
        AZ::OSString preprocessedModulePath = PreProcessModule(modulePath);
        if (auto moduleIt = m_nameToModuleMap.find(preprocessedModulePath); moduleIt != m_nameToModuleMap.end())
        {
            auto&&[moduleName, moduleDataWeak] = *moduleIt;
            return moduleDataWeak.lock();
        }

        return {};
    }

    //=========================================================================
    // HandleDependencySortError
    //=========================================================================
    void ModuleManager::HandleDependencySortError(const Entity::DependencySortOutcome& outcome)
    {
        // Print a short message to the log, and an extended message to the nativeUI (if available)
        auto errorMessage = AZStd::string::format("Modules Entities cannot be activated.\n\n%s", outcome.GetError().m_message.c_str());
        AZ_Error(s_moduleLoggingScope, false, errorMessage.c_str());

        auto nativeUI = AZ::Interface<AZ::NativeUI::NativeUIRequests>::Get();
        if (nativeUI)
        {
            errorMessage.append("\n\n");
            errorMessage.append(outcome.GetError().m_extendedMessage);
            auto choice = nativeUI->DisplayBlockingDialog(s_moduleLoggingScope, errorMessage, { "Quit", "Ignore" });
            m_quitRequested = (choice == "Quit");
        }
    }

    //=========================================================================
    // OnEntityActivated
    //=========================================================================
    void ModuleManager::OnEntityActivated(const AZ::EntityId& entityId)
    {
        AZ_Assert(entityId == SystemEntityId, "OnEntityActivated called for wrong entity id (expected SystemEntityId)!");
        EntityBus::Handler::BusDisconnect(entityId);

        ActivateEntities(m_ownedModules);
    }

    //=========================================================================
    // ActivateEntities
    //=========================================================================
    void ModuleManager::ActivateEntities(const AZStd::vector<AZStd::shared_ptr<ModuleDataImpl>>& modulesToInit)
    {
        // Grab the system entity
        Entity* systemEntity = nullptr;
        ComponentApplicationBus::BroadcastResult(systemEntity, &ComponentApplicationBus::Events::FindEntity, SystemEntityId);

        SerializeContext* serialize = nullptr;
        ComponentApplicationBus::BroadcastResult(serialize, &ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(serialize, "Internal error: serialize context not found");

        // Will store components that need activation
        Entity::ComponentArrayType componentsToActivate;

        // Init all modules
        for (auto& moduleData : modulesToInit)
        {
            const char* moduleName = moduleData->GetDebugName();

            // Create module entity if it wasn't deserialized
            if (!moduleData->m_moduleEntity)
            {
                moduleData->m_moduleEntity = AZStd::make_unique<ModuleEntity>(azrtti_typeid(moduleData->m_module), moduleName);
            }

            // Default to the Required System Components List
            ComponentTypeList requiredComponents = moduleData->m_module->GetRequiredSystemComponents();

            // Sort through components registered, pull ones required in this context
            for (const auto& descriptor : moduleData->m_module->GetComponentDescriptors())
            {
                if (ShouldUseSystemComponent(*descriptor, m_systemComponentTags, *serialize))
                {
                    requiredComponents.emplace_back(descriptor->GetUuid());
                }
            }

            // Populate with required system components
            for (const Uuid& componentTypeId : requiredComponents)
            {
                // Add the component to the entity (if it's not already there OR on the System Entity)
                Component* component = moduleData->m_moduleEntity->FindComponent(componentTypeId);
                if (!component && !(systemEntity && systemEntity->FindComponent(componentTypeId)))
                {
                    component = moduleData->m_moduleEntity->CreateComponent(componentTypeId);
                    AZ_Warning(s_moduleLoggingScope, component != nullptr, "Failed to find Required System Component of type %s in module %s. Did you register the descriptor?", componentTypeId.ToString<AZStd::string>().c_str(), moduleName);
                }

                // This can be nullptr if the component was on the System Entity or if the component was somehow not found
                if (component)
                {
                    // Store in list to topo sort later
                    componentsToActivate.emplace_back(component);
                }
            }

            // Init the module entity, set it to Activating state
            moduleData->m_moduleEntity->Init();
            // Remove from Component Application so that we can delete it, not the app. It gets added during Init()
            ComponentApplicationBus::Broadcast(&ComponentApplicationBus::Events::RemoveEntity, moduleData->m_moduleEntity.get());
            // Set the entity to think it's activating
            moduleData->m_moduleEntity->SetState(Entity::State::Activating);
        }

        // Add all components that are currently activated so that dependencies may be fulfilled
        componentsToActivate.insert(componentsToActivate.begin(), m_systemComponents.begin(), m_systemComponents.end());

        // Get all the components from the System Entity, to include for sorting purposes
        if (systemEntity)
        {
            const Entity::ComponentArrayType& systemEntityComponents = systemEntity->GetComponents();
            componentsToActivate.insert(componentsToActivate.begin(), systemEntityComponents.begin(), systemEntityComponents.end());
        }

        // Topo sort components, activate them
        Entity::DependencySortOutcome outcome = ModuleEntity::DependencySort(componentsToActivate);
        if (!outcome.IsSuccess())
        {
            HandleDependencySortError(outcome);
            if (m_quitRequested)
            {
                // Before letting the application quit, all the module entities should be restored back to init state
                // because they never fully exited the activating state.
                for (auto& moduleData : modulesToInit)
                {
                    moduleData->m_moduleEntity->SetState(Entity::State::Init);
                }
            }
            return;
        }

        for (auto componentIt = componentsToActivate.begin(); componentIt != componentsToActivate.end(); )
        {
            Component* component = *componentIt;

            // Remove the system entity and already activated components, we don't need to activate or store those
            if (component->GetEntityId() == SystemEntityId ||
                AZStd::find(m_systemComponents.begin(), m_systemComponents.end(), component) != m_systemComponents.end())
            {
                componentIt = componentsToActivate.erase(componentIt);
            }
            else
            {
                ++componentIt;
            }
        }

        AZStd::string componentNamesArray = R"({ "SystemComponents":[)";
        const char* comma = "";
        // Activate the entities in the appropriate order
        for (Component* component : componentsToActivate)
        {
            ModuleEntity::ActivateComponent(*component);

            componentNamesArray += AZStd::string::format(R"(%s"%s")", comma, component->RTTI_GetTypeName());
            comma = ", ";
        }
        componentNamesArray += R"(]})";

        // Done activating; set state to active
        for (auto& moduleData : modulesToInit)
        {
            if (moduleData->m_moduleEntity)
            {
                moduleData->m_moduleEntity->SetState(Entity::State::Active);
            }
            moduleData->m_lastCompletedStep = ModuleInitializationSteps::ActivateEntity;
        }

        // Save the activated components for deactivation later
        m_systemComponents.insert(m_systemComponents.end(), componentsToActivate.begin(), componentsToActivate.end());

        // Signal that the System Components are activated
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            AZ::ComponentApplicationLifecycle::SignalEvent(*settingsRegistry, "SystemComponentsActivated",
                componentNamesArray);
        }
    }
} // namespace AZ
