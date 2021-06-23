
#pragma once

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace PrefabDependencyViewer
{
    /// System component for PrefabDependencyViewer editor
    class PrefabDependencyViewerEditorSystemComponent
        : public AZ::Component
        , private AzToolsFramework::EditorEvents::Bus::Handler
    {
    public:
        AZ_COMPONENT(PrefabDependencyViewerEditorSystemComponent, "{1eb2c3bf-ef82-4bb4-82a0-4b6bd2d9895c}");
        static void Reflect(AZ::ReflectContext* context);

        PrefabDependencyViewerEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
    };
} // namespace PrefabDependencyViewer
