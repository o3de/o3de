
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzFramework/API/ApplicationAPI.h>

namespace PythonCoverage
{
    /// System component for PythonCoverage editor
    class PythonCoverageEditorSystemComponent
        : public AZ::Component
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private AZ::EntitySystemBus::Handler
        , private AzFramework::ApplicationRequests::Bus::Handler
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

        // AzFramework::ApplicationRequests::Bus ...
        void NormalizePath(AZStd::string& /*path*/) override {}
        void NormalizePathKeepCase(AZStd::string& /*path*/) override{}
        void CalculateBranchTokenForEngineRoot(AZStd::string& /*token*/) const override {}
        void ExitMainLoop() override;
    };
} // namespace PythonCoverage
