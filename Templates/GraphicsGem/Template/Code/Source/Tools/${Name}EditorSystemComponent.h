
#pragma once

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Clients/${Name}SystemComponent.h>

namespace ${Name}
{
    /// System component for ${Name} editor
    class ${Name}EditorSystemComponent
        : public ${Name}SystemComponent
        , protected AzToolsFramework::EditorEvents::Bus::Handler
    {
        using BaseSystemComponent = ${Name}SystemComponent;
    public:
        AZ_COMPONENT_DECL(${Name}EditorSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        ${Name}EditorSystemComponent();
        ~${Name}EditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
    };
} // namespace ${Name}
