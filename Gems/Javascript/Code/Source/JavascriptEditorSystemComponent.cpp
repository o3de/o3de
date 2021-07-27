
#include <AzCore/Serialization/SerializeContext.h>
#include <JavascriptEditorSystemComponent.h>

namespace Javascript
{
    void JavascriptEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<JavascriptEditorSystemComponent, JavascriptSystemComponent>()
                ->Version(0);
        }
    }

    JavascriptEditorSystemComponent::JavascriptEditorSystemComponent() = default;

    JavascriptEditorSystemComponent::~JavascriptEditorSystemComponent() = default;

    void JavascriptEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("JavascriptEditorService"));
    }

    void JavascriptEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("JavascriptEditorService"));
    }

    void JavascriptEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void JavascriptEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void JavascriptEditorSystemComponent::Activate()
    {
        JavascriptSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void JavascriptEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        JavascriptSystemComponent::Deactivate();
    }

} // namespace Javascript
