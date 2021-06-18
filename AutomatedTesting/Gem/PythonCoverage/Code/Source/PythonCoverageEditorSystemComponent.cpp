#include <AzCore/Module/ModuleManagerBus.h>
#include <AzCore/Module/Module.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <AzCore/IO/Path/Path.h>

#include <PythonCoverageEditorSystemComponent.h>

#include <inttypes.h>

#include <fstream>

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
        AzToolsFramework::EditorPythonRunnerNotificationBus::Handler::BusConnect();
        AZ::EntitySystemBus::Handler::BusConnect();
        EnumerateAllModuleComponents();
    }

    void PythonCoverageEditorSystemComponent::Deactivate()
    {
        AZ::EntitySystemBus::Handler::BusDisconnect();
        AzToolsFramework::EditorPythonRunnerNotificationBus::Handler::BusDisconnect();
    }

    void PythonCoverageEditorSystemComponent::OnEntityActivated(const AZ::EntityId& entityId)
    {
        EnumerateComponentsForEntity(entityId);
        WriteCoverageFile();
    }
    
    void PythonCoverageEditorSystemComponent::WriteCoverageFile()
    {
        // Yes, we're doing blocking file operations on the main thread... If this becomes an issue this can be offloaded
        // to a worker thread
        if (m_coverageFile.has_value())
        {
            AZStd::string contents;
            const auto coveringModules = GetParentComponentModulesForAllActivatedEntities();
            if (coveringModules.empty())
            {
                return;
            }
    
            for (const auto& coveringModule : coveringModules)
            {
                contents += coveringModule + "\n";
            }
    
            AZ::IO::SystemFile file;
            const AZStd::vector<char> bytes(contents.begin(), contents.end());
            if (!file.Open(
                    m_coverageFile.value().c_str(),
                    AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY))
            {
                AZ_Error(
                    "PythonCoverageEditorSystemComponent", false,
                    AZStd::string::format("Couldn't open file %s for writing", m_coverageFile.value().c_str()).c_str());
                return;
            }
    
            if (!file.Write(bytes.data(), bytes.size()))
            {
                AZ_Error(
                    "PythonCoverageEditorSystemComponent", false,
                    AZStd::string::format("Couldn't write contents for file %s", m_coverageFile.value().c_str()).c_str());
                return;
            }
        }
    }
    
    void PythonCoverageEditorSystemComponent::EnumerateAllModuleComponents()
    {
        AZ::ModuleManagerRequestBus::Broadcast(
            &AZ::ModuleManagerRequestBus::Events::EnumerateModules,
            [this](const AZ::ModuleData& moduleData)
            {
                const AZStd::string moduleName = moduleData.GetDebugName();
                if (moduleData.GetDynamicModuleHandle())
                {
                    const auto fileName = moduleData.GetDynamicModuleHandle()->GetFilename();
                    for (const auto* moduleComponentDescriptor : moduleData.GetModule()->GetComponentDescriptors())
                    {
                        m_moduleComponents[moduleComponentDescriptor->GetUuid()] = moduleData.GetDebugName();
                    }
                }
    
                return true;
            });
    }
    
    void PythonCoverageEditorSystemComponent::EnumerateComponentsForEntity(const AZ::EntityId& entityId)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, AZ::EntityId(entityId));
    
        if (entity)
        {
            for (const auto& entityComponent : entity->GetComponents())
            {
                const auto componentTypeId = entityComponent->GetUnderlyingComponentType();
                AZ::ComponentDescriptor* componentDescriptor = nullptr;
                AZ::ComponentDescriptorBus::EventResult(
                    componentDescriptor, componentTypeId, &AZ::ComponentDescriptorBus::Events::GetDescriptor);
                m_entityComponents[componentTypeId] = componentDescriptor;
            }
        }
    }
    
    AZStd::unordered_set<AZStd::string> PythonCoverageEditorSystemComponent::GetParentComponentModulesForAllActivatedEntities() const
    {
        AZStd::unordered_set<AZStd::string> coveringModuleOutputNames;
        for (const auto& [uuid, componentDescriptor] : m_entityComponents)
        {
            if (const auto moduleComponent = m_moduleComponents.find(uuid); moduleComponent != m_moduleComponents.end())
            {
                coveringModuleOutputNames.insert(moduleComponent->second);
            }
        }
    
        return coveringModuleOutputNames;
    }
    
    void PythonCoverageEditorSystemComponent::ExecuteByFilenameAsTest(AZStd::string_view filename, const AZStd::vector<AZStd::string_view>& args)
    {
        m_entityComponents.clear();
        m_coverageFile = AZStd::nullopt;
        
        // Check to see if we have the coverage directory argument
        if (auto tiafDirArg = AZStd::find(args.begin(), args.end(), "tiaf_coverage_dir");
            tiafDirArg != args.end())
        {
            // Check to see if we have a value for this argument
            if (++tiafDirArg != args.end())
            {
                const AZStd::string testName = AZ::IO::Path(filename).Stem().Native();
                m_coverageFile = AZ::IO::Path(*tiafDirArg) / AZStd::string::format("%s.coverage", testName.c_str());
            }
            else
            {
                AZ_Error("PythonCoverageEditorSystemComponent", false, "tiaf_coverage_dir argument has no value");
            }
        }
    }
} // namespace PythonCoverage
