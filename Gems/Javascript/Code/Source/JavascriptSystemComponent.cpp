
#include <JavascriptSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace Javascript
{
    void JavascriptSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<JavascriptSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<JavascriptSystemComponent>("Javascript", "Run Javascript Code")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void JavascriptSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("JavascriptService"));
    }

    void JavascriptSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("JavascriptService"));
    }

    void JavascriptSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void JavascriptSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    JavascriptSystemComponent::JavascriptSystemComponent()
    {
        if (JavascriptInterface::Get() == nullptr)
        {
            JavascriptInterface::Register(this);
        }
    }

    JavascriptSystemComponent::~JavascriptSystemComponent()
    {
        if (JavascriptInterface::Get() == this)
        {
            JavascriptInterface::Unregister(this);
        }
    }

    void JavascriptSystemComponent::Init()
    {
    }

    void JavascriptSystemComponent::Activate()
    {
        JavascriptRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void JavascriptSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        JavascriptRequestBus::Handler::BusDisconnect();
    }

    void JavascriptSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }

} // namespace Javascript
