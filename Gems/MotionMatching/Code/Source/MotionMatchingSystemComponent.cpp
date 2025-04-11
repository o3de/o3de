/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/IConsole.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/PoseDataFactory.h>

#include <Integration/EMotionFXBus.h>

#include <BlendTreeMotionMatchNode.h>
#include <Feature.h>
#include <FeatureAngularVelocity.h>
#include <FeaturePosition.h>
#include <FeatureTrajectory.h>
#include <FeatureVelocity.h>
#include <EventData.h>
#include <MotionMatchingSystemComponent.h>
#include <PoseDataJointVelocities.h>

namespace EMotionFX::MotionMatching
{
    AZ_CVAR(bool, mm_debugDraw, true, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Global flag for motion matching debug drawing. Feature-wise debug drawing can be enabled or disabled in the anim graph itself.");

    AZ_CVAR(float, mm_debugDrawVelocityScale, 0.1f, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Scaling value used for velocity debug rendering.");

    AZ_CVAR(bool, mm_debugDrawQueryPose, false, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Draw the query skeletal pose used as input pose for the motion matching search.");

    AZ_CVAR(bool, mm_debugDrawQueryVelocities, false, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Draw the query joint velocities used as input for the motion matching search.");

    AZ_CVAR(bool, mm_useKdTree, true, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Use Kd-Tree to accelerate the motion matching search for the best next matching frame. "
        "Disabling it will heavily slow down performance and should only be done for debugging purposes");

    AZ_CVAR(bool, mm_multiThreadedInitialization, true, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Use multi-threading to initialize motion matching.");

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
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        EMotionFX::MotionMatching::DiscardFrameEventData::Reflect(context);
        EMotionFX::MotionMatching::TagEventData::Reflect(context);

        EMotionFX::MotionMatching::FeatureSchema::Reflect(context);
        EMotionFX::MotionMatching::Feature::Reflect(context);
        EMotionFX::MotionMatching::FeaturePosition::Reflect(context);
        EMotionFX::MotionMatching::FeatureTrajectory::Reflect(context);
        EMotionFX::MotionMatching::FeatureVelocity::Reflect(context);
        EMotionFX::MotionMatching::FeatureAngularVelocity::Reflect(context);

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

    void MotionMatchingSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("EMotionFXAnimationService"));
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

        // Register the motion matching anim graph node.
        {
            EMotionFX::AnimGraphObject* animGraphObject = EMotionFX::AnimGraphObjectFactory::Create(azrtti_typeid<EMotionFX::MotionMatching::BlendTreeMotionMatchNode>());
            auto animGraphNode = azdynamic_cast<EMotionFX::MotionMatching::BlendTreeMotionMatchNode*>(animGraphObject);
            if (animGraphNode)
            {
                EMotionFX::Integration::EMotionFXRequestBus::Broadcast(&EMotionFX::Integration::EMotionFXRequests::RegisterAnimGraphObjectType, animGraphNode);
            }
            delete animGraphObject;
        }

        // Register the joint velocities pose data.
        EMotionFX::GetPoseDataFactory().AddPoseDataType(azrtti_typeid<EMotionFX::MotionMatching::PoseDataJointVelocities>());
    }

    void MotionMatchingSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        MotionMatchingRequestBus::Handler::BusDisconnect();
    }

    void MotionMatchingSystemComponent::DebugDraw(AZ::s32 debugDisplayId)
    {
        AZ_PROFILE_SCOPE(Animation, "MotionMatchingSystemComponent::DebugDraw");

        if (debugDisplayId == -1)
        {
            return;
        }

        AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
        AzFramework::DebugDisplayRequestBus::Bind(debugDisplayBus, debugDisplayId);

        AzFramework::DebugDisplayRequests* debugDisplay = AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);
        if (debugDisplay)
        {
            const AZ::u32 prevState = debugDisplay->GetState();
            EMotionFX::MotionMatching::DebugDrawRequestBus::Broadcast(&EMotionFX::MotionMatching::DebugDrawRequests::DebugDraw, *debugDisplay);
            debugDisplay->SetState(prevState);
        }
    }

    void MotionMatchingSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        MotionMatchingSystemComponent::DebugDraw(AzFramework::g_defaultSceneEntityDebugDisplayId);
    }
} // namespace EMotionFX::MotionMatching
