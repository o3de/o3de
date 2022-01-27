
#pragma once

#include <MotionMatchingSystemComponent.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace MotionMatching
{
    /// System component for MotionMatching editor
    class MotionMatchingEditorSystemComponent
        : public MotionMatchingSystemComponent
        , private AzToolsFramework::EditorEvents::Bus::Handler
    {
        using BaseSystemComponent = MotionMatchingSystemComponent;
    public:
        AZ_COMPONENT(MotionMatchingEditorSystemComponent, "{a43957d3-5a2d-4c29-873d-7daacc357722}", BaseSystemComponent);
        static void Reflect(AZ::ReflectContext* context);

        MotionMatchingEditorSystemComponent();
        ~MotionMatchingEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
    };
} // namespace MotionMatching
