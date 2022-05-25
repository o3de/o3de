
#include <RecastNavigationSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace RecastNavigation
{
    void RecastNavigationSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<RecastNavigationSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<RecastNavigationSystemComponent>("RecastNavigation", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void RecastNavigationSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("RecastNavigationService"));
    }

    void RecastNavigationSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("RecastNavigationService"));
    }

    void RecastNavigationSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void RecastNavigationSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    RecastNavigationSystemComponent::RecastNavigationSystemComponent()
    {
        if (RecastNavigationInterface::Get() == nullptr)
        {
            RecastNavigationInterface::Register(this);
        }
    }

    RecastNavigationSystemComponent::~RecastNavigationSystemComponent()
    {
        if (RecastNavigationInterface::Get() == this)
        {
            RecastNavigationInterface::Unregister(this);
        }
    }

    void RecastNavigationSystemComponent::Init()
    {
    }

    void RecastNavigationSystemComponent::Activate()
    {
        RecastNavigationRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void RecastNavigationSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        RecastNavigationRequestBus::Handler::BusDisconnect();
    }

    void RecastNavigationSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }

} // namespace RecastNavigation
