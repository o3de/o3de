
#pragma once

#include <AzCore/Component/Component.h>
#include <O3DEMaterialEditor/O3DEMaterialEditorBus.h>

namespace PhysX
{
    /// System component for MaterialEditor editor
    class PhysXMaterialEditorSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(PhysXMaterialEditorSystemComponent, "{6710C447-ED80-48BF-887D-89DEF461AFB5}");
        static void Reflect(AZ::ReflectContext* context);

        PhysXMaterialEditorSystemComponent();
        ~PhysXMaterialEditorSystemComponent();

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
    };
} // namespace PhysX
