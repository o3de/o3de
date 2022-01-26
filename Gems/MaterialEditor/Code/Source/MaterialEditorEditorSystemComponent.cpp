
#include <AzCore/Serialization/SerializeContext.h>

#include <AzToolsFramework/API/ViewPaneOptions.h>

#include <MaterialEditorWidget.h>
#include <MaterialEditorEditorSystemComponent.h>

namespace MaterialEditor
{
    void MaterialEditorEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MaterialEditorEditorSystemComponent, MaterialEditorSystemComponent>()
                ->Version(0);
        }
    }

    MaterialEditorEditorSystemComponent::MaterialEditorEditorSystemComponent() = default;

    MaterialEditorEditorSystemComponent::~MaterialEditorEditorSystemComponent() = default;

    void MaterialEditorEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("MaterialEditorEditorService"));
    }

    void MaterialEditorEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("MaterialEditorEditorService"));
    }

    void MaterialEditorEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void MaterialEditorEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void MaterialEditorEditorSystemComponent::Activate()
    {
        MaterialEditorSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void MaterialEditorEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        MaterialEditorSystemComponent::Deactivate();
    }

    void MaterialEditorEditorSystemComponent::NotifyRegisterViews()
    {
        AzToolsFramework::ViewPaneOptions options;
        options.paneRect = QRect(100, 100, 500, 400);
        options.showOnToolsToolbar = true;
        options.toolbarIcon = ":/MaterialEditor/toolbar_icon.svg";

        // Register our custom widget as a dockable tool with the Editor under an Examples sub-menu
        AzToolsFramework::RegisterViewPane<MaterialEditorWidget>("MaterialEditor", "Examples", options);
    }

} // namespace MaterialEditor
