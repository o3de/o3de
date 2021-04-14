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
#include <AssImpTypeConverter.h>
#include <assimp/mesh.h>
#include <assimp/scene.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/FbxSceneBuilder/FbxSceneSystem.h>
#include <SceneAPI/FbxSceneBuilder/ImportContexts/AssImpImportContexts.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpAnimationImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpImporterUtilities.h>
#include <SceneAPI/FbxSceneBuilder/Importers/Utilities/RenamedNodesMap.h>
#include <SceneAPI/FbxSDKWrapper/FbxAnimCurveWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxAnimLayerWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxNodeWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxSceneWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxTimeSpanWrapper.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneData/GraphData/AnimationData.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            const char* AssImpAnimationImporter::s_animationNodeName = "animation";

            // Downstream only supports 30 frames per second sample rate. Adjusting to 60 doubles the
            // length of the animations, they still play back at 30 frames per second.
            const double AssImpAnimationImporter::s_defaultTimeStepBetweenFrames = 1.0 / 30.0;

            AZ::u32 GetNumKeyFrames(const AZ::u32 keysSize, const double duration, const double ticksPerSecond)
            {
                const double totalTicks = duration / ticksPerSecond;
                AZ::u32 numKeys = keysSize;
                double totalFramesAtDefaultTimeStep = totalTicks / AssImpAnimationImporter::s_defaultTimeStepBetweenFrames;
                if (!AZ::IsClose(totalFramesAtDefaultTimeStep, numKeys, 1))
                {
                    numKeys = AZStd::ceilf(totalFramesAtDefaultTimeStep) + 1; // +1 because the animation is from [0, duration] - we have a keyframe at the end of the duration which needs to be included
                }
                return numKeys;
            }

            double GetTimeForFrame(const AZ::u32 frame, const double ticksPerSecond)
            {
                return frame * AssImpAnimationImporter::s_defaultTimeStepBetweenFrames * ticksPerSecond;
            }

            // Helper class to store key data, when translating from AssImp layout to the engine's scene format.
            struct KeyData
            {
                KeyData(float value, float time) :
                    mValue(value),
                    mTime(time)
                {

                }

                bool operator<(const KeyData& other) const
                {
                    return mTime < other.mTime;
                }

                float mValue = 0;
                float mTime = 0;
            };

            template<class T>
            void LerpTemplate(T& start, const T& end, float t)
            {
                start = start * (1.0f - t) + end * t;
            }

            template<>
            void LerpTemplate(aiQuaternion& start, const aiQuaternion& end, float t)
            {
                aiQuaternion::Interpolate(start, start, end, t);
            }

            template<>
            void LerpTemplate(float& start, const float& end, float t)
            {
                start = AZ::Lerp(start, end, t);
            }

            template<class T, class T2>
            bool SampleKeyFrame(T2& result, const T& keys, AZ::u32 numKeys, double time, AZ::u32& lastIndex)
            {
                if (numKeys == 0)
                {
                    AZ_Error("AnimationImporter", numKeys > 0, "Animation key set must have at least 1 key");
                    return false;
                }
                if (numKeys == 1)
                {
                    result = keys[0].mValue;
                    return true;
                }

                while (lastIndex < numKeys - 1 && time >= keys[lastIndex + 1].mTime)
                {
                    ++lastIndex;
                }
                result = keys[lastIndex].mValue;
                if (lastIndex < numKeys - 1)
                {
                    auto nextValue = keys[lastIndex + 1].mValue;
                    float normalizedTimeBetweenFrames = 0;
                    if (keys[lastIndex + 1].mTime != keys[lastIndex].mTime)
                    {
                        normalizedTimeBetweenFrames =
                            (time - keys[lastIndex].mTime) / (keys[lastIndex + 1].mTime - keys[lastIndex].mTime);
                    }
                    else
                    {
                        AZ_Warning("AnimationImporter", false,
                            "Duplicates has keys with duplicate time, at indices %d and %d. The second will be ignored.",
                            lastIndex,
                            lastIndex + 1);
                    }
                    LerpTemplate(result, nextValue, normalizedTimeBetweenFrames);
                }
                return true;
            }

            AssImpAnimationImporter::AssImpAnimationImporter()
            {
                BindToCall(&AssImpAnimationImporter::ImportAnimation);
            }

            void AssImpAnimationImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<AssImpAnimationImporter, SceneCore::LoadingComponent>()->Version(2); // [LYN-2281] Skinned mesh loading fixes
                }
            }

            AZStd::string GetName(const aiMeshAnim* anim)
            {
                return anim->mName.C_Str();
            }

            AZStd::string GetName(const aiNodeAnim* anim)
            {
                return anim->mNodeName.C_Str();
            }

            AZStd::string GetName(const aiMeshMorphAnim* anim)
            {
                return anim->mName.C_Str();
            }

            struct ConsolidatedNodeAnim
            {
                ConsolidatedNodeAnim() = default;
                ConsolidatedNodeAnim(const ConsolidatedNodeAnim& other) = delete;

                ConsolidatedNodeAnim(ConsolidatedNodeAnim&& other) noexcept
                    : mNumPositionKeys{ other.mNumPositionKeys },
                      mPositionKeys{ other.mPositionKeys },
                      mNumRotationKeys{ other.mNumRotationKeys },
                      mRotationKeys{ other.mRotationKeys },
                      mNumScalingKeys{ other.mNumScalingKeys },
                      mScalingKeys{ other.mScalingKeys },
                      m_ownedPositionKeys{ std::move(other.m_ownedPositionKeys) },
                      m_ownedScalingKeys{ std::move(other.m_ownedScalingKeys) },
                      m_ownedRotationKeys{ std::move(other.m_ownedRotationKeys) },
                      mPreState{ other.mPreState },
                      mPostState{ other.mPostState }
                {
                }

                ConsolidatedNodeAnim& operator=(const ConsolidatedNodeAnim& other) = delete;

                ConsolidatedNodeAnim& operator=(ConsolidatedNodeAnim&& other) noexcept
                {
                    if (this == &other)
                    {
                        return *this;
                    }

                    mNumPositionKeys = other.mNumPositionKeys;
                    mPositionKeys = other.mPositionKeys;
                    mNumRotationKeys = other.mNumRotationKeys;
                    mRotationKeys = other.mRotationKeys;
                    mNumScalingKeys = other.mNumScalingKeys;
                    mScalingKeys = other.mScalingKeys;
                    m_ownedPositionKeys = std::move(other.m_ownedPositionKeys);
                    m_ownedScalingKeys = std::move(other.m_ownedScalingKeys);
                    m_ownedRotationKeys = std::move(other.m_ownedRotationKeys);
                    mPreState = other.mPreState;
                    mPostState = other.mPostState;

                    return *this;
                }

                // These variables are named using AssImp naming convention for consistency
                AZ::u32 mNumPositionKeys{};
                aiVectorKey* mPositionKeys{};
                
                AZ::u32 mNumRotationKeys{};
                aiQuatKey* mRotationKeys{};

                AZ::u32 mNumScalingKeys{};
                aiVectorKey* mScalingKeys{};
                
                AZStd::vector<aiVectorKey> m_ownedPositionKeys{};
                AZStd::vector<aiVectorKey> m_ownedScalingKeys{};
                AZStd::vector<aiQuatKey> m_ownedRotationKeys{};

                aiAnimBehaviour mPreState{};
                aiAnimBehaviour mPostState{};
            };

            AZStd::pair<const aiAnimation*, ConsolidatedNodeAnim> MakeMyPair(const aiAnimation* animation, const aiNodeAnim* anim)
            {
                ConsolidatedNodeAnim consolidatedNodeAnim{};

                consolidatedNodeAnim.mNumRotationKeys = anim->mNumRotationKeys;
                consolidatedNodeAnim.mNumPositionKeys = anim->mNumPositionKeys;
                consolidatedNodeAnim.mNumScalingKeys = anim->mNumScalingKeys;
                consolidatedNodeAnim.mRotationKeys = anim->mRotationKeys;
                consolidatedNodeAnim.mPositionKeys = anim->mPositionKeys;
                consolidatedNodeAnim.mScalingKeys = anim->mScalingKeys;
                
                return {animation, AZStd::move(consolidatedNodeAnim)};
            }

            auto MakeMyPair(const aiAnimation* animation, const aiMeshAnim* anim)
            {
                return AZStd::make_pair(animation, anim);
            }

            auto MakeMyPair(const aiAnimation* animation, const aiMeshMorphAnim* anim)
            {
                return AZStd::make_pair(animation, anim);
            }

            Events::ProcessingResult AssImpAnimationImporter::ImportAnimation(AssImpSceneNodeAppendedContext& context)
            {
                AZ_TraceContext("Importer", "Animation");

                aiNode* currentNode = context.m_sourceNode.GetAssImpNode();
                const aiScene* scene = context.m_sourceScene.GetAssImpScene();

                // Add check for animation layers at the scene level.
                
                if (!scene->HasAnimations() || IsPivotNode(currentNode->mName))
                {
                    return Events::ProcessingResult::Ignored;
                }

                AZStd::unordered_multimap<AZStd::string, AZStd::pair<const aiAnimation*, ConsolidatedNodeAnim>> boneAnimations;
                typedef AZStd::pair<const aiAnimation*, const aiMeshMorphAnim*> AnimAndMorphAnim;
                typedef AZStd::unordered_map<AZStd::string, AnimAndMorphAnim> ChannelToMorphAnim;
                typedef AZStd::unordered_map<AZStd::string, ChannelToMorphAnim> NodeToChannelToMorphAnim;
                NodeToChannelToMorphAnim meshMorphAnimations;

                // Goes through all the animation channels of a given type and adds them to a map so we can easily find
                // all the animations for a given node
                // In the case of bone animations, the data is copied into a ConsolidatedNodeAnim so we can
                // do fix-ups later without affecting the original data
                auto mapAnimationsFunc = [](AZ::u32 numChannels, auto** channels, const aiAnimation* animation, auto& map)
                {
                    map.reserve(numChannels);

                    for (AZ::u32 channelIndex = 0; channelIndex < numChannels; ++channelIndex)
                    {
                        auto* nodeAnim = channels[channelIndex];
                        AZStd::string name = GetName(nodeAnim);

                        map.emplace(name, MakeMyPair(animation, nodeAnim));
                    }
                };

                for (AZ::u32 animIndex = 0; animIndex < scene->mNumAnimations; ++animIndex)
                {
                    const aiAnimation* animation = scene->mAnimations[animIndex];
                    if (animation->mTicksPerSecond == 0)
                    {
                        AZ_Error(
                            "AnimationImporter", false,
                            "Animation name %s has a speed of 0 ticks per second and cannot be processed.",
                            animation->mName.C_Str());
                        return Events::ProcessingResult::Failure;
                    }
                    
                    mapAnimationsFunc(animation->mNumChannels, animation->mChannels, animation, boneAnimations);

                    for (AZ::u32 channelIndex = 0; channelIndex < animation->mNumMorphMeshChannels; ++channelIndex)
                    {
                        auto* nodeAnim = animation->mMorphMeshChannels[channelIndex];
                        AZStd::string name = GetName(nodeAnim);

                        AZStd::vector<AZStd::string> meshNodeNameAndChannel;

                        // Morph target animations include the channel in the name,
                        // so if a mesh is named Mesh01, the morph target for the first channel
                        // will be named Mesh01*0
                        AZ::StringFunc::Tokenize(name, meshNodeNameAndChannel, '*');

                        if (meshNodeNameAndChannel.size() != 2)
                        {
                            AZ_Error(
                                "AnimationImporter", false,
                                "Morph animation name %s was not in the expected format of: node name, asterisk, node channel. "
                                "Example: 'NodeName*0'",
                                name.c_str());
                            continue;
                        }

                        AZStd::string meshNodeName(meshNodeNameAndChannel[0]);
                        AZStd::string channel(meshNodeNameAndChannel[1]);
                        meshMorphAnimations[meshNodeName][channel] = AnimAndMorphAnim(animation, nodeAnim);
                    }
                }

                // Go through all the bone animations and find any that reference a pivot node
                // We'll make a new node anim and store all the combined animation channels there
                // with the name set to the base bone name
                decltype(boneAnimations) combinedAnimations;

                for (auto&& animation : boneAnimations)
                {
                    size_t pos = AZ::StringFunc::Find(animation.first, PivotNodeMarker, 0);
                    
                    if (pos != animation.first.npos)
                    {
                        AZStd::string_view name, part;
                        SplitPivotNodeName(aiString(animation.first.c_str()), pos, name, part);
                        
                        auto&& iterator = combinedAnimations.find(name);
                    
                        if (iterator == combinedAnimations.end())
                        {
                            combinedAnimations.emplace(AZStd::string(name), AZStd::move(animation.second));
                            iterator = combinedAnimations.find(name);
                        }
                    
                        if (iterator != combinedAnimations.end())
                        {
                            auto& existingNode = iterator->second.second;
                            auto&& src = animation.second.second;
                            
                            if (part == "Translation")
                            {
                                existingNode.mNumPositionKeys = src.mNumPositionKeys;
                                existingNode.mPositionKeys = src.mPositionKeys;
                            }
                            else if (part == "Rotation")
                            {
                                existingNode.mNumRotationKeys = src.mNumRotationKeys;
                                existingNode.mRotationKeys = src.mRotationKeys;
                            }
                            else if (part == "Scaling")
                            {
                                existingNode.mNumScalingKeys = src.mNumScalingKeys;
                                existingNode.mScalingKeys = src.mScalingKeys;
                            }
                        }
                    }
                }

                if (!combinedAnimations.empty())
                {
                    boneAnimations.swap(combinedAnimations);
                }

                Events::ProcessingResultCombiner combinedAnimationResult;
                for (AZ::u32 meshIndex = 0; meshIndex < currentNode->mNumMeshes; ++meshIndex)
                {
                    aiMesh* mesh = scene->mMeshes[currentNode->mMeshes[meshIndex]];

                    if (NodeToChannelToMorphAnim::iterator channelsForMeshName = meshMorphAnimations.find(mesh->mName.C_Str());
                        channelsForMeshName != meshMorphAnimations.end())
                    {
                        const auto [nodeIterName, channels] = *channelsForMeshName;
                        for (const auto& [channel, animAndMorphAnim] : channels)
                        {
                            const auto& [animation, morphAnimation] = animAndMorphAnim;
                            combinedAnimationResult += ImportBlendShapeAnimation(
                                context, animation, morphAnimation, mesh);
                        }
                    }
                }

                AZStd::string nodeName = s_animationNodeName;
                RenamedNodesMap::SanitizeNodeName(nodeName, context.m_scene.GetGraph(), context.m_currentGraphPosition);
                AZ_TraceContext("Animation node name", nodeName);

                // If there are no bone animations, but there are mesh animations,
                // then a stub animation needs to be created so the exporter can create the exported morph target animation.
                if (boneAnimations.empty() && !meshMorphAnimations.empty())
                {
                    const aiAnimation* animation = scene->mAnimations[0];

                    // Morph animations need a regular animation on the node, as well.
                    // If there is no bone animation on the current node, then generate one here.
                    AZStd::shared_ptr<SceneData::GraphData::AnimationData> createdAnimationData =
                        AZStd::make_shared<SceneData::GraphData::AnimationData>();
                    createdAnimationData->ReserveKeyFrames(
                        animation->mDuration +
                        1); // +1 because we start at 0 and the last keyframe is at mDuration instead of mDuration-1
                    createdAnimationData->SetTimeStepBetweenFrames(1.0 / animation->mTicksPerSecond);

                    // Set every frame of the animation to the start location of the node.
                    aiMatrix4x4 combinedTransform = GetConcatenatedLocalTransform(currentNode);
                    DataTypes::MatrixType localTransform = AssImpSDKWrapper::AssImpTypeConverter::ToTransform(combinedTransform);
                    context.m_sourceSceneSystem.SwapTransformForUpAxis(localTransform);
                    context.m_sourceSceneSystem.ConvertUnit(localTransform);
                    for (AZ::u32 time = 0; time <= animation->mDuration; ++time)
                    {
                        createdAnimationData->AddKeyFrame(localTransform);
                    }

                    Containers::SceneGraph::NodeIndex addNode = context.m_scene.GetGraph().AddChild(
                        context.m_currentGraphPosition, nodeName.c_str(), AZStd::move(createdAnimationData));
                    context.m_scene.GetGraph().MakeEndPoint(addNode);

                    return combinedAnimationResult.GetResult();
                }
                decltype(boneAnimations) parentFillerAnimations;

                // Go through all the animations and make sure we create animations for bones who's parents don't have an animation
                for (auto&& anim : boneAnimations)
                {
                    aiNode* node = scene->mRootNode->FindNode(anim.first.c_str());
                    aiNode* parent = node->mParent;

                    while (parent && parent != scene->mRootNode)
                    {
                        if (!IsPivotNode(parent->mName))
                        {
                            if (boneAnimations.find(parent->mName.C_Str()) == boneAnimations.end() &&
                                parentFillerAnimations.find(parent->mName.C_Str()) == parentFillerAnimations.end())
                            {
                                // Create 1 key for each type that just copies the current transform
                                ConsolidatedNodeAnim emptyAnimation;
                                aiMatrix4x4 globalTransform = GetConcatenatedLocalTransform(parent);

                                aiVector3D position, scale;
                                aiQuaternion rotation;

                                globalTransform.Decompose(scale, rotation, position);

                                emptyAnimation.mNumRotationKeys = emptyAnimation.mNumPositionKeys = emptyAnimation.mNumScalingKeys = 1;
                                
                                emptyAnimation.m_ownedPositionKeys.emplace_back(0, position);
                                emptyAnimation.mPositionKeys = emptyAnimation.m_ownedPositionKeys.data();

                                emptyAnimation.m_ownedRotationKeys.emplace_back(0, rotation);
                                emptyAnimation.mRotationKeys = emptyAnimation.m_ownedRotationKeys.data();

                                emptyAnimation.m_ownedScalingKeys.emplace_back(0, scale);
                                emptyAnimation.mScalingKeys = emptyAnimation.m_ownedScalingKeys.data();
                                
                                parentFillerAnimations.insert(
                                    AZStd::make_pair(parent->mName.C_Str(), AZStd::make_pair(anim.second.first, AZStd::move(emptyAnimation))));
                            }
                        }

                        parent = parent->mParent;
                    }
                }

                boneAnimations.insert(AZStd::make_move_iterator(parentFillerAnimations.begin()), AZStd::make_move_iterator(parentFillerAnimations.end()));

                auto animItr = boneAnimations.equal_range(currentNode->mName.C_Str());

                if (animItr.first == animItr.second)
                {
                    return combinedAnimationResult.GetResult();
                }
                
                bool onlyOne = false;

                for (auto it = animItr.first; it != animItr.second; ++it)
                {
                    if (onlyOne)
                    {
                        AZ_Error("AnimationImporter", false, "Bone %s has multiple animations.  Only 1 animation per bone is supported", currentNode->mName.C_Str());
                        break;
                    }

                    const aiAnimation* animation = it->second.first;
                    const ConsolidatedNodeAnim* anim = &it->second.second;

                    // We don't currently handle having a different number of keys, with 1 exception:
                    // 1 key is essentially a constant so we do handle that case
                    if ((anim->mNumPositionKeys != anim->mNumRotationKeys && anim->mNumPositionKeys > 1 && anim->mNumRotationKeys > 1) ||
                        (anim->mNumPositionKeys != anim->mNumScalingKeys && anim->mNumPositionKeys > 1 && anim->mNumScalingKeys > 1) ||
                        (anim->mNumRotationKeys != anim->mNumScalingKeys && anim->mNumRotationKeys > 1 && anim->mNumScalingKeys > 1))
                    {
                        AZ_Error("AnimationImporter", false, "Bone Animation with different number of position (%d)/rotation (%d)/scaling (%d) keys not supported",
                            anim->mNumPositionKeys, anim->mNumRotationKeys, anim->mNumScalingKeys);
                        return Events::ProcessingResult::Failure;
                    }

                    // Resample the animations at a fixed time step. This matches the behaviour of
                    // the previous SDK used. Longer term, this could be data driven, or based on the
                    // smallest time step between key frames.
                    // AssImp has an animation->mTicksPerSecond and animation->mDuration, but those
                    // are less predictable than just using a fixed time step.
                    // AssImp documentation claims animation->mDuration is the duration of the animation in ticks, but
                    // not all animations we've tested follow that pattern. Sometimes duration is in seconds.
                    const AZ::u32 numKeyFrames = GetNumKeyFrames(
                        AZStd::max(AZStd::max(anim->mNumScalingKeys, anim->mNumPositionKeys), anim->mNumRotationKeys),
                        animation->mDuration,
                        animation->mTicksPerSecond);

                    AZStd::shared_ptr<SceneData::GraphData::AnimationData> createdAnimationData =
                       AZStd::make_shared<SceneData::GraphData::AnimationData>();
                    createdAnimationData->ReserveKeyFrames(numKeyFrames);
                    createdAnimationData->SetTimeStepBetweenFrames(s_defaultTimeStepBetweenFrames);

                    AZ::u32 lastScaleIndex = 0;
                    AZ::u32 lastPositionIndex = 0;
                    AZ::u32 lastRotationIndex = 0;
                    for (AZ::u32 frame = 0; frame < numKeyFrames; ++frame)
                    {
                        const double time = GetTimeForFrame(frame, animation->mTicksPerSecond);
                        aiVector3D scale = aiVector3D(1.f, 1.f, 1.f), position = aiVector3D(0.f, 0.f, 0.f);
                        aiQuaternion rotation(1.f, 0.f, 0.f, 0.f);
                        if (!SampleKeyFrame(scale, anim->mScalingKeys, anim->mNumScalingKeys, time, lastScaleIndex) ||
                            !SampleKeyFrame(position, anim->mPositionKeys, anim->mNumPositionKeys, time, lastPositionIndex) ||
                            !SampleKeyFrame(rotation, anim->mRotationKeys, anim->mNumRotationKeys, time, lastRotationIndex))
                        {
                            return Events::ProcessingResult::Failure;
                        }
                        
                        aiMatrix4x4 transform(scale, rotation, position);
                        
                        DataTypes::MatrixType animTransform = AssImpSDKWrapper::AssImpTypeConverter::ToTransform(transform);

                        context.m_sourceSceneSystem.SwapTransformForUpAxis(animTransform);
                        context.m_sourceSceneSystem.ConvertBoneUnit(animTransform);
                        
                        createdAnimationData->AddKeyFrame(animTransform);
                    }

                    Containers::SceneGraph::NodeIndex addNode = context.m_scene.GetGraph().AddChild(
                        context.m_currentGraphPosition, nodeName.c_str(), AZStd::move(createdAnimationData));
                    context.m_scene.GetGraph().MakeEndPoint(addNode);
                    
                    onlyOne = true;
                }
                
                combinedAnimationResult += Events::ProcessingResult::Success;
                return combinedAnimationResult.GetResult();
            }

            Events::ProcessingResult AssImpAnimationImporter::ImportBlendShapeAnimation(
                AssImpSceneNodeAppendedContext& context,
                const aiAnimation* animation,
                const aiMeshMorphAnim* meshMorphAnim,
                const aiMesh* mesh)
            {
                if (meshMorphAnim->mNumKeys == 0)
                {
                    return Events::ProcessingResult::Ignored;
                }
                Events::ProcessingResultCombiner combinedAnimationResult;

                // In:
                //  Key index
                //      Time
                //      Values in AssImp, Channel in FBX SDK
                //      Weights in AssImp, Values in FBX SDK
                //      Num values & weights

                // Out:
                // One BlendShapeAnimationData per value (Channel in FBX SDK) index
                //      SetTimeStepBetweenFrames set on the animation data
                //      Keyframes. Weights (Values in FBX SDK) per key time.
                //      Keyframes generated for every single frame of the animation.
                typedef AZStd::map<int, AZStd::vector<KeyData>> ValueToKeyDataMap;
                ValueToKeyDataMap valueToKeyDataMap;

                for (int keyIdx = 0; keyIdx < meshMorphAnim->mNumKeys; keyIdx++)
                {
                    aiMeshMorphKey& key = meshMorphAnim->mKeys[keyIdx];
                    for (int valIdx = 0; valIdx < key.mNumValuesAndWeights; ++valIdx)
                    {
                        int currentValue = key.mValues[valIdx];
                        KeyData thisKey(key.mWeights[valIdx], key.mTime);
                        valueToKeyDataMap[currentValue].insert(
                        AZStd::upper_bound(valueToKeyDataMap[currentValue].begin(), valueToKeyDataMap[currentValue].end(),thisKey),
                            thisKey);
                    }
                }

                for (const auto& [meshIdx, keys] : valueToKeyDataMap)
                {
                    AZStd::shared_ptr<SceneData::GraphData::BlendShapeAnimationData> morphAnimNode =
                        AZStd::make_shared<SceneData::GraphData::BlendShapeAnimationData>();

                    const AZ::u32 numKeyFrames = GetNumKeyFrames(keys.size(), animation->mDuration, animation->mTicksPerSecond);
                    morphAnimNode->ReserveKeyFrames(numKeyFrames);
                    morphAnimNode->SetTimeStepBetweenFrames(s_defaultTimeStepBetweenFrames);

                    aiAnimMesh* aiAnimMesh = mesh->mAnimMeshes[meshIdx];
                    AZStd::string_view nodeName(aiAnimMesh->mName.C_Str());

                    const AZ::u32 maxKeys = keys.size();
                    AZ::u32 keyIdx = 0;
                    for (AZ::u32 frame = 0; frame <= numKeyFrames; ++frame)
                    {
                        const double time = GetTimeForFrame(frame, animation->mTicksPerSecond);

                        float weight = 0;
                        if (!SampleKeyFrame(weight, keys, keys.size(), time, keyIdx))
                        {
                            return Events::ProcessingResult::Failure;
                        }

                        morphAnimNode->AddKeyFrame(weight);
                    }


                    const size_t dotIndex = nodeName.find_last_of('.');
                    nodeName = nodeName.substr(dotIndex + 1);

                    morphAnimNode->SetBlendShapeName(nodeName.data());

                    AZStd::string animNodeName(AZStd::string::format("%s_%s", s_animationNodeName, nodeName.data()));
                    Containers::SceneGraph::NodeIndex addNode = context.m_scene.GetGraph().AddChild(
                        context.m_currentGraphPosition, animNodeName.c_str(), AZStd::move(morphAnimNode));
                    context.m_scene.GetGraph().MakeEndPoint(addNode);

                }

                return Events::ProcessingResult::Success;
            }

        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
