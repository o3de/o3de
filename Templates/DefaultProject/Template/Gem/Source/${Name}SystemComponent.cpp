
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include "${Name}SystemComponent.h"

namespace ${Name}
{
    void ${Name}SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<${Name}SystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<${Name}SystemComponent>("${Name}", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void ${Name}SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("${Name}Service"));
    }

    void ${Name}SystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("${Name}Service"));
    }

    void ${Name}SystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void ${Name}SystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    ${Name}SystemComponent::${Name}SystemComponent()
    {
        if (${Name}Interface::Get() == nullptr)
        {
            ${Name}Interface::Register(this);
        }
    }

    ${Name}SystemComponent::~${Name}SystemComponent()
    {
        if (${Name}Interface::Get() == this)
        {
            ${Name}Interface::Unregister(this);
        }
    }

    void ${Name}SystemComponent::Init()
    {
    }

    void ${Name}SystemComponent::Activate()
    {
        ${Name}RequestBus::Handler::BusConnect();
    }

    void ${Name}SystemComponent::Deactivate()
    {
        ${Name}RequestBus::Handler::BusDisconnect();
    }
}
