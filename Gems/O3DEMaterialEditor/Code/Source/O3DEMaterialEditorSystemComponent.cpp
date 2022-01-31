
#include <AzCore/Serialization/SerializeContext.h>

#include <AzToolsFramework/API/ViewPaneOptions.h>

#include <O3DEMaterialEditorWidget.h>
#include <O3DEMaterialEditorSystemComponent.h>

namespace O3DEMaterialEditor
{
    void O3DEMaterialEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<O3DEMaterialEditorSystemComponent, AZ::Component>()
                ->Version(0);
        }
    }

    O3DEMaterialEditorSystemComponent::O3DEMaterialEditorSystemComponent() = default;

    O3DEMaterialEditorSystemComponent::~O3DEMaterialEditorSystemComponent() = default;

    const AZStd::vector<TabsInfo>& O3DEMaterialEditorSystemComponent::GetRegisteredTabs() const
    {
        return m_registeredTabs;
    }

    void O3DEMaterialEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("O3DEMaterialEditorService"));
    }

    void O3DEMaterialEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("O3DEMaterialEditorService"));
    }

    void O3DEMaterialEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void O3DEMaterialEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void O3DEMaterialEditorSystemComponent::Activate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void O3DEMaterialEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
    }

    void O3DEMaterialEditorSystemComponent::RegisterViewPane(const AZStd::string& name, const WidgetCreationFunc& widgetCreationFunc)
    {
        m_registeredTabs.emplace_back(name, widgetCreationFunc);
    }

    void O3DEMaterialEditorSystemComponent::NotifyRegisterViews()
    {
        // Notify all systems that want to register material editor views
        m_notifyRegisterViewsEvent.Signal();

        AzToolsFramework::ViewPaneOptions options;
        options.isPreview = true; // indicates it's a pre-release tool
        options.showInMenu = true;
        options.showOnToolsToolbar = true;
        options.toolbarIcon = ":/Menu/material_editor.svg"; // icon location 'Code/Framework/AzQtComponents/AzQtComponents/Images'

        // Register our custom widget as a dockable tool with the Editor under an Examples sub-menu
        AzToolsFramework::RegisterViewPane<O3DEMaterialEditorWidget>("O3DE Material Editor", "Tools", options);
    }

    O3DEMaterialEditorSystemComponent* GetO3DEMaterialEditorSystem()
    {
        return azdynamic_cast<O3DEMaterialEditorSystemComponent*>(O3DEMaterialEditorInterface::Get());
    }
} // namespace O3DEMaterialEditor
