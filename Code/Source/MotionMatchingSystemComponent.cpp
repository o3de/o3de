/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <MotionMatchingSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/PoseDataFactory.h>
#include <Integration/EMotionFXBus.h>

#include <MotionMatchingConfig.h>
#include <MotionMatchingInstance.h>
#include <Feature.h>
#include <MotionMatchEventData.h>
#include <FeaturePosition.h>
#include <FeatureTrajectory.h>
#include <FeatureVelocity.h>
#include <PoseDataJointVelocities.h>
#include <BlendTreeMotionMatchNode.h>

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

        EMotionFX::MotionMatching::MotionMatchEventData::Reflect(context);

        EMotionFX::MotionMatching::MotionMatchingInstance::Reflect(context);
        EMotionFX::MotionMatching::MotionMatchingConfig::Reflect(context);

        EMotionFX::MotionMatching::FeatureSchema::Reflect(context);
        EMotionFX::MotionMatching::Feature::Reflect(context);
        EMotionFX::MotionMatching::FeaturePosition::Reflect(context);
        EMotionFX::MotionMatching::FeatureTrajectory::Reflect(context);
        EMotionFX::MotionMatching::FeatureVelocity::Reflect(context);

        EMotionFX::MotionMatching::PoseDataJointVelocities::Reflect(context);

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

        // Register the joint velocities pose data.
        EMotionFX::GetPoseDataFactory().AddPoseDataType(azrtti_typeid<EMotionFX::MotionMatching::PoseDataJointVelocities>());
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
