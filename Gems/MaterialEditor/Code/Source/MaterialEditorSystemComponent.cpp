
#include <AzCore/Serialization/SerializeContext.h>

#include <AzToolsFramework/API/ViewPaneOptions.h>

#include <MaterialEditorWidget.h>
#include <MaterialEditorSystemComponent.h>

namespace MaterialEditor
{
    void MaterialEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MaterialEditorSystemComponent>()
                ->Version(0);
        }
    }

    MaterialEditorSystemComponent::MaterialEditorSystemComponent() = default;

    MaterialEditorSystemComponent::~MaterialEditorSystemComponent() = default;

    void MaterialEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MaterialEditorEditorService"));
    }

    void MaterialEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MaterialEditorEditorService"));
    }

    void MaterialEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void MaterialEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void MaterialEditorSystemComponent::Activate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void MaterialEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
    }

    void MaterialEditorSystemComponent::NotifyRegisterViews()
    {
        AzToolsFramework::ViewPaneOptions options;
        options.isPreview = true; // indicates it's a pre-release tool
        options.showInMenu = true;
        options.showOnToolsToolbar = true;
        options.toolbarIcon = ":/Menu/material_editor.svg";

        // Register our custom widget as a dockable tool with the Editor under an Examples sub-menu
        AzToolsFramework::RegisterViewPane<MaterialEditorWidget>("O3DE Material Editor", "Tools", options);
    }

} // namespace MaterialEditor
