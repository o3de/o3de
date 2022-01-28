
#pragma once

#include <AzCore/Component/Component.h>
#include <O3DEMaterialEditor/O3DEMaterialEditorBus.h>

namespace Blast
{
    /// System component for MaterialEditor editor
    class BlastMaterialEditorSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(BlastMaterialEditorSystemComponent, "{5F9C963B-3E45-46D7-853D-C445524B1C23}");
        static void Reflect(AZ::ReflectContext* context);

        BlastMaterialEditorSystemComponent();
        ~BlastMaterialEditorSystemComponent();

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
} // namespace Blast
