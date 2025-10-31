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
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneData/Groups/SkeletonGroup.h>
#include <SceneAPI/SceneData/Behaviors/SkeletonGroup.h>
#include <SceneAPI/SceneData/GraphData/RootBoneData.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Behaviors
        {
            void SkeletonGroup::Activate()
            {
            }

            void SkeletonGroup::Deactivate()
            {
            }

            void SkeletonGroup::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<SkeletonGroup, BehaviorComponent>()->Version(1);
                }
            }

            void SkeletonGroup::GetCategoryAssignments(CategoryRegistrationList& categories, const Containers::Scene& scene)
            {
                if (SceneHasSkeletonGroup(scene) || Utilities::DoesSceneGraphContainDataLike<DataTypes::IBoneData>(scene, false))
                {
                    categories.emplace_back("Rigs", SceneData::SkeletonGroup::TYPEINFO_Uuid(), s_rigsPreferredTabOrder);
                }
            }

            void SkeletonGroup::InitializeObject(const Containers::Scene& scene, DataTypes::IManifestObject& target)
            {
                if (!m_isDefaultConstructing && target.RTTI_IsTypeOf(SceneData::SkeletonGroup::TYPEINFO_Uuid()))
                {
                    SceneData::SkeletonGroup* group = azrtti_cast<SceneData::SkeletonGroup*>(&target);

                    group->SetName(DataTypes::Utilities::CreateUniqueName<DataTypes::ISkeletonGroup>(scene.GetName(), scene.GetManifest()));

                    const Containers::SceneGraph &graph = scene.GetGraph();
                    auto contentStorage = graph.GetContentStorage();
                    auto nameStorage = graph.GetNameStorage();
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
                }
            }

            Events::ProcessingResult SkeletonGroup::UpdateManifest(Containers::Scene& scene, ManifestAction action,
                RequestingApplication /*requester*/)
            {
                if (action == ManifestAction::ConstructDefault)
                {
                    return BuildDefault(scene);
                }
                else if (action == ManifestAction::Update)
                {
                    return UpdateSkeletonGroups(scene);
                }
                else
                {
                    return Events::ProcessingResult::Ignored;
                }
            }

            Events::ProcessingResult SkeletonGroup::BuildDefault(Containers::Scene& scene)
            {
                if (SceneHasSkeletonGroup(scene))
                {
                    return Events::ProcessingResult::Ignored;
                }

                const Containers::SceneGraph &graph = scene.GetGraph();
                auto contentStorage = graph.GetContentStorage();
                auto nameStorage = graph.GetNameStorage();
                auto nameContentView = Containers::Views::MakePairView(nameStorage, contentStorage);
                
                bool hasCreatedSkeletons = false;
                m_isDefaultConstructing = true;
                for (auto it = nameContentView.begin(); it != nameContentView.end(); ++it)
                {
                    if (!it->second || !it->second->RTTI_IsTypeOf(AZ::SceneData::GraphData::RootBoneData::TYPEINFO_Uuid()))
                    {
                        continue;
                    }

                    // Check if this is a virtual type. There are no known virtual types supported by skeletons so this skeleton
                    //      pretends to be something that's not understood by this behavior, so skip it.
                    Events::GraphMetaInfo::VirtualTypesSet virtualTypes;
                    Events::GraphMetaInfoBus::Broadcast(&Events::GraphMetaInfoBus::Events::GetVirtualTypes, virtualTypes, 
                        scene, graph.ConvertToNodeIndex(it.GetFirstIterator()));
                    if (!virtualTypes.empty())
                    {
                        continue;
                    }
                    
                    AZStd::shared_ptr<SceneData::SkeletonGroup> group = AZStd::make_shared<SceneData::SkeletonGroup>();
                    AZStd::string name = DataTypes::Utilities::CreateUniqueName<DataTypes::ISkeletonGroup>(scene.GetName(), it->first.GetName(), scene.GetManifest());
                    // This is a group that's generated automatically so may not be saved to disk but would need to be recreated
                    //      in the same way again. To guarantee the same uuid, generate a stable one instead.
                    group->OverrideId(DataTypes::Utilities::CreateStableUuid(scene, SceneData::SkeletonGroup::TYPEINFO_Uuid(), name));
                    group->SetName(AZStd::move(name));
                    group->SetSelectedRootBone(it->first.GetPath());

                    Events::ManifestMetaInfoBus::Broadcast(&Events::ManifestMetaInfoBus::Events::InitializeObject, scene, *group);

                    scene.GetManifest().AddEntry(AZStd::move(group));
                    hasCreatedSkeletons = true;
                }
                m_isDefaultConstructing = false;

                return hasCreatedSkeletons ? Events::ProcessingResult::Success : Events::ProcessingResult::Ignored;
            }

            Events::ProcessingResult SkeletonGroup::UpdateSkeletonGroups(Containers::Scene& scene) const
            {
                bool updated = false;
                Containers::SceneManifest& manifest = scene.GetManifest();
                auto valueStorage = manifest.GetValueStorage();
                auto view = Containers::MakeDerivedFilterView<SceneData::SkeletonGroup>(valueStorage);
                for (SceneData::SkeletonGroup& group : view)
                {
                    if (group.GetName().empty())
                    {
                        group.SetName(DataTypes::Utilities::CreateUniqueName<DataTypes::ISkeletonGroup>(scene.GetName(), scene.GetManifest()));
                        updated = true;
                    }
                    if (group.GetId().IsNull())
                    {
                        // When the uuid is null it's likely because the manifest has been updated from an older version. Include the 
                        // name of the group as there could be multiple groups.
                        group.OverrideId(DataTypes::Utilities::CreateStableUuid(scene, SceneData::SkeletonGroup::TYPEINFO_Uuid(), group.GetName()));
                        updated = true;
                    }
                }

                return updated ? Events::ProcessingResult::Success : Events::ProcessingResult::Ignored;
            }

            bool SkeletonGroup::SceneHasSkeletonGroup(const Containers::Scene& scene) const
            {
                const Containers::SceneManifest& manifest = scene.GetManifest();
                Containers::SceneManifest::ValueStorageConstData manifestData = manifest.GetValueStorage();
                auto skeletonGroup = AZStd::find_if(manifestData.begin(), manifestData.end(), Containers::DerivedTypeFilter<DataTypes::ISkeletonGroup>());
                return skeletonGroup != manifestData.end();
            }
        } // namespace Behaviors
    } // namespace SceneAPI
} // namespace AZ
