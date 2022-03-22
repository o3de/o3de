#include <${SanitizedCppName}Component.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace ${SanitizedCppName}
{
    void ${SanitizedCppName}Component::Activate()
    {
        ${SanitizedCppName}RequestBus::Handler::BusConnect(GetEntityId());
    }

    void ${SanitizedCppName}Component::Deactivate()
    {
        ${SanitizedCppName}RequestBus::Handler::BusDisconnect(GetEntityId());
    }

    void ${SanitizedCppName}Component::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<${SanitizedCppName}Component, AZ::Component>()
                ->Version(1)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<${SanitizedCppName}Component>("${SanitizedCppName}Component", "[Description of functionality provided by this component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "ComponentCategory")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/ComponentPlaceholder.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<${SanitizedCppName}Component>("${SanitizedCppName} Component Group")
                ->Attribute(AZ::Script::Attributes::Category, "${SanitizedCppName} Gem Group")
                ;
        }
    }

    void ${SanitizedCppName}Component::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("${SanitizedCppName}ComponentService"));
    }

    void ${SanitizedCppName}Component::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
    }

    void ${SanitizedCppName}Component::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void ${SanitizedCppName}Component::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }
} // namespace ${SanitizedCppName}
