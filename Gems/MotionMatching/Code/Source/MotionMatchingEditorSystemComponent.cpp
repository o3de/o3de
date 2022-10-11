/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/MotionSystem.h>
#include <EMotionFX/Source/TransformData.h>
#include <MotionMatchingEditorSystemComponent.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/ViewportPluginBus.h>

namespace EMotionFX::MotionMatching
{
    void MotionMatchingEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MotionMatchingEditorSystemComponent, MotionMatchingSystemComponent>()
                ->Version(0);
        }
    }

    MotionMatchingEditorSystemComponent::MotionMatchingEditorSystemComponent()
    {
        if (MotionMatchingEditorInterface::Get() == nullptr)
        {
            MotionMatchingEditorInterface::Register(this);
        }
    }

    MotionMatchingEditorSystemComponent::~MotionMatchingEditorSystemComponent()
    {
        if (MotionMatchingEditorInterface::Get() == this)
        {
            MotionMatchingEditorInterface::Unregister(this);
        }
    }

    void MotionMatchingEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("MotionMatchingEditorService"));
    }

    void MotionMatchingEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("MotionMatchingEditorService"));
    }

    void MotionMatchingEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void MotionMatchingEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void MotionMatchingEditorSystemComponent::Activate()
    {
        MotionMatchingEditorRequestBus::Handler::BusConnect();
        MotionMatchingSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void MotionMatchingEditorSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        MotionMatchingSystemComponent::Deactivate();
        MotionMatchingEditorRequestBus::Handler::BusDisconnect();
    }

    void MotionMatchingEditorSystemComponent::SetDebugDrawFeatureSchema(FeatureSchema* featureSchema)
    {
        m_debugVisFeatureSchema = featureSchema;
        m_data.reset();
    }

    FeatureSchema* MotionMatchingEditorSystemComponent::GetDebugDrawFeatureSchema() const
    {
        return m_debugVisFeatureSchema;
    }

    void MotionMatchingEditorSystemComponent::DebugDrawFrameFeatures(AzFramework::DebugDisplayRequests* debugDisplay)
    {
        // Skip directly in case there is no feature schema to be visualized.
        if (!m_debugVisFeatureSchema)
        {
            return;
        }

        // Find currently playing motion instance.
        MotionInstance* motionInstance = nullptr;
        {
            const ActorManager* actorManager = GetEMotionFX().GetActorManager();
            const size_t numActorInstances = actorManager->GetNumActorInstances();
            for (size_t i = 0; i < numActorInstances; ++i)
            {
                ActorInstance* actorInstance = actorManager->GetActorInstance(i);
                if (actorInstance->GetMotionSystem() &&
                    actorInstance->GetMotionSystem()->GetIsPlaying())
                {
                    motionInstance = actorInstance->GetMotionSystem()->GetMotionInstance(0);
                    break;
                }
            }
        }

        // If there is a motion playing on an actor instance, draw the features given by the feature schema for the current playtime.
        if (motionInstance)
        {
            Motion* motion = motionInstance->GetMotion();
            ActorInstance* actorInstance = motionInstance->GetActorInstance();
            const FeatureSchema& featureSchema = *m_debugVisFeatureSchema;
            const Pose& currentPose = *actorInstance->GetTransformData()->GetCurrentPose();

            // Re-create the motion matching features in case the motion changed.
            if (!m_data ||
                m_lastMotionInstance != motionInstance)
            {
                m_data.reset(aznew MotionMatchingData(featureSchema));

                MotionMatchingData::InitSettings settings;
                settings.m_actorInstance = actorInstance;
                settings.m_motionList.emplace_back(motionInstance->GetMotion());
                m_data->Init(settings);
            }

            // Find the frame index in the frame database that belongs to the currently used pose.
            const size_t currentFrame = m_data->GetFrameDatabase().FindFrameIndex(motion, motionInstance->GetCurrentTime());
            if (currentFrame != InvalidIndex) // Render the feature debug visualizations for the current frame.
            {
                const AZStd::string currentFrameText = AZStd::string::format("Frame = %zu", currentFrame);
                debugDisplay->Draw2dTextLabel(10, 10, 1.0f, currentFrameText.c_str());

                for (Feature* feature : featureSchema.GetFeatures())
                {
                    if (feature->GetDebugDrawEnabled())
                    {
                        feature->DebugDraw(*debugDisplay, currentPose, m_data->GetFeatureMatrix(), m_data->GetFeatureTransformer(), currentFrame);
                    }
                }
            }
        }
        else
        {
            // Release the motion matching data in case the motion is not playing anymore.
            m_data.reset();
            motionInstance = nullptr;
        }
        m_lastMotionInstance = motionInstance;
    }

    void MotionMatchingEditorSystemComponent::DebugDraw(AZ::s32 debugDisplayId)
    {
        AZ_PROFILE_SCOPE(Animation, "MotionMatchingEditorSystemComponent::DebugDraw");

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
            DebugDrawFrameFeatures(debugDisplay);
            debugDisplay->SetState(prevState);
        }
    }

    void MotionMatchingEditorSystemComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        // Draw the debug visualizations to the Animation Editor as well as the LY Editor viewport.
        AZ::s32 animationEditorViewportId = -1;
        EMStudio::ViewportPluginRequestBus::BroadcastResult(animationEditorViewportId, &EMStudio::ViewportPluginRequestBus::Events::GetViewportId);

        // Base system component
        MotionMatchingSystemComponent::OnTick(deltaTime, time);
        MotionMatchingSystemComponent::DebugDraw(animationEditorViewportId);

        // Editor system component
        MotionMatchingEditorSystemComponent::DebugDraw(AzFramework::g_defaultSceneEntityDebugDisplayId);
        MotionMatchingEditorSystemComponent::DebugDraw(animationEditorViewportId);
    }
} // namespace EMotionFX::MotionMatching
