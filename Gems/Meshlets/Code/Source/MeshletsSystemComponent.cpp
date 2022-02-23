
#include <MeshletsSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace Meshlets
{
    void MeshletsSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MeshletsSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<MeshletsSystemComponent>("Meshlets", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void MeshletsSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MeshletsService"));
    }

    void MeshletsSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MeshletsService"));
    }

    void MeshletsSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void MeshletsSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    MeshletsSystemComponent::MeshletsSystemComponent()
    {
        if (MeshletsInterface::Get() == nullptr)
        {
            MeshletsInterface::Register(this);
        }
    }

    MeshletsSystemComponent::~MeshletsSystemComponent()
    {
        if (MeshletsInterface::Get() == this)
        {
            MeshletsInterface::Unregister(this);
        }
    }

    void MeshletsSystemComponent::Init()
    {
    }

    void MeshletsSystemComponent::Activate()
    {
        MeshletsRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void MeshletsSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        MeshletsRequestBus::Handler::BusDisconnect();
    }

    void MeshletsSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }

} // namespace Meshlets
