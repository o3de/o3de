
#pragma once

#include <MaterialEditorSystemComponent.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace MaterialEditor
{
    /// System component for MaterialEditor editor
    class MaterialEditorEditorSystemComponent
        : public MaterialEditorSystemComponent
        , private AzToolsFramework::EditorEvents::Bus::Handler
    {
        using BaseSystemComponent = MaterialEditorSystemComponent;
    public:
        AZ_COMPONENT(MaterialEditorEditorSystemComponent, "{fd8b8d15-88b6-4240-89ca-d52b5c21c3be}", BaseSystemComponent);
        static void Reflect(AZ::ReflectContext* context);

        MaterialEditorEditorSystemComponent();
        ~MaterialEditorEditorSystemComponent();

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
