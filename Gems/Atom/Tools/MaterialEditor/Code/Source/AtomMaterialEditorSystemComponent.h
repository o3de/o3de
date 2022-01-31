
#pragma once

#include <AzCore/Component/Component.h>
#include <O3DEMaterialEditor/O3DEMaterialEditorBus.h>

namespace MaterialEditor
{
    class MaterialEditorBrowserInteractions;
}

namespace AtomMaterialEditor
{
    /// System component for MaterialEditor editor
    class AtomMaterialEditorSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(AtomMaterialEditorSystemComponent, "{CCEC0F13-77C5-4BF9-A325-AA580F1B5354}");
        static void Reflect(AZ::ReflectContext* context);

        AtomMaterialEditorSystemComponent();
        ~AtomMaterialEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        void RegisterAtomWindow();

        O3DEMaterialEditor::O3DEMaterialEditorRequests::NotifyRegisterViewsEvent::Handler m_notifyRegisterViewsEventHandler;

        AZStd::unique_ptr<MaterialEditor::MaterialEditorBrowserInteractions> m_materialEditorBrowserInteractions;
    };
} // namespace MaterialEditor
