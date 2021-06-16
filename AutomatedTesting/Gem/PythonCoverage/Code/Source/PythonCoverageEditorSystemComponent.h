
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/optional.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>

namespace AZ
{
    class ComponentDescriptor;
}

namespace PythonCoverage
{
    /// System component for PythonCoverage editor
    class PythonCoverageEditorSystemComponent
        : public AZ::Component
        , private AZ::EntitySystemBus::Handler
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private AzToolsFramework::EditorPythonRunnerRequestBus::Handler
    {
    public:
        AZ_COMPONENT(PythonCoverageEditorSystemComponent, "{33370075-3aea-49c4-823d-476f8ac95b6f}");
        static void Reflect(AZ::ReflectContext* context);

        PythonCoverageEditorSystemComponent() = default;

    private:
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AZ::EntitySystemBus ...
        void OnEntityActivated(const AZ::EntityId& entityId) override;

        // AZ::EditorPythonRunnerRequestBus ...
        void ExecuteByFilenameAsTest(AZStd::string_view filename, const AZStd::vector<AZStd::string_view>& args) override;

        void EnumerateAllModuleComponents();
        void EnumerateComponentsForEntity(const AZ::EntityId& entityId);
        AZStd::unordered_set<AZStd::string> GetParentComponentModulesForAllActivatedEntities() const;
        void WriteCoverageFile();

        AZStd::unordered_map<AZ::Uuid, AZ::ComponentDescriptor*> m_entityComponents; //!< Map of component IDs to component descriptors for all activated entities.
        AZStd::unordered_map<AZ::Uuid, AZStd::string> m_moduleComponents; //!< Map of component IDs to module names for all modules.
        AZStd::optional<AZ::IO::Path> m_coverageFile; //!< File to write coverage data to.
    };
} // namespace PythonCoverage
