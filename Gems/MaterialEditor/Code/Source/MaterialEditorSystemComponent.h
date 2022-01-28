
#pragma once

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace MaterialEditor
{
    /// System component for MaterialEditor editor
    class MaterialEditorSystemComponent
        : public AZ::Component
        , private AzToolsFramework::EditorEvents::Bus::Handler
    {
    public:
        AZ_COMPONENT(MaterialEditorSystemComponent, "{fd8b8d15-88b6-4240-89ca-d52b5c21c3be}");
        static void Reflect(AZ::ReflectContext* context);

        MaterialEditorSystemComponent();
        ~MaterialEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AzToolsFramework::EditorEventsBus overrides ...
        void NotifyRegisterViews() override;
    };
} // namespace MaterialEditor
