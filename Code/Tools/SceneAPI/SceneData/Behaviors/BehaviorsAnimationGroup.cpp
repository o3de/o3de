/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IAnimationData.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneData/Groups/AnimationGroup.h>
#include <SceneAPI/SceneData/Behaviors/AnimationGroup.h>
#include <SceneAPI/SceneData/GraphData/RootBoneData.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Behaviors
        {
            void AnimationGroup::Activate()
            {       
            }

            void AnimationGroup::Deactivate()
            {
            }

            void AnimationGroup::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<AnimationGroup, BehaviorComponent>()->Version(1);
                }
            }

            void AnimationGroup::GetCategoryAssignments(CategoryRegistrationList& categories, const Containers::Scene& scene)
            {
                if (SceneHasAnimationGroup(scene) || Utilities::DoesSceneGraphContainDataLike<DataTypes::IAnimationData>(scene, false))
                {
                    categories.emplace_back("Animations", SceneData::AnimationGroup::TYPEINFO_Uuid(), s_animationsPreferredTabOrder);
                }
            }

            void AnimationGroup::InitializeObject(const Containers::Scene& scene, DataTypes::IManifestObject& target)
            {
                if (!target.RTTI_IsTypeOf(SceneData::AnimationGroup::TYPEINFO_Uuid()))
                {
                    return;
                }

                SceneData::AnimationGroup* group = azrtti_cast<SceneData::AnimationGroup*>(&target);
                group->SetName(DataTypes::Utilities::CreateUniqueName<DataTypes::IAnimationGroup>(scene.GetName(), scene.GetManifest()));
                const Containers::SceneGraph &graph = scene.GetGraph();
                auto nameStorage = graph.GetNameStorage();
                auto contentStorage = graph.GetContentStorage();
                
                auto nameContentView = Containers::Views::MakePairView(nameStorage, contentStorage);
                AZStd::string shallowestRootBoneName;
                auto graphDownwardsView = Containers::Views::MakeSceneGraphDownwardsView<Containers::Views::BreadthFirst>(graph, graph.GetRoot(), nameContentView.begin(), true);
                for (auto it = graphDownwardsView.begin(); it != graphDownwardsView.end(); ++it)
                {
                    if (!it->second)
                    {
                        continue;
                    }
                    if (it->second->RTTI_IsTypeOf(AZ::SceneData::GraphData::RootBoneData::TYPEINFO_Uuid()))
                    {
                        shallowestRootBoneName = it->first.GetPath();
                        break;
                    }
                }
                group->SetSelectedRootBone(shallowestRootBoneName);

                Containers::SceneGraph::ContentStorageConstData graphContent = graph.GetContentStorage();
                auto animationData = AZStd::find_if(graphContent.begin(), graphContent.end(), Containers::DerivedTypeFilter<DataTypes::IAnimationData>());
                if (animationData == graphContent.end())
                {
                    return;
                }
                const DataTypes::IAnimationData* animation = azrtti_cast<const DataTypes::IAnimationData*>(animationData->get());
                uint32_t frameCount = aznumeric_caster(animation->GetKeyFrameCount());
                group->SetStartFrame(0);
                group->SetEndFrame(frameCount > 0 ? frameCount - 1 : 0);
            }

            Events::ProcessingResult AnimationGroup::UpdateManifest(Containers::Scene& scene, ManifestAction action,
                RequestingApplication /*requester*/)
            {
                if (action == ManifestAction::ConstructDefault)
                {
                    return BuildDefault(scene);
                }
                else if (action == ManifestAction::Update)
                {
                    return UpdateAnimationGroups(scene);
                }
                else
                {
                    return Events::ProcessingResult::Ignored;
                }
            }

            Events::ProcessingResult AnimationGroup::BuildDefault(Containers::Scene& scene) const
            {
                if (SceneHasAnimationGroup(scene) || !Utilities::DoesSceneGraphContainDataLike<DataTypes::IAnimationData>(scene, true))
                {
                    return Events::ProcessingResult::Ignored;
                }

                // There are animations but no animation group, so add a default animation group to the manifest.
                AZStd::shared_ptr<SceneData::AnimationGroup> group = AZStd::make_shared<SceneData::AnimationGroup>();

                // This is a group that's generated automatically so may not be saved to disk but would need to be recreated
                //      in the same way again. To guarantee the same uuid, generate a stable one instead.
                group->OverrideId(DataTypes::Utilities::CreateStableUuid(scene, SceneData::AnimationGroup::TYPEINFO_Uuid()));

                EBUS_EVENT(Events::ManifestMetaInfoBus, InitializeObject, scene, *group);
                scene.GetManifest().AddEntry(AZStd::move(group));

                return Events::ProcessingResult::Success;
            }

            Events::ProcessingResult AnimationGroup::UpdateAnimationGroups(Containers::Scene& scene) const
            {
                bool updated = false;
                Containers::SceneManifest& manifest = scene.GetManifest();
                auto valueStorage = manifest.GetValueStorage();
                auto view = Containers::MakeDerivedFilterView<SceneData::AnimationGroup>(valueStorage);
                for (SceneData::AnimationGroup& group : view)
                {
                    if (group.GetName().empty())
                    {
                        group.SetName(DataTypes::Utilities::CreateUniqueName<DataTypes::IAnimationGroup>(scene.GetName(), scene.GetManifest()));
                        updated = true;
                    }
                    if (group.GetId().IsNull())
                    {
                        // When the uuid is null it's likely because the manifest has been updated from an older version. Include the 
                        // name of the group as there could be multiple groups.
                        group.OverrideId(DataTypes::Utilities::CreateStableUuid(scene, SceneData::AnimationGroup::TYPEINFO_Uuid(), group.GetName()));
                        updated = true;
                    }
                }

                return updated ? Events::ProcessingResult::Success : Events::ProcessingResult::Ignored;
            }

            bool AnimationGroup::SceneHasAnimationGroup(const Containers::Scene& scene) const
            {
                const Containers::SceneManifest& manifest = scene.GetManifest();
                Containers::SceneManifest::ValueStorageConstData manifestData = manifest.GetValueStorage();
                auto animationGroup = AZStd::find_if(manifestData.begin(), manifestData.end(), Containers::DerivedTypeFilter<DataTypes::IAnimationGroup>());
                return animationGroup != manifestData.end();
            }
        } // namespace Behaviors
    } // namespace SceneAPI
} // namespace AZ
