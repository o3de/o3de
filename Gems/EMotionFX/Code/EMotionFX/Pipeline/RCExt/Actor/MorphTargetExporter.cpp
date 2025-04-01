/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/containers/array.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/algorithm.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IBlendShapeRule.h>
#include <SceneAPI/SceneCore/Utilities/CoordinateSystemConverter.h>

#include <SceneAPIExt/Groups/IActorGroup.h>
#include <RCExt/ExportContexts.h>
#include <RCExt/Actor/MorphTargetExporter.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/MorphTargetStandard.h>
#include <EMotionFX/Source/MorphSetup.h>
#include <Integration/System/SystemCommon.h>
#include <MCore/Source/AzCoreConversions.h>


namespace EMotionFX
{
    namespace Pipeline
    {
        namespace SceneEvents = AZ::SceneAPI::Events;
        namespace SceneUtil = AZ::SceneAPI::Utilities;

        MorphTargetExporter::MorphTargetExporter()
        {
            BindToCall(&MorphTargetExporter::ProcessMorphTargets);
        }

        void MorphTargetExporter::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<MorphTargetExporter, AZ::SceneAPI::SceneCore::ExportingComponent>()->Version(1);
            }
        }

        AZ::SceneAPI::Events::ProcessingResult MorphTargetExporter::ProcessMorphTargets(ActorMorphBuilderContext& context)
        {
            if (context.m_phase != AZ::RC::Phase::Filling)
            {
                return AZ::SceneAPI::Events::ProcessingResult::Ignored;
            }

            if (!context.m_group.RTTI_IsTypeOf(EMotionFX::Pipeline::Group::IActorGroup::TYPEINFO_Uuid()))
            {
                return AZ::SceneAPI::Events::ProcessingResult::Ignored;
            }

            AZStd::shared_ptr<const AZ::SceneAPI::DataTypes::IBlendShapeRule> morphTargetRule = context.m_group.GetRuleContainerConst().FindFirstByType<AZ::SceneAPI::DataTypes::IBlendShapeRule>();
            if (!morphTargetRule)
            {
                return AZ::SceneAPI::Events::ProcessingResult::Ignored;
            }

            const AZ::SceneAPI::Containers::SceneGraph& graph = context.m_scene.GetGraph();

            // Clone the actor
            auto baseActorImage = context.m_actor->Clone();
            baseActorImage->RemoveAllNodeMeshes();

            AZStd::unordered_set<AZStd::string> visitedBlendShapeNames;
            AZStd::string morphTargetName;
            AZStd::string morphNodeName;
            AZStd::string morphParentNodeName;

            AZ::SceneAPI::Events::ProcessingResult processingResult = SceneEvents::ProcessingResult::Success;

            // Outer loop isolates unique morph target names in the selection list
            morphTargetRule->GetSceneNodeSelectionList().EnumerateSelectedNodes(
                [&](const AZStd::string& name)
                {
                    AZ::SceneAPI::Containers::SceneGraph::NodeIndex nodeIndex = graph.Find(name);
                    morphTargetName = context.m_scene.GetGraph().GetNodeName(nodeIndex).GetName();
                    if (AZStd::find(visitedBlendShapeNames.begin(), visitedBlendShapeNames.end(), morphTargetName) !=
                        visitedBlendShapeNames.end())
                    {
                        return true;
                    }
                    visitedBlendShapeNames.insert(morphTargetName);
                    AZStd::unique_ptr<Actor> morphTargetActor = baseActorImage->Clone();
                    if (!morphTargetActor)
                    {
                        processingResult = SceneEvents::ProcessingResult::Failure;
                        return false;
                    }
                    morphTargetActor->SetName(morphTargetName.c_str());

                    // Add the morph target actor to the main actor
                    EMotionFX::MorphTargetStandard* morphTarget =
                        EMotionFX::MorphTargetStandard::Create(false, context.m_actor, morphTargetActor.get(), morphTargetName.c_str());

                    // Assume LOD 0
                    EMotionFX::MorphSetup* morphSetup = context.m_actor->GetMorphSetup(0);
                    if (!morphSetup)
                    {
                        morphSetup = EMotionFX::MorphSetup::Create();
                        context.m_actor->SetMorphSetup(0, morphSetup);
                    }

                    if (morphSetup)
                    {
                        morphSetup->AddMorphTarget(morphTarget);
                    }

                    return true;
                });

            return processingResult;
        }
    } // namespace Pipeline
} // namespace EMotionFX

