
#pragma once

#include <AzCore/Component/Component.h>
#include <O3DEMaterialEditor/O3DEMaterialEditorBus.h>

namespace AtomToolsFramework
{
    class AtomToolsAssetBrowserInteractions;
    class AtomToolsDocumentSystem;
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
        void SetupAssetBrowserInteractions();

        const AZStd::string m_targetName;
        const AZ::Crc32 m_toolId = {};

        O3DEMaterialEditor::O3DEMaterialEditorRequests::NotifyRegisterViewsEvent::Handler m_notifyRegisterViewsEventHandler;

        AZStd::unique_ptr<AtomToolsFramework::AtomToolsDocumentSystem> m_documentSystem;
        AZStd::unique_ptr<AtomToolsFramework::AtomToolsAssetBrowserInteractions> m_assetBrowserInteractions;
    };
} // namespace MaterialEditor
