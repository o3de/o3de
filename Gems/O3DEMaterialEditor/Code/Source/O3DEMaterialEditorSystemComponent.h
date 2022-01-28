
#pragma once

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace O3DEMaterialEditor
{
    /// System component for O3DEMaterialEditor editor
    class O3DEMaterialEditorSystemComponent
        : public AZ::Component
        , private AzToolsFramework::EditorEvents::Bus::Handler
    {
    public:
        AZ_COMPONENT(O3DEMaterialEditorSystemComponent, "{fd8b8d15-88b6-4240-89ca-d52b5c21c3be}");
        static void Reflect(AZ::ReflectContext* context);

        O3DEMaterialEditorSystemComponent();
        ~O3DEMaterialEditorSystemComponent();

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
} // namespace O3DEMaterialEditor
