#include <AzCore/Module/ModuleManagerBus.h>
#include <AzCore/Module/Module.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/IO/Path/Path.h>

#include <PythonCoverageEditorSystemComponent.h>

#include <inttypes.h> 

#pragma optimize("", off)

namespace PythonCoverage
{
    void PythonCoverageEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PythonCoverageEditorSystemComponent, AZ::Component>()->Version(1);
        }
    }

    void PythonCoverageEditorSystemComponent::Activate()
    {
        AZ::EntitySystemBus::Handler::BusConnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        AzFramework::ApplicationRequests::Bus::Handler::BusConnect();
    }

    void PythonCoverageEditorSystemComponent::Deactivate()
    {
        AzFramework::ApplicationRequests::Bus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        AZ::EntitySystemBus::Handler::BusDisconnect();
    }

    void PythonCoverageEditorSystemComponent::OnEntityActivated(const AZ::EntityId& entityId)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, AZ::EntityId(entityId));
        
        if (entity)
        {
            // Get the components on this entity
            AZStd::unordered_map<AZStd::string, AZ::ComponentDescriptor*> entityComponents;
            for(const auto& entityComponent : entity->GetComponents())
            {
                const auto componentTypeId = entityComponent->GetUnderlyingComponentType();
                //entityComponents.insert(entityComponent->GetUnderlyingComponentType().ToString<AZStd::string>());
                AZ::ComponentDescriptor* componentDescriptor = nullptr;
                AZ::ComponentDescriptorBus::EventResult(
                    componentDescriptor, componentTypeId,
                    &AZ::ComponentDescriptorBus::Events::GetDescriptor);
                entityComponents[componentTypeId.ToString<AZStd::string>()] = componentDescriptor;
            }

            // Get the modules and their components
            AZStd::unordered_map<AZStd::string, AZStd::string> moduleComponents;
            AZ::ModuleManagerRequestBus::Broadcast(
            &AZ::ModuleManagerRequestBus::Events::EnumerateModules,
                [&moduleComponents](const AZ::ModuleData& moduleData)
                {
                    const AZStd::string moduleName = moduleData.GetDebugName();
                    if (moduleData.GetDynamicModuleHandle())
                    {
                        const auto fileName = moduleData.GetDynamicModuleHandle()->GetFilename();
                        for (const auto* moduleComponentDescriptor : moduleData.GetModule()->GetComponentDescriptors())
                        {
                            moduleComponents[moduleComponentDescriptor->GetUuid().ToString<AZStd::string>()] = moduleData.GetDebugName();
                        }
                    }
                    
                    return true;
                });

            // Get the set of modules that are direct dependencies of this entity
            AZStd::unordered_set<AZStd::string> entityModules;
            for (const auto& [uuid, componentDescriptor] : entityComponents)
            {
                if (const auto moduleComponent = moduleComponents.find(uuid); moduleComponent != moduleComponents.end())
                {
                    entityModules.insert(moduleComponent->second);
                }
            }

            if (!entityModules.empty())
            {
                AZStd::string msg;
                msg += AZStd::string::format("Entity '%s' with has direct dependencies of the following gems:\n", entity->GetName().c_str());
                for (const auto& moduleName : entityModules)
                {
                    const AZStd::string gemName = AZ::IO::Path(moduleName).Stem().Native();
                    msg += AZStd::string::format("'%s'\n", gemName.c_str());
                }
                AZ_Printf("PPythonCoverage", "OnEntityActivated:\n%s", msg.c_str());
            }
        }
        else
        {
            AZ_Printf("PPythonCoverage", "OnEntityActivated: <unknown>");
        }
    }

    void PythonCoverageEditorSystemComponent::ExitMainLoop()
    {
        AZ_Printf("PPythonCoverage", "Goodbye!");
    }

} // namespace PythonCoverage
