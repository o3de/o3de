/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/sort.h>

#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphBus.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphReferenceNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/MotionSet.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphReferenceNode, AnimGraphAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphReferenceNode::UniqueData, AnimGraphObjectUniqueDataAllocator, 0)

    static const char* sMaskedParameterNamesMember = "maskedParameterNames";

    AnimGraphReferenceNode::UniqueData::UniqueData(AnimGraphNode* node, AnimGraphInstance* parentAnimGraphInstance)
        : AnimGraphNodeData(node, parentAnimGraphInstance)
    {
    }

    AnimGraphReferenceNode::UniqueData::~UniqueData()
    {
        if (m_referencedAnimGraphInstance)
        {
            // If the anim graph instance is deleted through the AnimGraphManager, when we delete here,
            // the reference count may already be 0. We check if we didn't hit that case.
            // This should go away once the managers handle assets
            if (m_referencedAnimGraphInstance->GetReferenceCount() > 0)
            {
                m_referencedAnimGraphInstance->Destroy();
            }
            m_referencedAnimGraphInstance = nullptr;
        }
    }

    void AnimGraphReferenceNode::UniqueData::OnReferenceAnimGraphAssetChanged()
    {
        // This gets called with AnimGraphReferenceNode::OnAnimGraphAssetChanged().
        // At this state, the anim graph asset of the reference node already got changed but is not loaded yet.
        // We need to destruct the reference anim graph instance and nullptr our pointer so that we don't use it in case the node gets updated, pointing
        // to the non-existing old anim graph, while the new one is about to be loaded asynchronously.

        // In case the asset already got destroyed (AnimGraphAssetHandler::DestroyAsset()), it removed all anim graph instances already.
        if (GetAnimGraphManager().FindAnimGraphInstanceIndex(m_referencedAnimGraphInstance) != InvalidIndex)
        {
            m_referencedAnimGraphInstance->Destroy();
        }
        m_referencedAnimGraphInstance = nullptr;

        Clear();
        Update();
    }

    void AnimGraphReferenceNode::UniqueData::Update()
    {
        AnimGraphReferenceNode* referenceNode = azdynamic_cast<AnimGraphReferenceNode*>(m_object);
        AZ_Assert(referenceNode, "Unique data linked to incorrect node type.");

        MotionSet* motionSet = referenceNode->GetMotionSet();
        AnimGraphInstance* animGraphInstance = GetAnimGraphInstance();
        const AZ::Data::Asset<Integration::AnimGraphAsset> referenceAnimGraphAsset = referenceNode->GetReferencedAnimGraphAsset();

        const bool hasCycles = referenceNode->GetHasCycles();
        if (GetEMotionFX().GetIsInEditorMode())
        {
            SetHasError(hasCycles);
        }

        MotionSet* animGraphInstanceMotionSet = motionSet;
        if (!animGraphInstanceMotionSet)
        {
            // Use the parent's motion set
            animGraphInstanceMotionSet = animGraphInstance->GetMotionSet();
        }

        if (!m_referencedAnimGraphInstance &&
            referenceAnimGraphAsset &&
            referenceAnimGraphAsset.IsReady() &&
            !hasCycles)
        {
            AnimGraph* referenceAnimGraph = referenceAnimGraphAsset.Get()->GetAnimGraph();

            m_referencedAnimGraphInstance = AnimGraphInstance::Create(referenceAnimGraph,
                animGraphInstance->GetActorInstance(),
                animGraphInstanceMotionSet);
            m_referencedAnimGraphInstance->SetParentAnimGraphInstance(animGraphInstance);
        }
    }

    AnimGraphReferenceNode::AnimGraphReferenceNode()
        : AnimGraphNode()
        , m_hasCycles(false)
        , m_reinitMaskedParameters(false)
    {
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_RESULT, PORTID_OUTPUT_POSE);
    }


    AnimGraphReferenceNode::~AnimGraphReferenceNode()
    {
        // This node listens to changes in AnimGraph and MotionSet assets. We need to remove this node before disconnecting the asset bus to avoid the disconnect 
        // removing the MotionSet which can in turn access this node that is being deleted.
        if (m_animGraph)
        {
            m_animGraph->RemoveObject(this);
            m_animGraph = nullptr;
        }
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
    }

    void AnimGraphReferenceNode::Reinit()
    {
        AnimGraphNode::Reinit();

        LoadMotionSetAsset();
        LoadAnimGraphAsset();
    }

    void AnimGraphReferenceNode::RecursiveReinit()
    {
        Reinit();

        AnimGraph* referenceAnimGraph = GetReferencedAnimGraph();
        if (referenceAnimGraph)
        {
            referenceAnimGraph->RecursiveReinit();
        }
    }

    bool AnimGraphReferenceNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        LoadMotionSetAsset();
        LoadAnimGraphAsset();

        InitInternalAttributesForAllInstances();

        // No need to call InitAfterLoading for the referenced anim graph since that is called
        // from the anim graph asset

        return true;
    }

    const char* AnimGraphReferenceNode::GetPaletteName() const
    {
        return "Reference";
    }

    bool AnimGraphReferenceNode::GetHasVisualGraph() const
    {
        return m_animGraphAsset.GetId().IsValid() && m_animGraphAsset.IsReady();
    }

    AnimGraphObject::ECategory AnimGraphReferenceNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_SOURCES;
    }

    void AnimGraphReferenceNode::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_animGraphAsset)
        {
            AnimGraphNotificationBus::Broadcast(&AnimGraphNotificationBus::Events::OnReferenceAnimGraphAboutToBeChanged, this);
            m_animGraphAsset = asset;

            // TODO: remove once "owned by runtime" is gone
            asset.GetAs<Integration::AnimGraphAsset>()->GetAnimGraph()->SetIsOwnedByRuntime(false);

            OnAnimGraphAssetReady();
        }
        else if (asset == m_motionSetAsset)
        {
            m_motionSetAsset = asset;

            // TODO: remove once "owned by runtime"/ "SetIsOwnedByRuntime" is gone
            // The motion set asset held by reference node should not be editable in editor.
            asset.GetAs<Integration::MotionSetAsset>()->m_emfxMotionSet->SetIsOwnedByRuntime(true);

            OnMotionSetAssetReady();
        }
    }


    void AnimGraphReferenceNode::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_animGraphAsset)
        {
            AnimGraphNotificationBus::Broadcast(&AnimGraphNotificationBus::Events::OnReferenceAnimGraphAboutToBeChanged, this);
            m_animGraphAsset = asset;
            ReleaseAnimGraphInstances();

            // TODO: remove once "owned by runtime" is gone
            asset.GetAs<Integration::AnimGraphAsset>()->GetAnimGraph()->SetIsOwnedByRuntime(false);

            OnAnimGraphAssetReady();
        }
        else if (asset == m_motionSetAsset)
        {
            m_motionSetAsset = asset;

            // TODO: remove once "owned by runtime" is gone
            // The motion set asset held by reference node should not be editable in editor.
            asset.GetAs<Integration::MotionSetAsset>()->m_emfxMotionSet->SetIsOwnedByRuntime(true);

            OnMotionSetAssetReady();
        }
    }

    void AnimGraphReferenceNode::SetAnimGraphAsset(AZ::Data::Asset<Integration::AnimGraphAsset> asset)
    {
        m_animGraphAsset = AZStd::move(asset);
        m_reinitMaskedParameters = true;
    }


    void AnimGraphReferenceNode::Output(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphNode::Output(animGraphInstance);

        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        RequestPoses(animGraphInstance);
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();

        AnimGraph* referencedAnimGraph = GetReferencedAnimGraph();
        if (referencedAnimGraph && uniqueData->m_referencedAnimGraphInstance)
        {
            referencedAnimGraph->GetRootStateMachine()->PerformOutput(uniqueData->m_referencedAnimGraphInstance);
            *outputPose = *referencedAnimGraph->GetRootStateMachine()->GetMainOutputPose(uniqueData->m_referencedAnimGraphInstance);
        }
        else
        {
            outputPose->InitFromBindPose(actorInstance);
        }
    }


    void AnimGraphReferenceNode::TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        AnimGraphNode::TopDownUpdate(animGraphInstance, timePassedInSeconds);

        AnimGraph* referencedAnimGraph = GetReferencedAnimGraph();
        if (referencedAnimGraph)
        {
            UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
            if (uniqueData->m_referencedAnimGraphInstance)
            {
                referencedAnimGraph->GetRootStateMachine()->PerformTopDownUpdate(uniqueData->m_referencedAnimGraphInstance, timePassedInSeconds);
            }
        }
    }


    void AnimGraphReferenceNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        AnimGraphNode::Update(animGraphInstance, timePassedInSeconds); // update connections

        if (m_hasCycles)
        {
            // Don't continue if the graph has cycles
            return;
        }

        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));

        AnimGraph* referencedAnimGraph = GetReferencedAnimGraph();
        if (referencedAnimGraph)
        {
            AnimGraphInstance* referencedAnimGraphInstance = uniqueData->m_referencedAnimGraphInstance;
            if (referencedAnimGraphInstance)
            {
                if (uniqueData->m_parameterMappingCacheDirty)
                {
                    UpdateParameterMappingCache(animGraphInstance);
                    uniqueData->m_parameterMappingCacheDirty = false;
                }

                // Update the values for attributes that are fed through a connection
                AZ_Assert(m_inputPorts.size() == m_parameterIndexByPortIndex.size(), "Expected m_parameterIndexByPortIndex and numInputPorts to be in sync");

                const uint32 numInputPorts = static_cast<uint32>(m_inputPorts.size());
                for (uint32 i = 0; i < numInputPorts; ++i)
                {
                    MCore::Attribute* attribute = GetInputAttribute(animGraphInstance, i); // returns the attribute of the upstream side of the connection
                    if (attribute) // otherwise no connection
                    {
                        // Find the attribute in the reference anim graph
                        MCore::Attribute* parameterValue = referencedAnimGraphInstance->GetParameterValue(m_parameterIndexByPortIndex[i]);
                        parameterValue->InitFrom(attribute);
                        // TODO: check the output of InitFrom, mark it as an error if false
                    }
                }

                // Update the values for attributes that are being mapped
                for (const UniqueData::ValueParameterMappingCacheEntry& parameterMappingEntry : uniqueData->m_parameterMappingCache)
                {
                    MCore::Attribute* sourceAttribute = parameterMappingEntry.m_sourceAnimGraphInstance->GetParameterValue(parameterMappingEntry.m_sourceValueParameterIndex);
                    if (sourceAttribute)
                    {
                        // Find the attribute in the reference anim graph
                        MCore::Attribute* targetParameterValue = referencedAnimGraphInstance->GetParameterValue(parameterMappingEntry.m_targetValueParameterIndex);
                        targetParameterValue->InitFrom(sourceAttribute);
                        // TODO: check the output of InitFrom, mark it as an error if false
                    }

                }

                referencedAnimGraph->GetRootStateMachine()->PerformUpdate(uniqueData->m_referencedAnimGraphInstance, timePassedInSeconds);

                // update the sync track
                uniqueData->Init(referencedAnimGraphInstance, referencedAnimGraph->GetRootStateMachine());
            }
        }
    }


    void AnimGraphReferenceNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        AnimGraphNode::PostUpdate(animGraphInstance, timePassedInSeconds);

        if (m_hasCycles)
        {
            // Don't continue if the graph has cycles
            return;
        }

        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));

        AnimGraph* referencedAnimGraph = GetReferencedAnimGraph();
        if (referencedAnimGraph)
        {
            AnimGraphInstance* referencedAnimGraphInstance = uniqueData->m_referencedAnimGraphInstance;
            if (referencedAnimGraphInstance)
            {
                AnimGraphStateMachine* referenceStateMachine = referencedAnimGraph->GetRootStateMachine();

                referenceStateMachine->IncreaseRefDataRefCount(referencedAnimGraphInstance);

                referenceStateMachine->PerformPostUpdate(referencedAnimGraphInstance, timePassedInSeconds);

                AnimGraphStateMachine::UniqueData* referenceRootStateMachineUniqueData = static_cast<AnimGraphStateMachine::UniqueData*>(referenceStateMachine->FindOrCreateUniqueNodeData(referencedAnimGraphInstance));
                AnimGraphRefCountedData* referenceRootStateMachineData = referenceRootStateMachineUniqueData->GetRefCountedData();

                if (referenceRootStateMachineData)
                {
                    AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
                    data->SetEventBuffer(referenceRootStateMachineData->GetEventBuffer());
                    data->SetTrajectoryDelta(referenceRootStateMachineData->GetTrajectoryDelta());
                    data->SetTrajectoryDeltaMirrored(referenceRootStateMachineData->GetTrajectoryDeltaMirrored());
                    data->GetEventBuffer().UpdateEmitters(this);
                }

                referenceStateMachine->DecreaseRefDataRef(referencedAnimGraphInstance);

                // Release any left over ref data for the referenced anim graph instance.
                const uint32 threadIndex = referencedAnimGraphInstance->GetActorInstance()->GetThreadIndex();
                AnimGraphRefCountedDataPool& refDataPool = GetEMotionFX().GetThreadData(threadIndex)->GetRefCountedDataPool();
                const size_t numReferencedNodes = referencedAnimGraph->GetNumNodes();
                for (size_t i = 0; i < numReferencedNodes; ++i)
                {
                    const AnimGraphNode* node = referencedAnimGraph->GetNode(i);
                    AnimGraphNodeData* nodeData = static_cast<AnimGraphNodeData*>(referencedAnimGraphInstance->GetUniqueObjectData(node->GetObjectIndex()));
                    if (nodeData)
                    {
                        AnimGraphRefCountedData* refData = nodeData->GetRefCountedData();
                        if (refData)
                        {
                            refDataPool.Free(refData);
                            nodeData->SetRefCountedData(nullptr);
                        }
                    }
                }
            }
        }
    }

    void AnimGraphReferenceNode::RecursiveOnChangeMotionSet(AnimGraphInstance* animGraphInstance, MotionSet* newMotionSet)
    {
        MotionSet* motionSet = GetMotionSet();
        if (!motionSet)
        {
            motionSet = newMotionSet;
        }
        AnimGraphNode::RecursiveOnChangeMotionSet(animGraphInstance, motionSet);

        AnimGraph* referencedAnimGraph = GetReferencedAnimGraph();
        if (referencedAnimGraph)
        {
            UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
            if (uniqueData->m_referencedAnimGraphInstance)
            {
                referencedAnimGraph->GetRootStateMachine()->RecursiveOnChangeMotionSet(uniqueData->m_referencedAnimGraphInstance, motionSet);
            }
        }
    }


    void AnimGraphReferenceNode::Rewind(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphNode::Rewind(animGraphInstance);

        AnimGraph* referencedAnimGraph = GetReferencedAnimGraph();
        if (referencedAnimGraph)
        {
            UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
            if (uniqueData->m_referencedAnimGraphInstance)
            {
                referencedAnimGraph->GetRootStateMachine()->Rewind(uniqueData->m_referencedAnimGraphInstance);
            }
        }
    }

    void AnimGraphReferenceNode::RecursiveInvalidateUniqueDatas(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphNode::RecursiveInvalidateUniqueDatas(animGraphInstance);

        AnimGraph* referencedAnimGraph = GetReferencedAnimGraph();
        if (referencedAnimGraph)
        {
            UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->GetUniqueObjectData(m_objectIndex));
            if (uniqueData && uniqueData->m_referencedAnimGraphInstance)
            {
                uniqueData->m_referencedAnimGraphInstance->RecursiveInvalidateUniqueDatas();
            }
        }
    }

    void AnimGraphReferenceNode::RecursiveResetFlags(AnimGraphInstance* animGraphInstance, uint32 flagsToDisable)
    {
        AnimGraphNode::RecursiveResetFlags(animGraphInstance, flagsToDisable);

        AnimGraph* referencedAnimGraph = GetReferencedAnimGraph();
        if (referencedAnimGraph)
        {
            UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
            if (uniqueData->m_referencedAnimGraphInstance)
            {
                referencedAnimGraph->GetRootStateMachine()->RecursiveResetFlags(uniqueData->m_referencedAnimGraphInstance, flagsToDisable);
            }
        }
    }


    void AnimGraphReferenceNode::RecursiveSetUniqueDataFlag(AnimGraphInstance* animGraphInstance, uint32 flag, bool enabled)
    {
        AnimGraphNode::RecursiveSetUniqueDataFlag(animGraphInstance, flag, enabled);

        AnimGraph* referencedAnimGraph = GetReferencedAnimGraph();
        if (referencedAnimGraph)
        {
            UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
            if (uniqueData->m_referencedAnimGraphInstance)
            {
                referencedAnimGraph->GetRootStateMachine()->RecursiveSetUniqueDataFlag(uniqueData->m_referencedAnimGraphInstance, flag, enabled);
            }
        }
    }

 
    void AnimGraphReferenceNode::RecursiveCollectActiveNodes(AnimGraphInstance* animGraphInstance, AZStd::vector<AnimGraphNode*>* outNodes, const AZ::TypeId& nodeType) const
    {
        AnimGraphNode::RecursiveCollectActiveNodes(animGraphInstance, outNodes, nodeType);

        AnimGraph* referencedAnimGraph = GetReferencedAnimGraph();
        if (referencedAnimGraph)
        {
            UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
            if (uniqueData->m_referencedAnimGraphInstance)
            {
                referencedAnimGraph->GetRootStateMachine()->RecursiveCollectActiveNodes(uniqueData->m_referencedAnimGraphInstance, outNodes, nodeType);
            }
        }
    }


    AnimGraphPose* AnimGraphReferenceNode::GetMainOutputPose(AnimGraphInstance* animGraphInstance) const
    {
        return GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
    }


    void AnimGraphReferenceNode::RecursiveCollectObjects(AZStd::vector<AnimGraphObject*>& outObjects) const
    {
        AnimGraphNode::RecursiveCollectObjects(outObjects);

        AnimGraph* referencedAnimGraph = GetReferencedAnimGraph();
        if (referencedAnimGraph)
        {
            referencedAnimGraph->GetRootStateMachine()->RecursiveCollectObjects(outObjects);
        }
    }


    void AnimGraphReferenceNode::RecursiveCollectObjectsAffectedBy(AnimGraph* animGraph, AZStd::vector<AnimGraphObject*>& outObjects) const
    {
        AnimGraphNode::RecursiveCollectObjectsAffectedBy(animGraph, outObjects);

        AnimGraph* referencedAnimGraph = GetReferencedAnimGraph();
        if (referencedAnimGraph && referencedAnimGraph == animGraph)
        {
            outObjects.emplace_back(const_cast<AnimGraphReferenceNode*>(this));
        }
    }


    bool AnimGraphReferenceNode::RecursiveDetectCycles(AZStd::unordered_set<const AnimGraphNode*>& nodes) const
    {
        if (m_animGraphAsset && m_animGraphAsset.IsReady())
        {
            AnimGraph* referenceAnimGraph = m_animGraphAsset.Get()->GetAnimGraph();

            // Use an anim graph instance to recursively go through the parents. If we hit a parent that is referenceAnimGraph,
            // that means that the child we are about to add is a parent, therefore a cycle
            const size_t numAnimGraphInstances = m_animGraph->GetNumAnimGraphInstances();
            if (numAnimGraphInstances > 0)
            {
                AnimGraphInstance* animGraphInstance = m_animGraph->GetAnimGraphInstance(0);
                do 
                {
                    if (animGraphInstance->GetAnimGraph() == referenceAnimGraph)
                    {
                        return true;
                    }
                    animGraphInstance = animGraphInstance->GetParentAnimGraphInstance();
                } 
                while (animGraphInstance);
            }
            // If we don't have an animgraph instance, we have to go through the anim graph to find recursive, which is a bit slow.
            // We only want to do this in editor mode to save time.
            else if(GetEMotionFX().GetIsInEditorMode())
            {
                if(AzFramework::StringFunc::Equal(referenceAnimGraph->GetFileName(), GetAnimGraph()->GetFileName()))
                {
                    // Check if the reference animgraph and the animgraph from this node are the same file
                    return true;
                }

                if (!AnimGraphNode::RecursiveDetectCycles(nodes))
                {
                    // Check that any child node doesn't have this node. We have to be extra careful in this case to detect
                    // duplicates because an AnimGraphNode could be multiple times included through multiple reference nodes.
                    // The cycle is only present if this referenced graph ends up including the reference node.
                    for (const AnimGraphNode* node : nodes)
                    {
                        if (node == this)
                        {
                            return true;
                        }
                    }
                }
                else
                {
                    return true;
                }
                nodes.emplace(this);
                return referenceAnimGraph->GetRootStateMachine()->RecursiveDetectCycles(nodes);
            }
        }
        return false;
    }


    void AnimGraphReferenceNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphReferenceNode, AnimGraphNode>()
            ->Version(1)
            ->Field("animGraphAsset", &AnimGraphReferenceNode::m_animGraphAsset)
            ->Field("motionSetAsset", &AnimGraphReferenceNode::m_motionSetAsset)
            ->Field("activeMotionSetName", &AnimGraphReferenceNode::m_activeMotionSetName)
            ->Field(sMaskedParameterNamesMember, &AnimGraphReferenceNode::m_maskedParameterNames)
            ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphReferenceNode>("Reference node", "Reference node attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphReferenceNode::m_animGraphAsset, "Anim graph", "Animation graph to be assigned to this reference node.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphReferenceNode::OnAnimGraphAssetChanged)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphReferenceNode::m_motionSetAsset, "Motion set asset", "Motion set asset to be used for this reference.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphReferenceNode::OnMotionSetAssetChanged)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ_CRC("MotionSetName", 0xcf534ea6), &AnimGraphReferenceNode::m_activeMotionSetName, "Active motion set", "Motion set to use for this anim graph instance")
                ->Attribute(AZ_CRC("MotionSetAsset", 0xd4e88984), &AnimGraphReferenceNode::GetMotionSetAsset)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphReferenceNode::OnMotionSetChanged)
                ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphReferenceNode::HasMotionSetAsset)
            ->DataElement(AZ_CRC("AnimGraphParameterMask", 0x67dd0993), &AnimGraphReferenceNode::m_maskedParameterNames, "Parameter mask", "Parameters to be used as inputs. Parameters not selected as inputs are mapped.")
                ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::HideChildren)
            ;
    }
    
    void AnimGraphReferenceNode::ReleaseAnimGraphInstances()
    {
        // Inform the unique datas as well as other systems about the changed anim graph asset, destroy and nullptr the reference
        // anim graph instances so that we don't try to update an anim graph instance or while the asset already got destructed.
        const size_t numAnimGraphInstances = m_animGraph->GetNumAnimGraphInstances();
        for (size_t i = 0; i < numAnimGraphInstances; ++i)
        {
            AnimGraphInstance* animGraphInstance = m_animGraph->GetAnimGraphInstance(i);
            UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->GetUniqueObjectData(m_objectIndex));
            if (uniqueData)
            {
                uniqueData->OnReferenceAnimGraphAssetChanged();
            }
        }
    }

    void AnimGraphReferenceNode::OnAnimGraphAssetChanged()
    {
        AnimGraphNotificationBus::Broadcast(&AnimGraphNotificationBus::Events::OnReferenceAnimGraphAboutToBeChanged, this);

        ReleaseAnimGraphInstances();

        AnimGraphNotificationBus::Broadcast(&AnimGraphNotificationBus::Events::OnReferenceAnimGraphChanged, this);

        m_reinitMaskedParameters = true;
        if (GetNumConnections())
        {
            RemoveAllConnections();
        }
        LoadAnimGraphAsset();
    }

    void AnimGraphReferenceNode::OnMotionSetAssetChanged()
    {
        LoadMotionSetAsset();
    }


    void AnimGraphReferenceNode::OnMotionSetChanged()
    {
        OnMotionSetAssetReady(); // this method takes care of updating the motion set
    }


    void AnimGraphReferenceNode::OnMaskedParametersChanged()
    {
        const size_t numAnimGraphInstances = m_animGraph->GetNumAnimGraphInstances();
        for (size_t i = 0; i < numAnimGraphInstances; ++i)
        {
            AnimGraphInstance* animGraphInstance = m_animGraph->GetAnimGraphInstance(i);
            UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->GetUniqueObjectData(m_objectIndex));
            if (uniqueData)
            {
                uniqueData->m_parameterMappingCacheDirty = true;
                uniqueData->Invalidate();
            }
        }

        ReinitInputPorts();
    }


    void AnimGraphReferenceNode::LoadAnimGraphAsset()
    {
        if (m_animGraphAsset.GetId().IsValid())
        {
            // TODO: detect that the anim graph we are pointing to, is not in the parent hierarchy
            // (that would generate an infinite loop)

            if (!m_animGraphAsset.QueueLoad())
            {
                // If the asset is not ready and not queue loaded (probably is deleted or renamed), we have to clear out the input ports.
                ReinitInputPorts();
            }

            AZ::Data::AssetBus::MultiHandler::BusConnect(m_animGraphAsset.GetId());
        }
        else
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();
            if (m_motionSetAsset.GetId().IsValid())
            {
                AZ::Data::AssetBus::MultiHandler::BusConnect(m_motionSetAsset.GetId());
            }

            OnAnimGraphAssetReady(); // the method takes care of the case where the asset was cleared
        }
    }


    void AnimGraphReferenceNode::LoadMotionSetAsset()
    {
        if (m_motionSetAsset.GetId().IsValid())
        {
            m_motionSetAsset.QueueLoad();
            AZ::Data::AssetBus::MultiHandler::BusConnect(m_motionSetAsset.GetId());
        }
        else
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();
            if (m_animGraphAsset.GetId().IsValid())
            {
                AZ::Data::AssetBus::MultiHandler::BusConnect(m_animGraphAsset.GetId());
            }

            OnMotionSetAssetReady(); // the method takes care of the case where the asset was cleared
        }
    }


    AnimGraph* AnimGraphReferenceNode::GetReferencedAnimGraph() const
    {
        if (!m_hasCycles && m_animGraphAsset.GetId().IsValid() && m_animGraphAsset.IsReady())
        {
            return m_animGraphAsset.Get()->GetAnimGraph();
        }
        return nullptr;
    }


    MotionSet* AnimGraphReferenceNode::GetMotionSet() const
    {
        MotionSet* motionSet = nullptr;

        if (m_motionSetAsset && m_motionSetAsset.IsReady())
        {
            motionSet = m_motionSetAsset.Get()->m_emfxMotionSet.get();

            if (!m_activeMotionSetName.empty())
            {
                // If the motion set name is not empty, we need to find it. The motionset we are currently
                // pointing at is the root of that asset
                motionSet = motionSet->RecursiveFindMotionSetByName(m_activeMotionSetName);
            }
        }

        return motionSet;
    }


    AZ::Data::Asset<Integration::AnimGraphAsset> AnimGraphReferenceNode::GetReferencedAnimGraphAsset() const
    {
        return m_animGraphAsset;
    }


    AZ::Data::Asset<Integration::MotionSetAsset> AnimGraphReferenceNode::GetReferencedMotionSetAsset() const
    {
        return m_motionSetAsset;
    }


    AnimGraphInstance* AnimGraphReferenceNode::GetReferencedAnimGraphInstance(AnimGraphInstance* animGraphInstance) const
    {
        if (animGraphInstance)
        {
            EMotionFX::AnimGraphReferenceNode::UniqueData* uniqueData = static_cast<EMotionFX::AnimGraphReferenceNode::UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
            if (uniqueData->m_referencedAnimGraphInstance)
            {
                return uniqueData->m_referencedAnimGraphInstance;
            }
        }
        return nullptr;
    }


    void AnimGraphReferenceNode::OnAnimGraphAssetReady()
    {
        AZStd::unordered_set<const AnimGraphNode*> nodeSet;
        m_hasCycles = RecursiveDetectCycles(nodeSet);

        // Set the node info text.
        if (m_animGraphAsset)
        {
            if (!m_hasCycles)
            {
                if (GetEMotionFX().GetIsInEditorMode())
                {
                    // Extract just the filename (so we dont show the full path)
                    AZStd::string filename;
                    AzFramework::StringFunc::Path::GetFileName(m_animGraphAsset.Get()->GetAnimGraph()->GetFileName(), filename);
                    SetNodeInfo(filename.c_str());
                }
            }
            else if (GetEMotionFX().GetIsInEditorMode())
            {
                SetNodeInfo("Cyclic reference!");
            }
        }
        else if (GetEMotionFX().GetIsInEditorMode())
        {
            SetNodeInfo("<empty>");
        }

        ReinitInputPorts();

        const uint32 newAnimGraphId = m_animGraphAsset ? m_animGraphAsset->GetAnimGraph()->GetID() : MCORE_INVALIDINDEX32;

        if (m_lastProcessedAnimGraphId != newAnimGraphId)
        {
            InvalidateUniqueDatas();

            AnimGraphNotificationBus::Broadcast(&AnimGraphNotificationBus::Events::OnReferenceAnimGraphChanged, this);
        }

        m_lastProcessedAnimGraphId = newAnimGraphId;
    }


    void AnimGraphReferenceNode::OnMotionSetAssetReady()
    {
        MotionSet* motionSet = GetMotionSet();

        const size_t numAnimGraphInstances = m_animGraph->GetNumAnimGraphInstances();
        for (size_t i = 0; i < numAnimGraphInstances; ++i)
        {
            AnimGraphInstance* animGraphInstance = m_animGraph->GetAnimGraphInstance(i);
            UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));

            if (uniqueData->m_referencedAnimGraphInstance)
            {
                MotionSet* animGraphInstanceMotionSet = motionSet;
                if (!animGraphInstanceMotionSet)
                {
                    // Use the parent's motion set
                    animGraphInstanceMotionSet = animGraphInstance->GetMotionSet();
                }
                if (uniqueData->m_referencedAnimGraphInstance->GetMotionSet() != animGraphInstanceMotionSet)
                {
                    uniqueData->m_referencedAnimGraphInstance->SetMotionSet(animGraphInstanceMotionSet);
                }
            }
        }
    }

    AZStd::vector<AZStd::string> AnimGraphReferenceNode::GetParameters() const
    {
        return m_maskedParameterNames;
    }

    AnimGraph* AnimGraphReferenceNode::GetParameterAnimGraph() const
    {
        return GetReferencedAnimGraph();
    }

    void AnimGraphReferenceNode::ParameterMaskChanged(const AZStd::vector<AZStd::string>& newParameterMask)
    {
        AnimGraph* referenceAnimGraph = GetReferencedAnimGraph();
        if (referenceAnimGraph)
        {
            // If m_maskedParameterNames is empty, all parameters are being mapped (or attempted to). All requested
            // parameters should become ports, so we just sort and filter
            AZStd::vector<AZStd::string> newInputPorts = newParameterMask;
            SortAndRemoveDuplicates(referenceAnimGraph, newInputPorts);
            GetEventManager().OnInputPortsChanged(this, newInputPorts, sMaskedParameterNamesMember, newInputPorts);
            OnMaskedParametersChanged();
        }
    }

    void AnimGraphReferenceNode::AddRequiredParameters(AZStd::vector<AZStd::string>& parameterNames) const
    {
        // Only connected parameters are required, user should remove the connections before changing the mask to
        // exclude those parameters
        for (const AnimGraphNode::Port& port : GetInputPorts())
        {
            if (port.m_connection)
            {
                parameterNames.emplace_back(port.GetNameString());
            }
        }
        AnimGraph* referencedAnmGraph = GetReferencedAnimGraph();
        if (referencedAnmGraph)
        {
            SortAndRemoveDuplicates(referencedAnmGraph, parameterNames);
        }
    }

    void AnimGraphReferenceNode::ParameterAdded(const AZStd::string& newParameterName)
    {
        AZ_UNUSED(newParameterName);

        // When a new parameter is added, we dont put it into the mask, the user has to do that manually (by default
        // we will map it).
        // We just need to reinit the ports since inserting the parameter index requires to update m_parameterIndexByPortIndex
        ReinitInputPorts();
    }

    void AnimGraphReferenceNode::ParameterRenamed(const AZStd::string& oldParameterName, const AZStd::string& newParameterName)
    {
        AZ_UNUSED(newParameterName);
        if (m_maskedParameterNames.empty() || AZStd::find(m_maskedParameterNames.begin(), m_maskedParameterNames.end(), oldParameterName) != m_maskedParameterNames.end())
        {
            ReinitInputPorts();
        }
    }

    void AnimGraphReferenceNode::ParameterOrderChanged(const ValueParameterVector& beforeChange, const ValueParameterVector& afterChange)
    {
        AZ_UNUSED(beforeChange);

        AnimGraph* referencedAnimGraph = GetReferencedAnimGraph();
        if (referencedAnimGraph)
        {
            // Check if any of the indices have changed
            bool somethingChanged = false;
            const size_t parameterCount = m_parameterIndexByPortIndex.size();
            const size_t afterChangeParameterCount = afterChange.size();
            for (size_t i = 0; i < parameterCount; ++i)
            {
                const AZ::u32 currentIndex = m_parameterIndexByPortIndex[i];
                if (currentIndex < afterChangeParameterCount || afterChange[currentIndex]->GetName() != m_maskedParameterNames[i])
                {
                    somethingChanged = true;
                    break;
                }
            }
            if (somethingChanged)
            {
                // The list of parameters is the same, we just need to re-sort it
                AZStd::vector<AZStd::string> newParameterNames = m_maskedParameterNames;
                SortAndRemoveDuplicates(referencedAnimGraph, newParameterNames);
                GetEventManager().OnInputPortsChanged(this, newParameterNames, sMaskedParameterNamesMember, newParameterNames);
                OnMaskedParametersChanged();
            }
        }
    }

    void AnimGraphReferenceNode::ParameterRemoved(const AZStd::string& oldParameterName)
    {
        AZ_UNUSED(oldParameterName);
        // This may look unnatural, but the method ParameterOrderChanged deals with this as well, we just need to pass an empty before the change
        // and the current parameters after the change
        AnimGraph* referencedAnimGraph = GetReferencedAnimGraph();
        if (referencedAnimGraph)
        {
            ParameterOrderChanged(ValueParameterVector(), referencedAnimGraph->RecursivelyGetValueParameters());
        }
    }


    void AnimGraphReferenceNode::ReinitMaskedParameters()
    {
        m_maskedParameterNames.clear();
        
        if (!m_animGraphAsset)
        {
            return;
        }

        AnimGraph* referencedAnimGraph = m_animGraphAsset.Get()->GetAnimGraph();

        if (!referencedAnimGraph)
        {
            return;
        }

        AZ_Assert(!GetNumConnections(), "Unexpected connections");

        const ValueParameterVector& valueParameters = m_animGraph->RecursivelyGetValueParameters();
        const ValueParameterVector& referencedValueParameters = referencedAnimGraph->RecursivelyGetValueParameters();

        // For each parameter in referencedValueParameters, if it is not in valueParameters or is not compatible, add it
        // to m_maskedParameterNames. Ports are going to be created for all parameters in m_maskedParameterNames
        
        for (const ValueParameter* referencedValueParameter : referencedValueParameters)
        {
            // find a parameter with the same name and matching attributes
            bool found = false;
            for (const ValueParameter* valueParameter : valueParameters)
            {
                if (valueParameter->GetType() == referencedValueParameter->GetType()
                    && valueParameter->GetName() == referencedValueParameter->GetName())
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                m_maskedParameterNames.emplace_back(referencedValueParameter->GetName());
            }
        }
    }


    void AnimGraphReferenceNode::ReinitInputPorts()
    {
        if (m_reinitMaskedParameters)
        {
            ReinitMaskedParameters();
            m_reinitMaskedParameters = false;
        }

        bool portChanged = !m_inputPorts.empty();

        // Remove all input ports
        m_inputPorts.clear();
        m_parameterIndexByPortIndex.clear();

        // Get the ValueParameters from the AnimGraph
        if (m_animGraphAsset && m_animGraphAsset.Get()->GetAnimGraph())
        {
            AnimGraph* animGraph = m_animGraphAsset.Get()->GetAnimGraph();

            const ValueParameterVector& valueParameters = animGraph->RecursivelyGetValueParameters();
            const uint32 valueParametersSize = static_cast<uint32>(valueParameters.size());

            // Remove parameters from the parameter mask that no longer exist in the referenced graph
            AZStd::set<AZStd::string> removedParametersInMask;
            AZStd::set<size_t> removedPortIndexes;
            size_t parameterNameIndex = 0;
            for (const AZStd::string& parameterName : m_maskedParameterNames)
            {
                const auto it = AZStd::find_if(valueParameters.begin(), valueParameters.end(), [&parameterName](const ValueParameter* parameter)
                {
                    return parameter->GetName() == parameterName;
                });

                if (it == valueParameters.end())
                {
                    removedParametersInMask.emplace(parameterName);
                    removedPortIndexes.emplace(parameterNameIndex);
                }
                ++parameterNameIndex;
            }

            m_maskedParameterNames.erase(
                AZStd::remove_if(m_maskedParameterNames.begin(), m_maskedParameterNames.end(), [&removedParametersInMask](const AZStd::string& parameterName)
                {
                    return removedParametersInMask.find(parameterName) != removedParametersInMask.end();
                }),
                m_maskedParameterNames.end()
            );
            m_connections.erase(
                AZStd::remove_if(m_connections.begin(), m_connections.end(), [&removedPortIndexes](const BlendTreeConnection* connection)
                {
                    return removedPortIndexes.find(connection->GetTargetPort()) != removedPortIndexes.end();
                }),
                m_connections.end()
            );

            // Shift the port indexes of the remaining connections
            for (BlendTreeConnection* connection : m_connections)
            {
                AZ::u16 originalTargetPort = connection->GetTargetPort();
                AZ::u16 targetPort = originalTargetPort;
                for (const size_t removedPortIndex : removedPortIndexes)
                {
                    if (originalTargetPort > removedPortIndex)
                    {
                        --targetPort;
                    }
                }
                connection->SetTargetPort(targetPort);
            }

            // Now create the ports for the parameters that still exist
            // Init the input ports with the worst case
            InitInputPorts(valueParametersSize);

            uint32 realPortCount = 0;
            for (uint32 i = 0; i < valueParametersSize; ++i)
            {
                ValueParameter* valueParameter = valueParameters[i];

                if (AZStd::find(m_maskedParameterNames.begin(), m_maskedParameterNames.end(), valueParameters[i]->GetName()) != m_maskedParameterNames.end())
                {
                    SetupInputPort(valueParameter->GetName().c_str(), realPortCount, valueParameter->GetType(), realPortCount);
                    AZ_Assert(m_parameterIndexByPortIndex.size() == realPortCount, "AnimGraphReferenceNode::m_parameterIndexByPortIndex should be in sync with the port indices");
                    m_parameterIndexByPortIndex.push_back(i);
                    ++realPortCount;
                }
            }

            InitInputPorts(realPortCount); // this does a resize so now we are adjusted to the right count

            portChanged = true;
        }
        else
        {
            m_maskedParameterNames.clear();

            // If we dont have a graph anymore, we remove the connections
            if (GetNumConnections())
            {
                RemoveAllConnections();
            }
        }

        if (portChanged)
        {
            // Update the input ports. Don't call RelinkPortConnections,
            // because ReinitInputPorts cannot guarantee that the connected
            // nodes have been initialized
            for (BlendTreeConnection* connection : m_connections)
            {
                const AZ::u16 targetPortNr = connection->GetTargetPort();

                if (targetPortNr < m_inputPorts.size())
                {
                    m_inputPorts[targetPortNr].m_connection = connection;
                }
                else
                {
                    AZ_Error("EMotionFX", false, "Can't make connection to input port %i of '%s', max port count is %i.", targetPortNr, GetName(), m_inputPorts.size());
                }
            }
            AnimGraphNotificationBus::Broadcast(&AnimGraphNotificationBus::Events::OnSyncVisualObject, this);
        }
    }

    void AnimGraphReferenceNode::UpdateParameterMappingCache(AnimGraphInstance* animGraphInstance)
    {
        AZ_Assert(animGraphInstance, "Expected non-null anim graph instance");

        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        uniqueData->m_parameterMappingCache.clear();

        AnimGraph* referencedAnimGraph = GetReferencedAnimGraph();

        AnimGraphInstance* referencedAnimGraphInstance = uniqueData->m_referencedAnimGraphInstance;
        const ValueParameterVector& valueParameters = referencedAnimGraph->RecursivelyGetValueParameters();

        // Remove those parameters that are in the mask since those parameters are being fed through input ports
        AZStd::vector<size_t> maskedParameterIndexes;
        maskedParameterIndexes.reserve(m_maskedParameterNames.size());
        for (const AZStd::string& paramName : m_maskedParameterNames)
        {
            const AZ::Outcome<size_t> parameterIndex = referencedAnimGraph->FindValueParameterIndexByName(paramName);
            if (parameterIndex.IsSuccess())
            {
                maskedParameterIndexes.emplace_back(parameterIndex.GetValue());
            }
        }

        // Sort them so we can find them faster in the loop below
        AZStd::sort(maskedParameterIndexes.begin(), maskedParameterIndexes.end());
        AZStd::vector<size_t>::const_iterator currentIndexToExcludeIt = maskedParameterIndexes.begin();

        // Fill the mapping
        const uint32 valueParametersSize = static_cast<uint32>(valueParameters.size());
        for (uint32 i = 0; i < valueParametersSize; ++i)
        {
            const ValueParameter* valueParameter = valueParameters[i];

            // Only map the parameters that are not currently selected as inputs
            if (currentIndexToExcludeIt != maskedParameterIndexes.end() && *currentIndexToExcludeIt == i)
            {
                // excluded, move to the next to exclude
                ++currentIndexToExcludeIt;
            }
            else
            {
                AnimGraphInstance* parentAnimGraphInstance = referencedAnimGraphInstance->GetParentAnimGraphInstance();
                while (parentAnimGraphInstance)
                {
                    // Name lookup is expensive, if the amount of parameters match between this and the parent, is very likely
                    // that they have the same, in which case we are going to to try if the index is the same, if it is, we
                    // avoid the lookup.
                    AnimGraph* parentAnimGraph = parentAnimGraphInstance->GetAnimGraph();
                    if (parentAnimGraph->GetNumValueParameters() == valueParametersSize)
                    {
                        const ValueParameter* parentValueParameter = parentAnimGraph->FindValueParameter(i);
                        if (parentValueParameter->GetName() == valueParameter->GetName()
                            && parentValueParameter->GetType() == valueParameter->GetType())
                        {
                            // The name and type matches, so the indexes are the same
                            uniqueData->m_parameterMappingCache.emplace_back(parentAnimGraphInstance,
                                i,          // source index
                                i);         // target index
                            break; // stop searching
                        }
                    }

                    AZ::Outcome<size_t> valueParameterIndex = parentAnimGraphInstance->FindParameterIndex(valueParameter->GetName());
                    if (valueParameterIndex.IsSuccess())
                    {
                        const ValueParameter* parentValueParameter = parentAnimGraph->FindValueParameter(valueParameterIndex.GetValue());
                        if (parentValueParameter->GetType() == valueParameter->GetType())
                        {
                            // Found a parameter to do the mapping
                            uniqueData->m_parameterMappingCache.emplace_back(parentAnimGraphInstance,
                                static_cast<uint32>(valueParameterIndex.GetValue()),            // source index
                                i);                                                             // target index
                            break; // stop searching through the parents
                        }
                    }

                    parentAnimGraphInstance = parentAnimGraphInstance->GetParentAnimGraphInstance();
                }
            }

            // Set back the default value, this will account for the cases where the parameter is added
            // to the mask and no longer mapped. We dont want old values from the mapping to stay in those
            // values
            MCore::Attribute* parameterValue = referencedAnimGraphInstance->GetParameterValue(i);
            if (parameterValue)
            {
                valueParameter->AssignDefaultValueToAttribute(parameterValue);
            }
        }
    }

}   // namespace EMotionFX
