
#include <AzCore/Serialization/SerializeContext.h>
#include <RemoteToolsEditorSystemComponent.h>

namespace RemoteTools
{
    void RemoteToolsEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RemoteToolsEditorSystemComponent, RemoteToolsSystemComponent>()
                ->Version(0);
        }
    }

    RemoteToolsEditorSystemComponent::RemoteToolsEditorSystemComponent() = default;

    RemoteToolsEditorSystemComponent::~RemoteToolsEditorSystemComponent() = default;

    void RemoteToolsEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("RemoteToolsEditorService"));
    }

    void RemoteToolsEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("RemoteToolsEditorService"));
    }

    void RemoteToolsEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void RemoteToolsEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void RemoteToolsEditorSystemComponent::Activate()
    {
        RemoteToolsSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void RemoteToolsEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        RemoteToolsSystemComponent::Deactivate();
    }

} // namespace RemoteTools
