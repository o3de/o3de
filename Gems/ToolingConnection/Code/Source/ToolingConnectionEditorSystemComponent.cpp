
#include <AzCore/Serialization/SerializeContext.h>
#include <ToolingConnectionEditorSystemComponent.h>

namespace ToolingConnection
{
    void ToolingConnectionEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ToolingConnectionEditorSystemComponent, ToolingConnectionSystemComponent>()
                ->Version(0);
        }
    }

    ToolingConnectionEditorSystemComponent::ToolingConnectionEditorSystemComponent() = default;

    ToolingConnectionEditorSystemComponent::~ToolingConnectionEditorSystemComponent() = default;

    void ToolingConnectionEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("ToolingConnectionEditorService"));
    }

    void ToolingConnectionEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("ToolingConnectionEditorService"));
    }

    void ToolingConnectionEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void ToolingConnectionEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void ToolingConnectionEditorSystemComponent::Activate()
    {
        ToolingConnectionSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void ToolingConnectionEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        ToolingConnectionSystemComponent::Deactivate();
    }

} // namespace ToolingConnection
