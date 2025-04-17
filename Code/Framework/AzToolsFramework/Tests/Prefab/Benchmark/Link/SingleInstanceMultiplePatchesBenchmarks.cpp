/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#if defined(HAVE_BENCHMARK)

#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <Prefab/Benchmark/Link/SingleInstanceMultiplePatchesBenchmarks.h>

#define REGISTER_MULTIPLE_PATCHES_BENCHMARK(BaseClass, Method)                                                                             \
    BENCHMARK_REGISTER_F(BaseClass, Method)                                                                                                \
        ->RangeMultiplier(10)                                                                                                              \
        ->Range(100, 10000)                                                                                                                \
        ->ArgNames({ "PatchesCount" })                                                                                                     \
        ->Unit(benchmark::kMillisecond);

namespace Benchmark
{
    using namespace AzToolsFramework::Prefab;

    void SingleInstanceMultiplePatchesBenchmarks::SetupHarness(const benchmark::State& state)
    {
        BM_Prefab::SetupHarness(state);
        
        const unsigned int numEntities = static_cast<unsigned int>(state.range());

        AZStd::vector<AZ::Entity*> entities;

        // The patch array generated in this function on transform component has 2 elements. To keep the patch count match the range,
        // we are only creating half the number of entities; (1 entity * 2 patches * n/2) = n patches, where n is the range.
        for (unsigned int i = 0; i < numEntities / 2; i++)
        {
            entities.emplace_back(CreateEntity("Entity"));
        }

        CreateFakePaths(2);
        AZStd::unique_ptr<AzToolsFramework::Prefab::Instance> nestedInstance =
            m_prefabSystemComponent->CreatePrefab(entities, {}, m_paths.front());

        m_parentInstance = m_prefabSystemComponent->CreatePrefab({}, MakeInstanceList(AZStd::move(nestedInstance)), m_paths.back());

        m_linkDomToSet = AZStd::make_unique<PrefabDom>();
        m_linkDomToSet->SetObject();
        PrefabDomValue patchesArray;
        patchesArray.SetArray();
        m_parentInstance->GetNestedInstances(
            [this, &patchesArray](AZStd::unique_ptr<Instance>& nestedInstance)
            {
                m_linkId = nestedInstance->GetLinkId();
                nestedInstance->GetEntities(
                    [this, &patchesArray](AZStd::unique_ptr<AZ::Entity>& entity)
                    {
                        PrefabDom entityDomBefore;
                        InstanceToTemplateInterface* instanceToTemplateInterface = AZ::Interface<InstanceToTemplateInterface>::Get();
                        AZ_Assert(instanceToTemplateInterface, "Could not retrieve instance of InstanceToTemplateInterface");
                        instanceToTemplateInterface->GenerateEntityDomBySerializing(entityDomBefore, *(entity.get()));

                        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldX, 10.0f);
                        PrefabDom entityDomAfter;
                        instanceToTemplateInterface->GenerateEntityDomBySerializing(entityDomAfter, *(entity.get()));

                        PrefabDom patch;
                        instanceToTemplateInterface->GeneratePatch(patch, entityDomBefore, entityDomAfter);
                        instanceToTemplateInterface->PrependEntityAliasPathToPatchPaths(patch, entity->GetId());
                        for (auto& entry : patch.GetArray())
                        {
                            PrefabDomValue patchEntryCopy;
                            patchEntryCopy.CopyFrom(entry, m_linkDomToSet->GetAllocator());
                            patchesArray.PushBack(patchEntryCopy.Move(), m_linkDomToSet->GetAllocator());
                        }
                        return true;
                    });
            });

        LinkReference link = m_prefabSystemComponent->FindLink(m_linkId);
        AZ_Assert(link.has_value(), "Link between prefabs is missing.");
        
        m_linkDomToSet->AddMember(
            rapidjson::StringRef(PrefabDomUtils::SourceName), rapidjson::StringRef(m_paths.front().c_str()), m_linkDomToSet->GetAllocator());

        m_linkDomToSet->AddMember(rapidjson::StringRef(PrefabDomUtils::PatchesName), patchesArray, m_linkDomToSet->GetAllocator());

        link->get().SetLinkDom(*m_linkDomToSet);
    }

    void SingleInstanceMultiplePatchesBenchmarks::TeardownHarness(const benchmark::State& state)
    {
        m_linkDomToSet.reset();
        m_parentInstance.reset();
        BM_Prefab::TeardownHarness(state);
    }

    BENCHMARK_DEFINE_F(SingleInstanceMultiplePatchesBenchmarks, GetLinkDom)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            LinkReference link = m_prefabSystemComponent->FindLink(m_linkId);
            AZ_Assert(link.has_value(), "Link between prefabs is missing.");
            PrefabDom linkDom;
            link->get().GetLinkDom(linkDom, linkDom.GetAllocator());
        }
    }
    REGISTER_MULTIPLE_PATCHES_BENCHMARK(SingleInstanceMultiplePatchesBenchmarks, GetLinkDom);

    BENCHMARK_DEFINE_F(SingleInstanceMultiplePatchesBenchmarks, SetLinkDom)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            LinkReference link = m_prefabSystemComponent->FindLink(m_linkId);
            AZ_Assert(link.has_value(), "Link between prefabs is missing.");
            link->get().SetLinkDom(*m_linkDomToSet);
        }
    }
    REGISTER_MULTIPLE_PATCHES_BENCHMARK(SingleInstanceMultiplePatchesBenchmarks, SetLinkDom);

} // namespace Benchmark
#endif
