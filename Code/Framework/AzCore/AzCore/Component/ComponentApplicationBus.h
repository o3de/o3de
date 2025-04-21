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
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/string/osstring.h>

namespace AZ
{
    class CommandLine;
    class ComponentApplication;
    class ComponentDescriptor;

    class Entity;
    class EntityId;

    class Module;
    class DynamicModuleHandle;

    class Component;

    class SerializeContext;
    class BehaviorContext;
    class JsonRegistrationContext;

    namespace Internal
    {
        class ComponentFactoryInterface;
    }

    struct ApplicationTypeQuery
    {
        //! Signals if the application is the Editor.
        bool IsEditor() const;

        //! Signals if the application is the tool application, i.e. AssetProcessor.
        bool IsTool() const;

        //! Signals if the application is a game or server launcher.
        bool IsGame() const;

        //! Signals if the application is running headless (console application with no graphics rendering capability enabled).
        bool IsHeadless() const;

        //! Signals if the application is valid or not. This means the application has not been categorized as any one of Editor, Tool, or Game.
        bool IsValid() const;

        //! Signals if the application is running in console mode where the native client window is not created but still (optionally) supports graphics rendering.
        bool IsConsoleMode() const;

        enum class Masks
        {
            Invalid = 0,
            Editor = 1 << 0,
            Tool = 1 << 1,
            Game = 1 << 2,
            Headless = 1 << 3,
            ConsoleMode = 1 << 4,
        };
        Masks m_maskValue = Masks::Invalid;
    };
    AZ_DEFINE_ENUM_BITWISE_OPERATORS(ApplicationTypeQuery::Masks);

    // use of the Masks operator&(Masks, Masks) needs to be after the definition above.
    inline bool ApplicationTypeQuery::IsEditor() const { return (m_maskValue & Masks::Editor) == Masks::Editor; }
    inline bool ApplicationTypeQuery::IsTool() const { return (m_maskValue & Masks::Tool) == Masks::Tool; }
    inline bool ApplicationTypeQuery::IsGame() const { return (m_maskValue & Masks::Game) == Masks::Game; }
    inline bool ApplicationTypeQuery::IsHeadless() const { return (m_maskValue & Masks::Headless) == Masks::Headless; }
    inline bool ApplicationTypeQuery::IsConsoleMode() const { return (m_maskValue & Masks::ConsoleMode) == Masks::ConsoleMode; }
    inline bool ApplicationTypeQuery::IsValid() const { return m_maskValue != Masks::Invalid; }

    using EntityAddedEvent = AZ::Event<AZ::Entity*>;
    using EntityRemovedEvent = AZ::Event<AZ::Entity*>;
    using EntityActivatedEvent = AZ::Event<AZ::Entity*>;
    using EntityDeactivatedEvent = AZ::Event<AZ::Entity*>;

    //! Interface that components can use to make requests of the main application.
    class ComponentApplicationRequests
    {
    public:
        AZ_RTTI(ComponentApplicationRequests, "{E8BE41B7-615F-4FE8-B611-8A9E441290A8}");

        //! Destroys the event bus that components use to make requests of the main application.
        virtual ~ComponentApplicationRequests() = default;

        //! Registers a component descriptor with the application.
        //! @param descriptor A component descriptor.
        virtual void RegisterComponentDescriptor(const ComponentDescriptor* descriptor) = 0;

        //! Unregisters a component descriptor with the application.
        //! @param descriptor A component descriptor.
        virtual void UnregisterComponentDescriptor(const ComponentDescriptor* descriptor) = 0;

        //! Gets a pointer to the application.
        //! @return A pointer to the application.
        virtual ComponentApplication* GetApplication() = 0;

        //! Registers an event handler that will be signalled whenever an entity is added.
        //! @param handler the event handler to signal.
        virtual void RegisterEntityAddedEventHandler(EntityAddedEvent::Handler& handler) = 0;

        //! Registers an event handler that will be signalled whenever an entity is removed.
        //! @param handler the event handler to signal.
        virtual void RegisterEntityRemovedEventHandler(EntityRemovedEvent::Handler& handler) = 0;

        //! Registers an event handler that will be signalled whenever an entity is added.
        //! @param handler the event handler to signal.
        virtual void RegisterEntityActivatedEventHandler(EntityActivatedEvent::Handler& handler) = 0;

        //! Registers an event handler that will be signalled whenever an entity is removed.
        //! @param handler the event handler to signal.
        virtual void RegisterEntityDeactivatedEventHandler(EntityDeactivatedEvent::Handler& handler) = 0;

        //! Signals that the provided entity has been activated.
        //! @param entity the entity being activated.
        virtual void SignalEntityActivated(AZ::Entity* entity) = 0;

        //! Signals that the provided entity has been deactivated.
        //! @param entity the entity being deactivated.
        virtual void SignalEntityDeactivated(AZ::Entity* entity) = 0;

        //! Adds an entity to the application's registry.
        //! Calling Init() on an entity automatically performs this operation.
        //! @param entity A pointer to the entity to add to the application's registry.
        //! @return True if the operation succeeded. False if the operation failed.
        virtual bool AddEntity(Entity* entity) = 0;

        //! Removes the specified entity from the application's registry.
        //! Deleting an entity automatically performs this operation.
        //! @param entity A pointer to the entity that will be removed from the application's registry.
        //! @return True if the operation succeeded. False if the operation failed.
        virtual bool RemoveEntity(Entity* entity) = 0;

        //! Unregisters and deletes the specified entity.
        //! @param entity A reference to the entity that will be unregistered and deleted.
        //! @return True if the operation succeeded. False if the operation failed.
        virtual bool DeleteEntity(const EntityId& id) = 0;

        //! Returns the entity with the matching ID, if the entity is registered with the application.
        //! @param entity A reference to the entity that you are searching for.
        //! @return A pointer to the entity with the specified entity ID.
        virtual Entity* FindEntity(const EntityId& id) = 0;

        //! Returns the name of the entity that has the specified entity ID.
        //! Entity names are not unique.
        //! This method exists to facilitate better debugging messages.
        //! @param entity A reference to the entity whose name you are seeking.
        //! @return The name of the entity with the specified entity ID. 
        //! If no entity is found for the specified ID, it returns an empty string. 
        virtual AZStd::string GetEntityName(const EntityId& id) { (void)id; return AZStd::string(); }

        //! Sets the name of the entity that has the specified entity ID.
        //! Entity names are not enforced to be unique.
        //! @param entityId A reference to the entity whose name you want to change.
        //! @return True if the name was changed successfully, false if it wasn't.
        virtual bool SetEntityName([[maybe_unused]] const EntityId& id, [[maybe_unused]] const AZStd::string_view name) { return false; }

        //! The type that AZ::ComponentApplicationRequests::EnumerateEntities uses to
        //! pass entity callbacks to the application for enumeration.
        using EntityCallback = AZStd::function<void(Entity*)>;

        //! Enumerates all registered entities and invokes the specified callback for each entity.
        //! @param callback A reference to the callback that is invoked for each entity.
        virtual void EnumerateEntities(const EntityCallback& callback) = 0;

        //! Returns the serialize context that was registered with the app.
        //! @return The serialize context, if there is one. SerializeContext is a class that contains reflection data
        //! for serialization and construction of objects.
        virtual class SerializeContext* GetSerializeContext() = 0;

        //! Returns the behavior context that was registered with the app.
        //! @return The behavior context, if there is one. BehaviorContext is a class that reflects classes, methods,
        //! and EBuses for runtime interaction.
        virtual class BehaviorContext*  GetBehaviorContext() = 0;

        //! Returns the Json Registration context that was registered with the app.
        //! @return The Json Registration context, if there is one. JsonRegistrationContext is a class that contains
        //! the serializers used by the best-effort json serialization.
        virtual class JsonRegistrationContext* GetJsonRegistrationContext() = 0;

        //! Gets the path of the working engine folder that the app is a part of.
        //! @return a pointer to the engine path.
        virtual const char* GetEngineRoot() const = 0;

        //! Gets the path to the directory that contains the application's executable.
        //! @return a pointer to the name of the path that contains the application's executable.
        virtual const char* GetExecutableFolder() const = 0;

        //! ResolveModulePath is called whenever LoadDynamicModule wants to resolve a module in order to actually load it.
        //! You can override this if you need to load modules from a different path or hijack module loading in some other way.
        //! If you do, ensure that you use platform-specific conventions to do so, as this is called by multiple platforms.
        //! The default implantation prepends the path to the executable to the module path, but you can override this behavior
        //! (Call the base class if you want this behavior to persist in overrides)
        virtual void ResolveModulePath([[maybe_unused]] AZ::OSString& modulePath) { }

        //! Returns AZ parsed command line structure.
        //! Command Line structure can be queried for switches (-<switch> /<switch>) or positional parameter (<value>)
        virtual AZ::CommandLine* GetAzCommandLine() { return{}; }

        //! Returns all the flags that are true for the current application.
        virtual void QueryApplicationType(ApplicationTypeQuery& appType) const = 0;
    };

    class ComponentApplicationRequestsEBusTraits
        : public AZ::EBusTraits
    {
    public:
        //! EBusTraits overrides - application is a singleton
        //! Overrides the default AZ::EBusTraits handler policy to allow one
        //! listener only, because only one application can exist at a time.
        static const AZ::EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;  // We sort components on m_initOrder.

        //! Overrides the default AZ::EBusTraits mutex type to the AZStd implementation of
        //! a recursive mutex with exclusive ownership semantics. A mutex prevents multiple
        //! threads from accessing shared data simultaneously.
        using MutexType = AZStd::recursive_mutex;
    };

    //! Used by components to make requests of the component application.
    using ComponentApplicationBus = AZ::EBus<ComponentApplicationRequests, ComponentApplicationRequestsEBusTraits>;
}

DECLARE_EBUS_EXTERN_DLL_SINGLE_ADDRESS_WITH_TRAITS(ComponentApplicationRequests, ComponentApplicationRequestsEBusTraits);
