
#pragma once

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

#include <O3DEMaterialEditor/O3DEMaterialEditorBus.h>

namespace O3DEMaterialEditor
{
    using TabsInfo = AZStd::pair<AZStd::string, WidgetCreationFunc>;

    //! System component for O3DEMaterialEditor editor.
    class O3DEMaterialEditorSystemComponent
        : public AZ::Component
        , private O3DEMaterialEditorInterface::Registrar
        , private AzToolsFramework::EditorEvents::Bus::Handler
    {
    public:
        AZ_COMPONENT(O3DEMaterialEditorSystemComponent, "{fd8b8d15-88b6-4240-89ca-d52b5c21c3be}");
        static void Reflect(AZ::ReflectContext* context);

        O3DEMaterialEditorSystemComponent();
        ~O3DEMaterialEditorSystemComponent();

        const AZStd::vector<TabsInfo>& GetRegisteredTabs() const;

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // O3DEMaterialEditorInterface overrides ...
        void RegisterViewPane(const AZStd::string& name, const WidgetCreationFunc& widgetCreationFunc) override;

        // AzToolsFramework::EditorEventsBus overrides ...
        void NotifyRegisterViews() override;

        AZStd::vector<TabsInfo> m_registeredTabs;
    };

    //! Helper function to obtain O3DEMaterialEditorInterface as O3DEMaterialEditorSystemComponent
    O3DEMaterialEditorSystemComponent* GetO3DEMaterialEditorSystem();

} // namespace O3DEMaterialEditor
