/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IAnimationData.h>
#include <SceneAPI/SceneData/Rules/CoordinateSystemRule.h>

#include <SceneAPIExt/Groups/IMotionGroup.h>
#include <SceneAPIExt/Rules/MotionRangeRule.h>
#include <SceneAPIExt/Rules/MotionAdditiveRule.h>
#include <SceneAPIExt/Rules/MotionSamplingRule.h>
#include <SceneAPIExt/Rules/RootMotionExtractionRule.h>
#include <RCExt/Motion/MotionDataBuilder.h>
#include <RCExt/ExportContexts.h>

#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionData/MotionDataFactory.h>
#include <EMotionFX/Source/MotionData/MotionData.h>
#include <EMotionFX/Source/MotionData/NonUniformMotionData.h>
#include <EMotionFX/Source/MotionData/UniformMotionData.h>
#include <MCore/Source/AzCoreConversions.h>

#include <AzCore/Math/Uuid.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/containers/array.h>
#include <AzToolsFramework/Debug/TraceContext.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace SceneEvents = AZ::SceneAPI::Events;
        namespace SceneUtil = AZ::SceneAPI::Utilities;
        namespace SceneContainers = AZ::SceneAPI::Containers;
        namespace SceneViews = AZ::SceneAPI::Containers::Views;
        namespace SceneDataTypes = AZ::SceneAPI::DataTypes;

        MotionDataBuilder::MotionDataBuilder()
        {
            BindToCall(&MotionDataBuilder::BuildMotionData);
        }

        void MotionDataBuilder::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<MotionDataBuilder, AZ::SceneAPI::SceneCore::ExportingComponent>()->Version(1);
            }
        }

        void InitAndOptimizeMotionData(MotionData* finalMotionData, const NonUniformMotionData* sourceMotionData, float sampleRate, const Rule::MotionSamplingRule* samplingRule, const AZStd::vector<size_t>& rootJoints)
        {
            // Init and resample.
            finalMotionData->InitFromNonUniformData(
                sourceMotionData,
                /*keepSameSampleRate=*/false,
                /*newSampleRate=*/sampleRate,
                samplingRule ? !samplingRule->GetKeepDuration() : false);

            // Get the quality in percentages.
            const float translationQualityPercentage = samplingRule ? AZ::GetClamp(samplingRule->GetTranslationQualityPercentage(), 1.0f, 100.0f) : 75.0f;
            const float rotationQualityPercentage = samplingRule ? AZ::GetClamp(samplingRule->GetRotationQualityPercentage(), 1.0f, 100.0f) : 75.0f;
            const float scaleQualityPercentage = samplingRule ? AZ::GetClamp(samplingRule->GetScaleQualityPercentage(), 1.0f, 100.0f) : 75.0f;

            // Perform keyframe optimization.
            MotionData::OptimizeSettings optimizeSettings;
            optimizeSettings.m_maxPosError = AZ::Lerp(0.0225f, 0.0f, (translationQualityPercentage - 1.0f) / 99.0f); // The percentage is remapped from 1.100 to 0..99, as otherwise a percentage of 1 doesn't start at the start of the lerp.
            optimizeSettings.m_maxRotError = AZ::Lerp(0.0225f, 0.0f, (rotationQualityPercentage - 1.0f) / 99.0f);
            optimizeSettings.m_maxScaleError = AZ::Lerp(0.0225f, 0.0f, (scaleQualityPercentage - 1.0f) / 99.0f);

            optimizeSettings.m_maxFloatError = 0.0001f;
            optimizeSettings.m_maxMorphError = 0.0001f;
            optimizeSettings.m_jointIgnoreList = rootJoints; // Skip optimizing root joints, as that makes the feet jitter.
            optimizeSettings.m_updateDuration = samplingRule ? !samplingRule->GetKeepDuration() : false;
            finalMotionData->Optimize(optimizeSettings);
        }

        // Automatically determine what produces the smallest memory footprint motion data, either UniformMotionData or NonUniformMotionData.
        // We don't iterate through all registered motion data types, because we dont know if smaller memory footprint is always better.
        // However, when we pick between Uniform or NonUniform, we always want Uniform if that's smaller in size, as it gives higher performance and is smaller in memory footprint.
        // Later on we can add more automatic modes, where we always find the smallest size between all, or the higher performance one.
        MotionData* AutoCreateMotionData(const NonUniformMotionData* sourceMotionData, float sampleRate, const Rule::MotionSamplingRule* samplingRule, const AZStd::vector<size_t>& rootJoints)
        {
            MotionData* finalMotionData = nullptr;

            AZ_TracePrintf("EMotionFX", "*** Automatic motion data picking has been selected");
            AZStd::array<MotionData*, 2> tempData { aznew UniformMotionData(), aznew NonUniformMotionData() };
            size_t smallestNumBytes = std::numeric_limits<size_t>::max();
            size_t smallestIndex = 0;
            size_t uniformDataNumBytes = 0;
            size_t uniformDataIndex = 0;
            for (size_t m = 0; m < tempData.size(); ++m)
            {
                // Init/fill and optimize the data.
                MotionData* data = tempData[m];
                InitAndOptimizeMotionData(data, sourceMotionData, sampleRate, samplingRule, rootJoints);

                // Calculate the size required on disk.
                MotionData::SaveSettings saveSettings;
                const size_t numBytes = data->CalcStreamSaveSizeInBytes(saveSettings);
                if (numBytes < smallestNumBytes)
                {
                    smallestNumBytes = numBytes;
                    smallestIndex = m;
                }

                if (azrtti_istypeof<UniformMotionData>(data))
                {
                    uniformDataNumBytes = numBytes;
                    uniformDataIndex = m;
                }

                AZ_TracePrintf("EMotionFX", "Estimated size for '%s' is %zu bytes.", data->RTTI_GetTypeName(), numBytes);
            }

            AZ_TracePrintf("EMotionFX", "Smallest data type is '%s'", tempData[smallestIndex]->RTTI_GetTypeName());

            // If the smallest type is not the fast UniformData type, check if the byte size difference between the uniform data and
            // the smallest data type is within given limits. If so, pick the uniform data instead.
            const MotionData* smallestData = tempData[smallestIndex];
            if (!azrtti_istypeof<UniformMotionData>(smallestData))
            {
                const size_t smallestSize = smallestNumBytes;
                const size_t sizeDiffInBytes = uniformDataNumBytes - smallestSize;
                const float allowedSizeOverheadPercentage = samplingRule ? samplingRule->GetAllowedSizePercentage() : 15.0f;
                const float overheadPercentage = sizeDiffInBytes / static_cast<float>(smallestSize); // Between 0 and 1.
                if (overheadPercentage <= allowedSizeOverheadPercentage * 0.01f) // Multiply by 0.01 as allowedSizeOverheadPercentage is in range of 0..100
                {
                    smallestIndex = uniformDataIndex;
                    AZ_TracePrintf("EMotionFX", "Overriding to use UniformMotionData because the size overhead is within specified limits of '%.1f' percent (actual overhead=%.1f percent)",
                        allowedSizeOverheadPercentage,
                        overheadPercentage * 100.0f);
                }
                else
                {
                    AZ_TracePrintf("EMotionFX", "Uniform data overhead is larger than the allowed overhead percentage (allowed=%.1f percent, actual=%.1f percent)",
                        allowedSizeOverheadPercentage,
                        overheadPercentage * 100.0f);
                }
            }

            finalMotionData = tempData[smallestIndex];

            // Delete all temp datas, except the one we used as final data, as we assign that to the motion later on.
            // Which means it will be automatically destroyed by the motion destructor later.
            for (size_t m = 0; m < tempData.size(); ++m)
            {
                if (m != smallestIndex)
                {
                    delete tempData[m];
                }
            }

            return finalMotionData;
        }

        AZ::SceneAPI::DataTypes::MatrixType MotionDataBuilder::GetLocalSpaceBindPose(const SceneContainers::SceneGraph& sceneGraph,
            const SceneContainers::SceneGraph::NodeIndex rootBoneNodeIndex,
            const SceneContainers::SceneGraph::NodeIndex nodeIndex,
            const SceneDataTypes::ITransform* transform,
            const SceneDataTypes::IBoneData* bone) const
        {
            AZ::SceneAPI::DataTypes::MatrixType nodeTransform = AZ::SceneAPI::DataTypes::MatrixType::CreateIdentity();
            if (bone)
            {
                nodeTransform = bone->GetWorldTransform();
            }
            else if (transform)
            {
                nodeTransform = transform->GetMatrix();
            }

            if (nodeIndex != rootBoneNodeIndex)
            {
                const SceneContainers::SceneGraph::NodeIndex parentNodeIndex = sceneGraph.GetNodeParent(nodeIndex);
                const SceneDataTypes::IGraphObject* parentNode = sceneGraph.GetNodeContent(parentNodeIndex).get();
                if (const SceneDataTypes::IBoneData* parentBone = azrtti_cast<const SceneDataTypes::IBoneData*>(parentNode))
                {
                    return parentBone->GetWorldTransform().GetInverseFull() * nodeTransform;
                }
            }

            return nodeTransform;
        }

        AZ::SceneAPI::Events::ProcessingResult MotionDataBuilder::BuildMotionData(MotionDataBuilderContext& context)
        {
            if (context.m_phase != AZ::RC::Phase::Filling)
            {
                return SceneEvents::ProcessingResult::Ignored;
            }

            const Group::IMotionGroup& motionGroup = context.m_group;
            const char* rootBoneName = motionGroup.GetSelectedRootBone().c_str();
            AZ_TraceContext("Root bone", rootBoneName);

            const SceneContainers::SceneGraph& graph = context.m_scene.GetGraph();

            SceneContainers::SceneGraph::NodeIndex rootBoneNodeIndex = graph.Find(rootBoneName);
            if (!rootBoneNodeIndex.IsValid())
            {
                AZ_TracePrintf(SceneUtil::ErrorWindow, "Root bone cannot be found.\n");
                return SceneEvents::ProcessingResult::Failure;
            }

            // Get the coordinate system conversion rule.
            AZ::SceneAPI::CoordinateSystemConverter coordSysConverter;
            AZStd::shared_ptr<AZ::SceneAPI::SceneData::CoordinateSystemRule> coordinateSystemRule = motionGroup.GetRuleContainerConst().FindFirstByType<AZ::SceneAPI::SceneData::CoordinateSystemRule>();
            if (coordinateSystemRule)
            {
                coordinateSystemRule->UpdateCoordinateSystemConverter();
                coordSysConverter = coordinateSystemRule->GetCoordinateSystemConverter();
            }

            // Set new motion data.
            NonUniformMotionData* motionData = aznew NonUniformMotionData();

            // Grab the rules we need before visiting the scene graph.
            AZStd::shared_ptr<const Rule::MotionSamplingRule> samplingRule = motionGroup.GetRuleContainerConst().FindFirstByType<Rule::MotionSamplingRule>();
            AZStd::shared_ptr<const Rule::MotionAdditiveRule> additiveRule = motionGroup.GetRuleContainerConst().FindFirstByType<Rule::MotionAdditiveRule>();
            AZStd::shared_ptr<const Rule::RootMotionExtractionRule> rootMotionExtractionRule = motionGroup.GetRuleContainerConst().FindFirstByType<Rule::RootMotionExtractionRule>();
            motionData->SetAdditive(additiveRule ? true : false);

            AZStd::vector<size_t> rootJoints; // The list of root nodes.

            // Data for root motion extraction
            const size_t InvalidJointDataIndex = AZStd::numeric_limits<size_t>::max();
            size_t sampleJointDataIndex = InvalidJointDataIndex;
            size_t rootJointDataIndex = InvalidJointDataIndex;

            size_t maxNumFrames = 0;
            double lowestTimeStep = 999999999.0;

            auto nameStorage = graph.GetNameStorage();
            auto contentStorage = graph.GetContentStorage();
            auto nameContentView = SceneContainers::Views::MakePairView(nameStorage, contentStorage);
            auto graphDownwardsView = SceneViews::MakeSceneGraphDownwardsView<SceneViews::BreadthFirst>(graph, rootBoneNodeIndex, nameContentView.begin(), true);
            for (auto it = graphDownwardsView.begin(); it != graphDownwardsView.end(); ++it)
            {
                if (!it->second)
                {
                    it.IgnoreNodeDescendants();
                    continue;
                }

                // Check if we are dealing with a transform node or a bone and only recurse down the node hierarchy in this case.
                const SceneDataTypes::IBoneData* nodeBone = azrtti_cast<const SceneDataTypes::IBoneData*>(it->second.get());
                const SceneDataTypes::ITransform* nodeTransform = azrtti_cast<const SceneDataTypes::ITransform*>(it->second.get());
                if (!nodeBone && !nodeTransform)
                {
                    it.IgnoreNodeDescendants();
                    continue;
                }

                const SceneContainers::SceneGraph::NodeIndex boneNodeIndex = graph.Find(it->first.GetPath());
                const char* nodeName = it->first.GetName();
                const char* nodePath = it->first.GetPath();

                // Add root joint to motion data when root motion extraction rule exsits.
                if (rootMotionExtractionRule && boneNodeIndex == rootBoneNodeIndex)
                {
                    rootJointDataIndex = motionData->AddJoint(nodeName, Transform::CreateIdentity(), Transform::CreateIdentity());
                }

                // Currently only get the first (one) AnimationData
                auto childView = SceneViews::MakeSceneGraphChildView<SceneViews::AcceptEndPointsOnly>(graph, graph.ConvertToNodeIndex(it.GetHierarchyIterator()),
                        graph.GetContentStorage().begin(), true);
                auto result = AZStd::find_if(childView.begin(), childView.end(), SceneContainers::DerivedTypeFilter<SceneDataTypes::IAnimationData>());
                if (result == childView.end())
                {
                    continue;
                }

                const SceneDataTypes::IAnimationData* animation = azrtti_cast<const SceneDataTypes::IAnimationData*>(result->get());
                const size_t jointDataIndex = motionData->AddJoint(nodeName, Transform::CreateIdentity(), Transform::CreateIdentity());

                // Keep track of the sample joint index.
                if (rootMotionExtractionRule && sampleJointDataIndex == InvalidJointDataIndex
                    && AzFramework::StringFunc::Find(nodePath, rootMotionExtractionRule->GetData()->m_sampleJoint.c_str()) != AZStd::string::npos)
                {
                    sampleJointDataIndex = jointDataIndex;
                }

                // If we deal with a root bone or one of its child nodes, disable the keytrack optimization.
                // This prevents sliding feet etc. A better solution is probably to increase compression rates based on the "distance" from the root node, hierarchy wise.
                if (graph.GetNodeParent(boneNodeIndex) == rootBoneNodeIndex || boneNodeIndex == rootBoneNodeIndex)
                {
                    rootJoints.emplace_back(jointDataIndex);
                }

                const size_t sceneFrameCount = aznumeric_caster(animation->GetKeyFrameCount());
                size_t startFrame = 0;
                size_t endFrame = 0;
                if (motionGroup.GetRuleContainerConst().ContainsRuleOfType<const Rule::MotionRangeRule>())
                {
                    AZStd::shared_ptr<const Rule::MotionRangeRule> motionRangeRule = motionGroup.GetRuleContainerConst().FindFirstByType<const Rule::MotionRangeRule>();
                    startFrame = aznumeric_caster(motionRangeRule->GetStartFrame());
                    endFrame = aznumeric_caster(motionRangeRule->GetEndFrame());

                    // Sanity check
                    if (startFrame >= sceneFrameCount)
                    {
                        AZ_TracePrintf(SceneUtil::ErrorWindow, "Start frame %d is greater or equal than the actual number of frames %d in animation.\n", startFrame, sceneFrameCount);
                        return SceneEvents::ProcessingResult::Failure;
                    }
                    if (endFrame >= sceneFrameCount)
                    {
                        AZ_TracePrintf(SceneUtil::WarningWindow, "End frame %d is greater or equal than the actual number of frames %d in animation. Clamping the end frame to %d\n", endFrame, sceneFrameCount, sceneFrameCount - 1);
                        endFrame = sceneFrameCount - 1;
                    }
                }
                else
                {
                    startFrame = 0;
                    endFrame = sceneFrameCount - 1;
                }

                const size_t numFrames = (endFrame - startFrame) + 1;

                maxNumFrames = AZ::GetMax(numFrames, maxNumFrames);

                motionData->AllocateJointPositionSamples(jointDataIndex, numFrames);
                motionData->AllocateJointRotationSamples(jointDataIndex, numFrames);
                EMFX_SCALECODE
                (
                    motionData->AllocateJointScaleSamples(jointDataIndex, numFrames);
                )

                // Get the bind pose transform in local space.
                using SceneAPIMatrixType = AZ::SceneAPI::DataTypes::MatrixType;
                const SceneAPIMatrixType bindSpaceLocalTransform = GetLocalSpaceBindPose(graph, rootBoneNodeIndex, boneNodeIndex, nodeTransform, nodeBone);

                // Get the time step and make sure it didn't change compared to other joint animations.
                const double timeStep = animation->GetTimeStepBetweenFrames();
                lowestTimeStep = AZ::GetMin<double>(timeStep, lowestTimeStep);

                AZ::SceneAPI::DataTypes::MatrixType sampleFrameTransformInverse;
                if (additiveRule)
                {
                    size_t sampleFrameIndex = additiveRule->GetSampleFrameIndex();
                    if (sampleFrameIndex >= sceneFrameCount)
                    {
                        AZ_Assert(false, "The requested sample frame index is greater than the total frame number. Please fix it, or the frame 0 will be used as the sample frame.");
                        sampleFrameIndex = 0;
                    }
                    sampleFrameTransformInverse = animation->GetKeyFrame(sampleFrameIndex).GetInverseFull();
                }
                
                for (size_t frame = 0; frame < numFrames; ++frame)
                {
                    const float time = aznumeric_cast<float>(frame * timeStep);
                    SceneAPIMatrixType boneTransform = animation->GetKeyFrame(frame + startFrame);
                    if (additiveRule)
                    {
                        // For additive motion, we stores the relative transform.
                        boneTransform = sampleFrameTransformInverse * boneTransform;
                    }

                    SceneAPIMatrixType boneTransformNoScale(boneTransform);
                    const AZ::Vector3 scale = boneTransformNoScale.ExtractScale();
                    const AZ::Transform convertedTransform = AZ::Transform::CreateFromMatrix3x4(coordSysConverter.ConvertMatrix3x4(boneTransformNoScale));
                    const AZ::Vector3 position = convertedTransform.GetTranslation();
                    const AZ::Quaternion rotation = convertedTransform.GetRotation();
                    
                    // Set the pose when this is the first frame.
                    // This is used as optimization so that poses or non-animated submotions do not need any key tracks.
                    if (frame == 0)
                    {
                        motionData->SetJointStaticPosition(jointDataIndex, position);
                        motionData->SetJointStaticRotation(jointDataIndex, rotation);
                        EMFX_SCALECODE
                        (
                            motionData->SetJointStaticScale(jointDataIndex, scale);
                        )
                    }

                    motionData->SetJointPositionSample(jointDataIndex, frame, {time, position});
                    motionData->SetJointRotationSample(jointDataIndex, frame, {time, rotation});
                    EMFX_SCALECODE
                    (
                        motionData->SetJointScaleSample(jointDataIndex, frame, {time, scale});
                    )
                }

                // Set the bind pose transform.
                SceneAPIMatrixType bindBoneTransformNoScale(bindSpaceLocalTransform);
                const AZ::Vector3    bindScale = coordSysConverter.ConvertScale(bindBoneTransformNoScale.ExtractScale());
                const AZ::Transform convertedbindTransform = AZ::Transform::CreateFromMatrix3x4(coordSysConverter.ConvertMatrix3x4(bindBoneTransformNoScale));
                const AZ::Vector3    bindPosition   = convertedbindTransform.GetTranslation();
                const AZ::Quaternion bindRotation = convertedbindTransform.GetRotation();

                motionData->SetJointBindPosePosition(jointDataIndex, bindPosition);
                motionData->SetJointBindPoseRotation(jointDataIndex, bindRotation);
                EMFX_SCALECODE
                (
                    motionData->SetJointBindPoseScale(jointDataIndex, bindScale);
                )
            } // End looping through bones and adding motion data.

            if (rootMotionExtractionRule && sampleJointDataIndex != InvalidJointDataIndex && rootJointDataIndex != InvalidJointDataIndex)
            {
                const auto& data = rootMotionExtractionRule->GetData();
                motionData->ExtractRootMotion(sampleJointDataIndex, rootJointDataIndex, *data);
            }

            if (coordinateSystemRule)
            {
                const float scaleFactor = coordinateSystemRule->GetScale();
                if (!AZ::IsClose(scaleFactor, 1.0f, FLT_EPSILON)) // If the scale factor is 1, no need to call Scale
                {
                    motionData->Scale(scaleFactor);
                }
            }

            // Process morphs.
            auto sceneGraphView = SceneViews::MakePairView(graph.GetNameStorage(), graph.GetContentStorage());
            auto sceneGraphDownardsIteratorView = SceneViews::MakeSceneGraphDownwardsView<SceneViews::BreadthFirst>(
                graph, graph.GetRoot(), sceneGraphView.begin(), true);

            auto iterator = sceneGraphDownardsIteratorView.begin();
            for (; iterator != sceneGraphDownardsIteratorView.end(); ++iterator)
            {
                SceneContainers::SceneGraph::HierarchyStorageConstIterator hierarchy = iterator.GetHierarchyIterator();
                [[maybe_unused]] SceneContainers::SceneGraph::NodeIndex currentIndex = graph.ConvertToNodeIndex(hierarchy);
                AZ_Assert(currentIndex.IsValid(), "While iterating through the Scene Graph an unexpected invalid entry was found.");
                AZStd::shared_ptr<const SceneDataTypes::IGraphObject> currentItem = iterator->second;
                if (hierarchy->IsEndPoint())
                {
                    if (currentItem->RTTI_IsTypeOf(SceneDataTypes::IBlendShapeAnimationData::TYPEINFO_Uuid()))
                    {
                        const SceneDataTypes::IBlendShapeAnimationData* blendShapeAnimationData = static_cast<const SceneDataTypes::IBlendShapeAnimationData*>(currentItem.get());

                        const size_t morphDataIndex = motionData->AddMorph(blendShapeAnimationData->GetBlendShapeName(), 0.0f);
                        const size_t keyFrameCount = blendShapeAnimationData->GetKeyFrameCount();
                        motionData->AllocateMorphSamples(morphDataIndex, keyFrameCount);
                        const double keyFrameStep = blendShapeAnimationData->GetTimeStepBetweenFrames();
                        for (int keyFrameIndex = 0; keyFrameIndex < keyFrameCount; keyFrameIndex++)
                        {
                            const float keyFrameValue = static_cast<float>(blendShapeAnimationData->GetKeyFrame(keyFrameIndex));
                            const float keyframeTime = static_cast<float>(keyFrameIndex * keyFrameStep);
                            motionData->SetMorphSample(morphDataIndex, keyFrameIndex, {keyframeTime, keyFrameValue});
                        }
                    }
                }
            }

            // Add missing keyframes at the end of the animation to match all keytracks' duration.
            motionData->FixMissingEndKeyframes();

            // Let's prepare the motion data in the type we want.
            // This can later be extended with other types of motion data like least square fit curves etc.
            motionData->UpdateDuration();
            if (!motionData->VerifyIntegrity())
            {
                AZ_Error(SceneUtil::ErrorWindow, false, "Data integrity issue in '%s'.", motionGroup.GetName().c_str());
                return SceneEvents::ProcessingResult::Failure;
            }

            // Get the sample rate we have setup or that we have used.
            // Also make sure we don't sample at higher rate than we want.
            float sampleRate = 30.0f;
            if (lowestTimeStep > 0.0)
            {
                const float maxSampleRate = static_cast<float>(1.0 / lowestTimeStep);
                if (samplingRule && samplingRule->GetSampleRateMethod() == EMotionFX::Pipeline::Rule::MotionSamplingRule::SampleRateMethod::Custom)
                {
                    sampleRate = AZ::GetMin(maxSampleRate, samplingRule->GetCustomSampleRate());
                }
                else
                {
                    sampleRate = maxSampleRate;
                }
            }
            AZ_TracePrintf("EMotionFX", "Motion sample rate = %f", sampleRate);
            motionData->RemoveRedundantKeyframes(samplingRule ? !samplingRule->GetKeepDuration() : false); // Clear any tracks of non-animated parts.
            if (!motionData->VerifyIntegrity())
            {
                AZ_Error(SceneUtil::ErrorWindow, false, "Data integrity issue after removing redundant keyframes for '%s'.", motionGroup.GetName().c_str());
                return SceneEvents::ProcessingResult::Failure;
            }

            // Create the desired type of motion data, based on what is selected in the motion sampling rule.
            MotionData* finalMotionData = nullptr;
            const AZ::TypeId motionDataTypeId = samplingRule ? samplingRule->GetMotionDataTypeId() : AZ::TypeId::CreateNull();
            const bool isAutomaticMode = motionDataTypeId.IsNull();
            const MotionDataFactory& motionDataFactory = GetMotionManager().GetMotionDataFactory();
            if (isAutomaticMode) // Automatically pick a motion data type, based on the data size.
            {
                finalMotionData = AutoCreateMotionData(motionData, sampleRate, samplingRule.get(), rootJoints);
            }
            else if (motionDataFactory.IsRegisteredTypeId(motionDataTypeId)) // Yay, we found the typeId, so let's create it through the factory.
            {
                finalMotionData = motionDataFactory.Create(motionDataTypeId);
                AZ_Assert(finalMotionData, "Expected a valid motion data pointer.");
                if (finalMotionData)
                {
                    AZ_TracePrintf("EMotionFX", "*************** Created type = '%s' (%s)", motionDataTypeId.ToString<AZStd::string>().c_str(), finalMotionData->RTTI_GetTypeName());
                }
            }
            else // The type somehow isn't registered.
            {
                AZ_Warning("EMotionFX", false, "The motion data factory has no registered type with typeId %s", motionDataTypeId.ToString<AZStd::string>().c_str());
            }

            // Fall back to a default type.
            if (!finalMotionData)
            {
                AZ_Warning("EMotionFX", false, "As we failed to create a valid motion data type, we're using a fallback UniformMotionData.");
                finalMotionData = aznew UniformMotionData();
            }

            // Initialize the final motion data.
            // We already have done this one page above, when we are in automatic mode, so skip when we use automatic mode.
            if (!isAutomaticMode)
            {
                InitAndOptimizeMotionData(finalMotionData, motionData, sampleRate, samplingRule.get(), rootJoints);
            }

            if (!finalMotionData->VerifyIntegrity())
            {
                AZ_Error(SceneUtil::ErrorWindow, false, "Data integrity issue in the final animation for '%s'.", motionGroup.GetName().c_str());
                return SceneEvents::ProcessingResult::Failure;
            }

            // Delete the data that we created out of the Scene API as it is no longer needed as we already extracted all the data from it
            // into our finalMotionData.
            delete motionData;
            context.m_motion.SetMotionData(finalMotionData);

            // Set root motion extraction data on the motion itself, so we can later edit it in animation editor.
            AZStd::shared_ptr<EMotionFX::RootMotionExtractionData> rootMotionData;
            if (EMotionFX::Pipeline::Rule::LoadFromGroup<EMotionFX::Pipeline::Rule::RootMotionExtractionRule>(motionGroup, rootMotionData))
            {
                context.m_motion.SetRootMotionExtractionData(rootMotionData);
            }

            return SceneEvents::ProcessingResult::Success;
        }
    } // namespace Pipeline
} // namespace EMotionFX
