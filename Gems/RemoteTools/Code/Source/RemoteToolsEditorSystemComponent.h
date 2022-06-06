
#pragma once

#include <RemoteToolsSystemComponent.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace RemoteTools
{
    /// System component for RemoteTools editor
    class RemoteToolsEditorSystemComponent
        : public RemoteToolsSystemComponent
        , private AzToolsFramework::EditorEvents::Bus::Handler
    {
        using BaseSystemComponent = RemoteToolsSystemComponent;
    public:
        AZ_COMPONENT(RemoteToolsEditorSystemComponent, "{66a3f96b-677e-47fb-8c3a-17fd4c9b7bbd}", BaseSystemComponent);
        static void Reflect(AZ::ReflectContext* context);

        RemoteToolsEditorSystemComponent();
        ~RemoteToolsEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
    };
} // namespace RemoteTools
