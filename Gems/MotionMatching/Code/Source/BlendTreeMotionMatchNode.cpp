/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
#include <FeatureSchemaDefault.h>
#include <MotionMatching/MotionMatchingBus.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Recorder.h>
#include <EMotionFX/Source/TransformData.h>

#include <FeaturePosition.h>

namespace EMotionFX::MotionMatching
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeMotionMatchNode, AnimGraphAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeMotionMatchNode::UniqueData, AnimGraphObjectUniqueDataAllocator)

    BlendTreeMotionMatchNode::BlendTreeMotionMatchNode()
        : AnimGraphNode()
    {
        // Setup the input ports.
        InitInputPorts(3);
        SetupInputPort("Goal Pos", INPUTPORT_TARGETPOS, MCore::AttributeVector3::TYPE_ID, PORTID_INPUT_TARGETPOS);
        SetupInputPort("Goal Facing Dir", INPUTPORT_TARGETFACINGDIR, MCore::AttributeVector3::TYPE_ID, PORTID_INPUT_TARGETFACINGDIR);
        SetupInputPort("Use Facing Dir", INPUTPORT_USEFACINGDIR, MCore::AttributeBool::TYPE_ID, PORTID_INPUT_USEFACINGDIR);

        // Setup the output ports.
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
    }

    BlendTreeMotionMatchNode::~BlendTreeMotionMatchNode()
    {
        FeatureSchema* usedSchema = nullptr;
        MotionMatchingEditorRequestBus::BroadcastResult(usedSchema, &MotionMatchingEditorRequests::GetDebugDrawFeatureSchema);
        if (usedSchema == &m_featureSchema)
        {
            MotionMatchingEditorRequestBus::Broadcast(&MotionMatchingEditorRequests::SetDebugDrawFeatureSchema, nullptr);
        }
    }

    bool BlendTreeMotionMatchNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        // Automatically register the default feature schema in case the schema is empty after loading the node.
        if (m_featureSchema.GetNumFeatures() == 0)
        {
            AZStd::string rootJointName;
            if (m_animGraph->GetNumAnimGraphInstances() > 0)
            {
                const Actor* actor = m_animGraph->GetAnimGraphInstance(0)->GetActorInstance()->GetActor();
                const Node* rootJoint = actor->GetMotionExtractionNode();
                if (rootJoint)
                {
                    rootJointName = rootJoint->GetNameString();
                }
            }

            DefaultFeatureSchemaInitSettings defaultSettings;
            defaultSettings.m_rootJointName = rootJointName.c_str();
            defaultSettings.m_leftFootJointName = "L_foot_JNT";
            defaultSettings.m_rightFootJointName = "R_foot_JNT";
            defaultSettings.m_pelvisJointName = "C_pelvis_JNT";
            DefaultFeatureSchema(m_featureSchema, defaultSettings);
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
        delete m_instance;
        delete m_data;

        m_data = aznew MotionMatching::MotionMatchingData(animGraphNode->m_featureSchema);
        m_instance = aznew MotionMatching::MotionMatchingInstance();

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
        AZ_Printf("Motion Matching", "Importing motion database...");
        MotionMatching::MotionMatchingData::InitSettings settings;
        settings.m_actorInstance = actorInstance;
        settings.m_frameImportSettings.m_sampleRate = animGraphNode->m_sampleRate;
        settings.m_importMirrored = animGraphNode->m_mirror;
        settings.m_maxKdTreeDepth = animGraphNode->m_maxKdTreeDepth;
        settings.m_minFramesPerKdTreeNode = animGraphNode->m_minFramesPerKdTreeNode;
        settings.m_motionList.reserve(animGraphNode->m_motionIds.size());
        settings.m_normalizeData = animGraphNode->m_normalizeData;
        settings.m_featureScalerType = animGraphNode->m_featureScalerType;
        settings.m_featureTansformerSettings.m_featureMin = animGraphNode->m_featureMin;
        settings.m_featureTansformerSettings.m_featureMax = animGraphNode->m_featureMax;
        settings.m_featureTansformerSettings.m_clip = animGraphNode->m_clipFeatures;

        for (const AZStd::string& id : animGraphNode->m_motionIds)
        {
            Motion* motion = motionSet->RecursiveFindMotionById(id);
            if (motion)
            {
                settings.m_motionList.emplace_back(motion);
            }
            else
            {
                AZ_Warning("Motion Matching", false, "Failed to get motion for motionset entry id '%s'", id.c_str());
            }
        }

        // Initialize the motion matching data (slow).
        AZ_Printf("Motion Matching", "Initializing motion matching...");
        if (!m_data->Init(settings))
        {
            AZ_Warning("Motion Matching", false, "Failed to initialize motion matching for anim graph node '%s'!", animGraphNode->GetName());
            SetHasError(true);
            return;
        }

        // Initialize the instance.
        AZ_Printf("Motion Matching", "Initializing instance...");
        MotionMatching::MotionMatchingInstance::InitSettings initSettings;
        initSettings.m_actorInstance = actorInstance;
        initSettings.m_data = m_data;
        m_instance->Init(initSettings);

        const float initTime = timer.GetDeltaTimeInSeconds();
        const size_t memUsage = m_data->GetFrameDatabase().CalcMemoryUsageInBytes();
        AZ_Printf("Motion Matching", "Finished in %.2f seconds (mem usage=%d bytes or %.2f mb)", initTime, memUsage, memUsage / (float)(1024 * 1024));
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

        AZ::Vector3 targetFacingDir = AZ::Vector3::CreateAxisY();
        TryGetInputVector3(animGraphInstance, INPUTPORT_TARGETFACINGDIR, targetFacingDir);

        bool useFacingDir = GetInputNumberAsBool(animGraphInstance, INPUTPORT_USEFACINGDIR);

        MotionMatching::MotionMatchingInstance* instance = uniqueData->m_instance;
        instance->Update(timePassedInSeconds, targetPos, targetFacingDir, useFacingDir, m_trajectoryQueryMode, m_pathRadius, m_pathSpeed);

        // set the current time to the new calculated time
        uniqueData->ClearInheritFlags();

        if (instance->GetMotionInstance())
        {
            uniqueData->SetPreSyncTime(instance->GetMotionInstance()->GetCurrentTime());
        }
        uniqueData->SetCurrentPlayTime(instance->GetNewMotionTime());

        if (uniqueData->GetPreSyncTime() > uniqueData->GetCurrentPlayTime())
        {
            uniqueData->SetPreSyncTime(uniqueData->GetCurrentPlayTime());
        }
            
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
        MotionMatching::MotionMatchingInstance* instance = uniqueData->m_instance;

        RequestRefDatas(animGraphInstance);
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
        data->ClearEventBuffer();
        data->ZeroTrajectoryDelta();

        if (uniqueData->GetHasError())
        {
            return;
        }

        MotionInstance* motionInstance = instance->GetMotionInstance();
        if (motionInstance)
        {
            motionInstance->UpdateByTimeValues(uniqueData->GetPreSyncTime(), uniqueData->GetCurrentPlayTime(), &data->GetEventBuffer());
            uniqueData->SetCurrentPlayTime(motionInstance->GetCurrentTime());
        }

        data->GetEventBuffer().UpdateEmitters(this);

        instance->PostUpdate(timePassedInSeconds);

        const Transform& trajectoryDelta = instance->GetMotionExtractionDelta();
        data->SetTrajectoryDelta(trajectoryDelta);
        data->SetTrajectoryDeltaMirrored(trajectoryDelta); // TODO: use a real mirrored version here.
        m_postUpdateTimeInMs = m_timer.GetDeltaTimeInSeconds() * 1000.0f;
    }

    void BlendTreeMotionMatchNode::Output(AnimGraphInstance* animGraphInstance)
    {
        AZ_PROFILE_SCOPE(Animation, "BlendTreeMotionMatchNode::Output");

        AZ_UNUSED(animGraphInstance);
        m_timer.Stamp();

        // Initialize to bind pose.
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        RequestPoses(animGraphInstance);
        AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
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
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_TARGETFACINGDIR));

        MotionMatching::MotionMatchingInstance* instance = uniqueData->m_instance;
        instance->SetLowestCostSearchFrequency(m_lowestCostSearchFrequency);

        Pose& outTransformPose = outputPose->GetPose();
        instance->Output(outTransformPose);

        // Performance metrics
        m_outputTimeInMs = m_timer.GetDeltaTimeInSeconds() * 1000.0f;
        {
#ifdef IMGUI_ENABLED
            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushPerformanceHistogramValue, "Update", m_updateTimeInMs);
            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushPerformanceHistogramValue, "Post Update", m_postUpdateTimeInMs);
            ImGuiMonitorRequestBus::Broadcast(&ImGuiMonitorRequests::PushPerformanceHistogramValue, "Output", m_outputTimeInMs);
#endif
        }
    }

    AZ::Crc32 BlendTreeMotionMatchNode::GetTrajectoryPathSettingsVisibility() const
    {
        if (m_trajectoryQueryMode == TrajectoryQuery::MODE_TARGETDRIVEN)
        {
            return AZ::Edit::PropertyVisibility::Hide;
        }

        return AZ::Edit::PropertyVisibility::Show;
    }

    AZ::Crc32 BlendTreeMotionMatchNode::GetFeatureScalerTypeSettingsVisibility() const
    {
        if (m_normalizeData)
        {
            return AZ::Edit::PropertyVisibility::Show;
        }

        return AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::Crc32 BlendTreeMotionMatchNode::GetMinMaxSettingsVisibility() const
    {
        if (m_normalizeData && m_featureScalerType == MotionMatchingData::MinMaxScalerType)
        {
            return AZ::Edit::PropertyVisibility::Show;
        }

        return AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::Crc32 BlendTreeMotionMatchNode::OnVisualizeSchemaButtonClicked()
    {
        FeatureSchema* usedSchema = nullptr;
        MotionMatchingEditorRequestBus::BroadcastResult(usedSchema, &MotionMatchingEditorRequests::GetDebugDrawFeatureSchema);
        if (usedSchema == &m_featureSchema)
        {
            MotionMatchingEditorRequestBus::Broadcast(&MotionMatchingEditorRequests::SetDebugDrawFeatureSchema, nullptr);
        }
        else
        {
            MotionMatchingEditorRequestBus::Broadcast(&MotionMatchingEditorRequests::SetDebugDrawFeatureSchema, &m_featureSchema);
        }

        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    AZStd::string BlendTreeMotionMatchNode::OnVisualizeSchemaButtonText() const
    {
        FeatureSchema* usedSchema = nullptr;
        MotionMatchingEditorRequestBus::BroadcastResult(usedSchema, &MotionMatchingEditorRequests::GetDebugDrawFeatureSchema);
        if (usedSchema == &m_featureSchema)
        {
            return "Disable Visualize Feature Schema";
        }

        return "Enable Visualize Feature Schema";
    }

    void BlendTreeMotionMatchNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeMotionMatchNode, AnimGraphNode>()
            ->Version(11)
            ->Field("lowestCostSearchFrequency", &BlendTreeMotionMatchNode::m_lowestCostSearchFrequency)
            ->Field("sampleRate", &BlendTreeMotionMatchNode::m_sampleRate)
            ->Field("controlSplineMode", &BlendTreeMotionMatchNode::m_trajectoryQueryMode)
            ->Field("pathRadius", &BlendTreeMotionMatchNode::m_pathRadius)
            ->Field("pathSpeed", &BlendTreeMotionMatchNode::m_pathSpeed)
            ->Field("normalizeData", &BlendTreeMotionMatchNode::m_normalizeData)
            ->Field("featureMin", &BlendTreeMotionMatchNode::m_featureMin)
            ->Field("featureMax", &BlendTreeMotionMatchNode::m_featureMax)
            ->Field("clipFeatures", &BlendTreeMotionMatchNode::m_clipFeatures)
            ->Field("maxKdTreeDepth", &BlendTreeMotionMatchNode::m_maxKdTreeDepth)
            ->Field("minFramesPerKdTreeNode", &BlendTreeMotionMatchNode::m_minFramesPerKdTreeNode)
            ->Field("mirror", &BlendTreeMotionMatchNode::m_mirror)
            ->Field("featureSchema", &BlendTreeMotionMatchNode::m_featureSchema)
            ->Field("motionIds", &BlendTreeMotionMatchNode::m_motionIds)
            ->Field("featureScalerType", &BlendTreeMotionMatchNode::m_featureScalerType)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeMotionMatchNode>("Motion Matching Node", "Motion Matching Attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMotionMatchNode::m_lowestCostSearchFrequency, "Search frequency", "How often per second we apply the motion matching search and find the lowest cost / best matching frame, and start to blend towards it.")
                ->Attribute(AZ::Edit::Attributes::Min, 0.001f)
                ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Step, 0.05f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMotionMatchNode::m_sampleRate, "Feature sample rate", "The sample rate (in Hz) used for extracting the features from the animations. The higher the sample rate, the more data will be used and the more options the motion matching search has available for the best matching frame.")
                ->Attribute(AZ::Edit::Attributes::Min, 1)
                ->Attribute(AZ::Edit::Attributes::Max, 240)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeMotionMatchNode::Reinit)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeMotionMatchNode::m_trajectoryQueryMode, "Trajectory Prediction", "Desired future trajectory generation mode.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                ->EnumAttribute(TrajectoryQuery::MODE_TARGETDRIVEN, "Target-driven")
                ->EnumAttribute(TrajectoryQuery::MODE_AUTOMATIC, "Automatic (Demo)")
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMotionMatchNode::m_pathRadius, "Path radius", "")
                ->Attribute(AZ::Edit::Attributes::Visibility, &BlendTreeMotionMatchNode::GetTrajectoryPathSettingsVisibility)
                ->Attribute(AZ::Edit::Attributes::Min, 0.0001f)
                ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMotionMatchNode::m_pathSpeed, "Path speed", "")
                ->Attribute(AZ::Edit::Attributes::Visibility, &BlendTreeMotionMatchNode::GetTrajectoryPathSettingsVisibility)
                ->Attribute(AZ::Edit::Attributes::Min, 0.0001f)
                ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
            ->ClassElement(AZ::Edit::ClassElements::Group, "Data Normalization")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMotionMatchNode::m_normalizeData, "Normalize Data", "Normalize feature data for more intuitive control over weighting the cost factors.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeMotionMatchNode::Reinit)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeMotionMatchNode::m_featureScalerType, "Type", "Feature scaler type to be used to normalize the data.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeMotionMatchNode::Reinit)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &BlendTreeMotionMatchNode::GetFeatureScalerTypeSettingsVisibility)
                    ->EnumAttribute(MotionMatchingData::StandardScalerType, "Standard Scaler")
                    ->EnumAttribute(MotionMatchingData::MinMaxScalerType, "Min-max Scaler")
                ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMotionMatchNode::m_featureMin, "Feature Minimum", "Minimum value after data transformation.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeMotionMatchNode::Reinit)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &BlendTreeMotionMatchNode::GetMinMaxSettingsVisibility)
                ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMotionMatchNode::m_featureMax, "Feature Maximum", "Maximum value after data transformation.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeMotionMatchNode::Reinit)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &BlendTreeMotionMatchNode::GetMinMaxSettingsVisibility)
                ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMotionMatchNode::m_clipFeatures, "Clip Features", "Clip feature values for outliers to the above range.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeMotionMatchNode::Reinit)
                ->Attribute(AZ::Edit::Attributes::Visibility, &BlendTreeMotionMatchNode::GetMinMaxSettingsVisibility)
            ->ClassElement(AZ::Edit::ClassElements::Group, "Acceleration Structure")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMotionMatchNode::m_maxKdTreeDepth, "Max kd-tree depth", "The maximum number of hierarchy levels in the kdTree.")
                ->Attribute(AZ::Edit::Attributes::Min, 1)
                ->Attribute(AZ::Edit::Attributes::Max, 20)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeMotionMatchNode::Reinit)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMotionMatchNode::m_minFramesPerKdTreeNode, "Min kd-tree node size", "The minimum number of frames to store per kdTree node.")
                ->Attribute(AZ::Edit::Attributes::Min, 1)
                ->Attribute(AZ::Edit::Attributes::Max, 100000)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeMotionMatchNode::Reinit)
            ->EndGroup()
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeMotionMatchNode::m_featureSchema, "FeatureSchema", "")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeMotionMatchNode::Reinit)
            ->UIElement(AZ::Edit::UIHandlers::Button, "", "")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeMotionMatchNode::OnVisualizeSchemaButtonClicked)
                ->Attribute(AZ::Edit::Attributes::ButtonText, &BlendTreeMotionMatchNode::OnVisualizeSchemaButtonText)
            ->DataElement(AZ_CRC_CE("MotionSetMotionIds"), &BlendTreeMotionMatchNode::m_motionIds, "Motions", "")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeMotionMatchNode::Reinit)
                ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::HideChildren)
            ;
    }
} // namespace EMotionFX::MotionMatching
