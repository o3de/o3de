
#pragma once

#include <ToolingConnectionSystemComponent.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace ToolingConnection
{
    /// System component for ToolingConnection editor
    class ToolingConnectionEditorSystemComponent
        : public ToolingConnectionSystemComponent
        , private AzToolsFramework::EditorEvents::Bus::Handler
    {
        using BaseSystemComponent = ToolingConnectionSystemComponent;
    public:
        AZ_COMPONENT(ToolingConnectionEditorSystemComponent, "{66a3f96b-677e-47fb-8c3a-17fd4c9b7bbd}", BaseSystemComponent);
        static void Reflect(AZ::ReflectContext* context);

        ToolingConnectionEditorSystemComponent();
        ~ToolingConnectionEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
    };
} // namespace ToolingConnection
