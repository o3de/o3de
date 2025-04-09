/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>

#include "TickBusOrderViewerSystemComponent.h"

#include <IGem.h>
#include <CryCommon/IConsole.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Module/ModuleManager.h>
#include <AzCore/std/string/conversions.h>

namespace TickBusOrderViewer
{
    /**
    * Helper function for finding an entity by ID. Checks both the component application, and loaded modules.
    */
    AZ::Entity* FindEntity(const AZ::EntityId& entityId)
    {
        // First check the component application for the entity. This is where game, editor, and the root system entities live.
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);

        if (entity == nullptr)
        {
            // The entity was not in the component application's entity list, but it may be a system entity for a module.
            AZ::ModuleManagerRequestBus::Broadcast(&AZ::ModuleManagerRequestBus::Events::EnumerateModules, [&entity, entityId](const AZ::ModuleData& moduleData)
            {
                AZ::Entity* moduleEntity = moduleData.GetEntity();
                if (moduleEntity != nullptr && moduleEntity->GetId() == entityId)
                {
                    entity = moduleEntity;
                    // The entity was found, so stop the enumeration.
                    return false;
                }
                // The matching entity is not in this module, keep looking through the rest of the modules.
                return true;
            });
        }
        return entity;
    }

    /**
    * Prints out all connected tickbus handlers, in the order they are ticked.
    * @param entityId An optional entity ID used to only display handlers for components on this entity.
    *                 displays all handlers if this is null.
    */
    void PrintTickbusHandlers(AZ::EntityId* entityId)
    {
        AZStd::string tickbusPrintoutTitle = "TickBus handlers in tick order";

        // If an entity ID was given, then update the printout title to contain information about that entity.
        if (entityId != nullptr)
        {
            // Search for the passed in entity, to print out its name.
            // Most people are going to think of their entities in terms of the name, and not the ID.
            AZ::Entity* entity = FindEntity(*entityId);

            if (entity != nullptr)
            {
                tickbusPrintoutTitle = AZStd::string::format("%s for entity \"%s\" %s",
                    tickbusPrintoutTitle.c_str(),
                    entity->GetName().c_str(),
                    entityId->ToString().c_str());
            }
            else
            {
                // If the entity wasn't found, then print out the ID at least.
                tickbusPrintoutTitle = AZStd::string::format("%s for entity with id %s, entity could not be found",
                    tickbusPrintoutTitle.c_str(),
                    entityId->ToString().c_str());
            }

        }
        AZ_Printf("TickBusOrderViewer", tickbusPrintoutTitle.c_str());

        // Visit every tickbus handler. These are already sorted in the order they will be called.
        AZ::TickBus::EnumerateHandlers([entityId](AZ::TickEvents* handler)
        {
            // If this handler is a component, then it will have an associated entity.
            // This will allow printing additional, useful information for the user.
            AZ::Component* component = azrtti_cast<AZ::Component*>(handler);
            if (component && (entityId == nullptr || component->GetEntityId() == *entityId))
            {
                AZStd::string entityName = component->GetEntity() != nullptr ?
                    component->GetEntity()->GetName() : "[No entity found]";
                // Print out everything about this tickbus listener that can help the user debug
                // their tick ordering issue that caused them to call this.
                // This includes:
                // * The component's type as a string and a UUID.
                // * The component's individual ID, which can be useful if the entity has duplicate components of the same type.
                // * The associated entity's name, so the user can trace this tick handler to the entity.
                // * The associated entity's ID, because entity names may not be unique.
                AZ_Printf("TickBusOrderViewer", "\t%d - Entity \"%s\" %s, component %s %s with ID %u",
                    handler->GetTickOrder(),
                    entityName.c_str(),
                    component->GetEntityId().ToString().c_str(),
                    component->RTTI_GetTypeName(),
                    component->RTTI_GetType().ToString<AZStd::string>().c_str(),
                    component->GetId());
            }
            else if(entityId == nullptr)
            {
                // This handler wasn't a component, so print out as much information as can be gathered.
                AZ_Printf("TickBusOrderViewer", "\t%d - Object with type %s %s",
                    handler->GetTickOrder(),
                    handler->RTTI_GetTypeName(),
                    handler->RTTI_GetType().ToString<AZStd::string>().c_str());
            }
            // Return true so the enumeration continues, all handles need to be checked.
            return true;
        });
    }

    /**
    * Console command to print the handlers for the tickbus, in the order they are ticked.
    */
    void PrintTickbusHandlerOrder(IConsoleCmdArgs* args)
    {
        // If only the command was supplied with no entity ID, then print out information for
        // all tickbus handlers.
        if (args == nullptr || args->GetArgCount() == 1)
        {
            PrintTickbusHandlers(nullptr);
            return;
        }
        // If the passed in argument was not valid, print a warning and then the information for
        // all tickbus handlers.
        const char* entityIdString = args->GetArg(1);
        if (entityIdString == nullptr)
        {
            AZ_Warning("TickBusOrderViewer", false, "print_tickbus_handlers was called with an invalid parameter, printing out all handlers.");
            PrintTickbusHandlers(nullptr);
            return;
        }
        // Convert the passed in string to an entity ID. If this fails, then the user will need
        // to run the command again with a better formatted entity ID.
        AZ::EntityId entityId(AZStd::stoull(AZStd::string(entityIdString)));
        PrintTickbusHandlers(&entityId);
    }

    class TickBusOrderViewerModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(TickBusOrderViewerModule, "{DAE8B6D3-23ED-4547-9D0C-9F42CA812A06}", CryHooksModule);
        AZ_CLASS_ALLOCATOR(TickBusOrderViewerModule, AZ::SystemAllocator);

        TickBusOrderViewerModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                TickBusOrderViewerSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<TickBusOrderViewerSystemComponent>(),
            };
        }
        /**
        * Override for CryHooksModule::OnCrySystemInitialized to add the console commands
        * to print out tick bus information.
        */
        void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& initParams) override
        {
            CryHooksModule::OnCrySystemInitialized(system, initParams);

            // Register the command to print the tickbus handlers out.
            REGISTER_COMMAND("print_tickbus_handlers", &PrintTickbusHandlerOrder, 0, "Prints out the handlers for the tickbus in tick order. "
            "With zero parameters, prints all handlers. With one parameter, it converts that to an entity ID and only prints components for that entity.");
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), TickBusOrderViewer::TickBusOrderViewerModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_TickBusOrderViewer, TickBusOrderViewer::TickBusOrderViewerModule)
#endif
