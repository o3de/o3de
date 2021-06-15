
#include <PythonCoverageSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace PythonCoverage
{
    void PythonCoverageSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<PythonCoverageSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<PythonCoverageSystemComponent>("PythonCoverage", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void PythonCoverageSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("PythonCoverageService"));
    }

    void PythonCoverageSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("PythonCoverageService"));
    }

    void PythonCoverageSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void PythonCoverageSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void PythonCoverageSystemComponent::Init()
    {
    }

    void PythonCoverageSystemComponent::Activate()
    {
        PythonCoverageRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void PythonCoverageSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        PythonCoverageRequestBus::Handler::BusDisconnect();
    }

    void PythonCoverageSystemComponent::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
    {

    }

} // namespace PythonCoverage
