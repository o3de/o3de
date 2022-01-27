
#include <MotionMatchingSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <BlendTreeMotionMatchNode.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <Integration/EMotionFXBus.h>

namespace MotionMatching
{
    void MotionMatchingSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MotionMatchingSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<MotionMatchingSystemComponent>("MotionMatching", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        EMotionFX::MotionMatching::BlendTreeMotionMatchNode::Reflect(context);
    }

    void MotionMatchingSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MotionMatchingService"));
    }

    void MotionMatchingSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MotionMatchingService"));
    }

    void MotionMatchingSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("EMotionFXAnimationService", 0x3f8a6369));
    }

    void MotionMatchingSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    MotionMatchingSystemComponent::MotionMatchingSystemComponent()
    {
        if (MotionMatchingInterface::Get() == nullptr)
        {
            MotionMatchingInterface::Register(this);
        }
    }

    MotionMatchingSystemComponent::~MotionMatchingSystemComponent()
    {
        if (MotionMatchingInterface::Get() == this)
        {
            MotionMatchingInterface::Unregister(this);
        }
    }

    void MotionMatchingSystemComponent::Init()
    {
    }

    void MotionMatchingSystemComponent::Activate()
    {
        MotionMatchingRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();

        // Register the motion matching anim graph node
        EMotionFX::AnimGraphObject* motionMatchNodeObject = EMotionFX::AnimGraphObjectFactory::Create(azrtti_typeid<EMotionFX::MotionMatching::BlendTreeMotionMatchNode>());
        auto motionMatchNode = azdynamic_cast<EMotionFX::MotionMatching::BlendTreeMotionMatchNode*>(motionMatchNodeObject);
        if (motionMatchNode)
        {
            EMotionFX::Integration::EMotionFXRequestBus::Broadcast(&EMotionFX::Integration::EMotionFXRequests::RegisterAnimGraphObjectType, motionMatchNode);
            delete motionMatchNode;
        }
    }

    void MotionMatchingSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        MotionMatchingRequestBus::Handler::BusDisconnect();
    }

    void MotionMatchingSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }

} // namespace MotionMatching
