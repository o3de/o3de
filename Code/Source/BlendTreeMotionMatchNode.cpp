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

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/Motion.h>
#include <BlendTreeMotionMatchNode.h>
#include <MotionMatchSystem.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Recorder.h>
#include <EMotionFX/Source/TransformData.h>

namespace EMotionFX
{
    namespace MotionMatching
    {
        AZ_CLASS_ALLOCATOR_IMPL(BlendTreeMotionMatchNode, AnimGraphAllocator, 0)
        AZ_CLASS_ALLOCATOR_IMPL(BlendTreeMotionMatchNode::UniqueData, AnimGraphObjectUniqueDataAllocator, 0)

        BlendTreeMotionMatchNode::BlendTreeMotionMatchNode()
            : AnimGraphNode()
        {
            // Setup the input ports.
            InitInputPorts(1);
            SetupInputPort("Goal Pos", INPUTPORT_TARGETPOS, MCore::AttributeVector3::TYPE_ID, PORTID_INPUT_TARGETPOS);

            // Setup the output ports.
            InitOutputPorts(1);
            SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
        }

        BlendTreeMotionMatchNode::~BlendTreeMotionMatchNode()
        {
        }

        bool BlendTreeMotionMatchNode::InitAfterLoading(AnimGraph* animGraph)
        {
            if (!AnimGraphNode::InitAfterLoading(animGraph))
            {
                return false;
            }

            InitInternalAttributesForAllInstances();

            Reinit();
            return true;
        }

        const char* BlendTreeMotionMatchNode::GetPaletteName() const
        {
            return "Motion Matching";
        }

        AnimGraphObject::ECategory BlendTreeMotionMatchNode::GetPaletteCategory() const
        {
            return AnimGraphObject::CATEGORY_SOURCES;
        }

        void BlendTreeMotionMatchNode::UniqueData::Update()
        {
            AZ_PROFILE_SCOPE(Animation, "BlendTreeMotionMatchNode::UniqueData::Update");

            auto animGraphNode = azdynamic_cast<BlendTreeMotionMatchNode*>(m_object);
            AZ_Assert(animGraphNode, "Unique data linked to incorrect node type.");

            ActorInstance* actorInstance = m_animGraphInstance->GetActorInstance();

            // Clear existing data.
            delete m_behaviorInstance;
            delete m_behavior;

            m_behavior = aznew MotionMatching::LocomotionBehavior();
            m_behaviorInstance = aznew MotionMatching::BehaviorInstance();

            MotionSet* motionSet = m_animGraphInstance->GetMotionSet();
            if (!motionSet)
            {
                SetHasError(true);
                return;
            }

            //---------------------------------
            AZ::Debug::Timer timer;
            timer.Stamp();

            // Build a list of motions we want to import the frames from.
            AZ_Printf("EMotionFX", "[MotionMatching] Importing frames...");
            MotionMatching::Behavior::InitSettings behaviorSettings;
            behaviorSettings.m_actorInstance = actorInstance;
            behaviorSettings.m_frameImportSettings.m_sampleRate = animGraphNode->m_sampleRate;
            behaviorSettings.m_importMirrored = animGraphNode->m_mirror;
            behaviorSettings.m_maxKdTreeDepth = animGraphNode->m_maxKdTreeDepth;
            behaviorSettings.m_minFramesPerKdTreeNode = animGraphNode->m_minFramesPerKdTreeNode;
            behaviorSettings.m_motionList.reserve(animGraphNode->m_motionIds.size());
            for (const AZStd::string& id : animGraphNode->m_motionIds)
            {
                Motion* motion = motionSet->RecursiveFindMotionById(id);
                if (motion)
                {
                    behaviorSettings.m_motionList.emplace_back(motion);
                }
                else
                {
                    AZ_Warning("EMotionFX", false, "Failed to get motion for motionset entry id '%s'", id.c_str());
                }
            }

            // Initialize the behavior (slow).
            AZ_Printf("EMotionFX", "[MotionMatching] Initializing behavior...");
            if (!m_behavior->Init(behaviorSettings))
            {
                AZ_Warning("EMotionFX", false, "Failed to initialize the motion matching behavior for anim graph node '%s'!", animGraphNode->GetName());
                SetHasError(true);
                return;
            }

            // Initialize the behavior instance.
            AZ_Printf("EMotionFX", "[MotionMatching] Initializing behavior instance...");
            MotionMatching::BehaviorInstance::InitSettings initSettings;
            initSettings.m_actorInstance = actorInstance;
            initSettings.m_behavior = m_behavior;
            m_behaviorInstance->Init(initSettings);

            const float initTime = timer.GetDeltaTimeInSeconds();
            const size_t memUsage = m_behavior->GetData().CalcMemoryUsageInBytes();
            AZ_Printf("EMotionFX", "[MotionMatching] Finished in %.2f seconds (mem usage=%d bytes or %.2f mb)", initTime, memUsage, memUsage / (float)(1024 * 1024));
            //---------------------------------

            SetHasError(false);
        }

        void BlendTreeMotionMatchNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
        {
            AZ_PROFILE_SCOPE(Animation, "BlendTreeMotionMatchNode::Update");

            m_timer.Stamp();
            
            UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
            UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);
            uniqueData->Clear();
            if (uniqueData->GetHasError())
            {
                m_updateTimeInMs = 0.0f;
                m_postUpdateTimeInMs = 0.0f;
                m_outputTimeInMs = 0.0f;
                return;
            }

            AZ::Vector3 targetPos = AZ::Vector3::CreateZero();
            TryGetInputVector3(animGraphInstance, INPUTPORT_TARGETPOS, targetPos);

            MotionMatching::BehaviorInstance* behaviorInstance = uniqueData->m_behaviorInstance;
            behaviorInstance->Update(timePassedInSeconds);

            // Register the current actor instance position to the history data of the spline.
            uniqueData->m_behavior->BuildControlSpline(uniqueData->m_behaviorInstance, m_controlSplineMode, targetPos, behaviorInstance->GetTrajectoryHistory(), timePassedInSeconds, m_pathRadius, m_pathSpeed);

            // set the current time to the new calculated time
            uniqueData->ClearInheritFlags();
            uniqueData->SetPreSyncTime(behaviorInstance->GetMotionInstance()->GetCurrentTime());
            uniqueData->SetCurrentPlayTime(behaviorInstance->GetNewMotionTime());

            if (uniqueData->GetPreSyncTime() > uniqueData->GetCurrentPlayTime())
            {
                uniqueData->SetPreSyncTime(uniqueData->GetCurrentPlayTime());
            }
                
            //AZ_Printf("EMotionFX", "%f, %f    =    %f", uniqueData->GetPreSyncTime(), uniqueData->GetCurrentPlayTime(), uniqueData->GetCurrentPlayTime() - uniqueData->GetPreSyncTime());
            
            m_updateTimeInMs = m_timer.GetDeltaTimeInSeconds() * 1000.0f;
        }

        void BlendTreeMotionMatchNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
        {
            AZ_PROFILE_SCOPE(Animation, "BlendTreeMotionMatchNode::PostUpdate");

            AZ_UNUSED(animGraphInstance);
            AZ_UNUSED(timePassedInSeconds);
            m_timer.Stamp();

            for (AZ::u32 i = 0; i < GetNumConnections(); ++i)
            {
                AnimGraphNode* node = GetConnection(i)->GetSourceNode();
                node->PerformPostUpdate(animGraphInstance, timePassedInSeconds);
            }

            UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
            MotionMatching::BehaviorInstance* behaviorInstance = uniqueData->m_behaviorInstance;

            RequestRefDatas(animGraphInstance);
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();

            if (uniqueData->GetHasError())
            {
                return;
            }

            MotionInstance* motionInstance = behaviorInstance->GetMotionInstance();
            motionInstance->UpdateByTimeValues(uniqueData->GetPreSyncTime(), uniqueData->GetCurrentPlayTime(), &data->GetEventBuffer());
            uniqueData->SetCurrentPlayTime(motionInstance->GetCurrentTime());
            data->GetEventBuffer().UpdateEmitters(this);

            Transform trajectoryDelta = behaviorInstance->GetMotionExtractionDelta();
            behaviorInstance->GetMotionInstance()->ExtractMotion(trajectoryDelta);
            data->SetTrajectoryDelta(trajectoryDelta);
            data->SetTrajectoryDeltaMirrored(trajectoryDelta); // TODO: use a real mirrored version here.

            m_postUpdateTimeInMs = m_timer.GetDeltaTimeInSeconds() * 1000.0f;
        }

        void BlendTreeMotionMatchNode::Output(AnimGraphInstance* animGraphInstance)
        {
            AZ_PROFILE_SCOPE(Animation, "BlendTreeMotionMatchNode::Output");

            AZ_UNUSED(animGraphInstance);
            m_timer.Stamp();

            AnimGraphPose* outputPose;

            // Initialize to bind pose.
            ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            outputPose->InitFromBindPose(actorInstance);

            if (m_disabled)
            {
                return;
            }

            UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
            if (GetEMotionFX().GetIsInEditorMode())
            {
                SetHasError(uniqueData, uniqueData->GetHasError());
            }

            if (uniqueData->GetHasError())
            {
                return;
            }

            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_TARGETPOS));

            Pose& outTransformPose = outputPose->GetPose();

            MotionMatching::LocomotionBehavior* behavior = uniqueData->m_behavior;
            MotionMatching::LocomotionBehavior::FactorWeights& factors = behavior->GetFactorWeights();
            factors.m_footPositionFactor = m_footPositionFactor;
            factors.m_footVelocityFactor = m_footVelocityFactor;
            factors.m_rootFutureFactor = m_rootFutureFactor;
            factors.m_rootPastFactor = m_rootPastFactor;
            factors.m_differentMotionFactor = m_differentMotionFactor;
            behavior->SetFactorWeights(factors);

            MotionMatching::BehaviorInstance* behaviorInstance = uniqueData->m_behaviorInstance;
            behaviorInstance->SetLowestCostSearchFrequency(m_lowestCostSearchFrequency);
            behaviorInstance->Output(outTransformPose);

            // Render some debug lines.
            if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
            {
                uniqueData->m_behaviorInstance->DebugDraw();
            }

            // Performance metrics
            m_outputTimeInMs = m_timer.GetDeltaTimeInSeconds() * 1000.0f;
            {
                //AZ_Printf("MotionMatch", "Update = %.2f, PostUpdate = %.2f, Output = %.2f", m_updateTime, m_postUpdateTime, m_outputTime);
                ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushPerformanceHistogramValue, "Update", m_updateTimeInMs);
                ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushPerformanceHistogramValue, "Post Update", m_postUpdateTimeInMs);
                ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushPerformanceHistogramValue, "Output", m_outputTimeInMs);
            }
        }

        void BlendTreeMotionMatchNode::Reflect(AZ::ReflectContext* context)
        {
            // Reflect the motion matching system first.
            MotionMatching::MotionMatchSystem::Reflect(context);

            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            serializeContext->Class<BlendTreeMotionMatchNode, AnimGraphNode>()
                ->Version(8)
                ->Field("motionIds", &BlendTreeMotionMatchNode::m_motionIds)
                ->Field("maxKdTreeDepth", &BlendTreeMotionMatchNode::m_maxKdTreeDepth)
                ->Field("minFramesPerKdTreeNode", &BlendTreeMotionMatchNode::m_minFramesPerKdTreeNode)
                ->Field("footPositionFactor", &BlendTreeMotionMatchNode::m_footPositionFactor)
                ->Field("footVelocity", &BlendTreeMotionMatchNode::m_footVelocityFactor)
                ->Field("rootFutureFactor", &BlendTreeMotionMatchNode::m_rootFutureFactor)
                ->Field("rootPastFactor", &BlendTreeMotionMatchNode::m_rootPastFactor)
                ->Field("differentMotionFactor", &BlendTreeMotionMatchNode::m_differentMotionFactor)
                ->Field("sampleRate", &BlendTreeMotionMatchNode::m_sampleRate)
                ->Field("lowestCostSearchFrequency", &BlendTreeMotionMatchNode::m_lowestCostSearchFrequency)
                ->Field("mirror", &BlendTreeMotionMatchNode::m_mirror)
                ->Field("controlSplineMode", &BlendTreeMotionMatchNode::m_controlSplineMode)
                ->Field("pathRadius", &BlendTreeMotionMatchNode::m_pathRadius)
                ->Field("pathSpeed", &BlendTreeMotionMatchNode::m_pathSpeed);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (!editContext)
            {
                return;
            }

            editContext->Class<BlendTreeMotionMatchNode>("Motion Matching Node", "Motion Matching Attributes")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ->DataElement(AZ_CRC("MotionSetMotionIds", 0x8695c0fa), &BlendTreeMotionMatchNode::m_motionIds, "Motions", "")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeMotionMatchNode::Reinit)
                ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::HideChildren)
                ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMotionMatchNode::m_maxKdTreeDepth, "Max kdTree depth", "The maximum number of hierarchy levels in the kdTree.")
                ->Attribute(AZ::Edit::Attributes::Min, 1)
                ->Attribute(AZ::Edit::Attributes::Max, 20)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeMotionMatchNode::Reinit)
                ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMotionMatchNode::m_minFramesPerKdTreeNode, "Min kdTree node size", "The minimum number of frames to store per kdTree node.")
                ->Attribute(AZ::Edit::Attributes::Min, 1)
                ->Attribute(AZ::Edit::Attributes::Max, 100000)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeMotionMatchNode::Reinit)
                ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMotionMatchNode::m_footPositionFactor, "Foot Position Factor", "")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMotionMatchNode::m_footVelocityFactor, "Foot Velocity Factor", "")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMotionMatchNode::m_rootFutureFactor, "Root Future Factor", "")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMotionMatchNode::m_rootPastFactor, "Root Past Factor", "")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMotionMatchNode::m_differentMotionFactor, "Different Motion Factor", "")
                ->Attribute(AZ::Edit::Attributes::Min, 1.0f)
                ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMotionMatchNode::m_sampleRate, "Sample rate", "The motion frame data sampling frequency.")
                ->Attribute(AZ::Edit::Attributes::Min, 5)
                ->Attribute(AZ::Edit::Attributes::Max, 60)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeMotionMatchNode::Reinit)
                ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMotionMatchNode::m_mirror, "Add mirrored poses?", "")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeMotionMatchNode::Reinit)
                ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMotionMatchNode::m_pathRadius, "Path radius", "")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0001f)
                ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMotionMatchNode::m_pathSpeed, "Path speed", "")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0001f)
                ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMotionMatchNode::m_lowestCostSearchFrequency, "Search frequency", "Lowest cost search frequency in seconds. So a value of 0.1 means 10 times per second.")
                ->Attribute(AZ::Edit::Attributes::Min, 0.001f)
                ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Step, 0.05f)
                ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeMotionMatchNode::m_controlSplineMode, "Control Spline Mode", "The trajectory function/shape to use.")
                ->EnumAttribute(MODE_TARGETDRIVEN, "Target driven")
                ->EnumAttribute(MODE_ONE, "Mode one")
                ->EnumAttribute(MODE_TWO, "Mode two")
                ->EnumAttribute(MODE_THREE, "Mode three")
                ->EnumAttribute(MODE_FOUR, "Mode four");
        }
    } // namespace MotionMatching
} // namespace EMotionFX
