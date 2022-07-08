/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifdef HAVE_BENCHMARK
#include <CommonBenchmarkSetup.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace Multiplayer
{
    /*
     * Hierarchy of 16 entities: Parent -> Child_2 -> .. -> Child_16
     * By default the maximum size of a hierarchy is defined by bg_hierarchyEntityMaxLimit (16).
     */
    class ServerDeepHierarchyBenchmark : public HierarchyBenchmarkBase
    {
    public:
        const NetEntityId RootNetEntityId = NetEntityId{ 1 };
        const NetEntityId ChildNetEntityId = NetEntityId{ 2 };
        const NetEntityId ChildOfChildNetEntityId = NetEntityId{ 3 };

        void internalSetUp() override
        {
            HierarchyBenchmarkBase::internalSetUp();

            m_root = AZStd::make_unique<EntityInfo>((1), "root", RootNetEntityId, EntityInfo::Role::Root);
            CreateParent(*m_root);

            m_children = AZStd::make_unique<AZStd::vector<AZStd::shared_ptr<EntityInfo>>>();

            EntityInfo* parent = m_root.get();
            for (int i = 0; i < 15; ++i)
            {
                m_children->push_back(AZStd::make_shared<EntityInfo>((i + 2), "child", ChildNetEntityId, EntityInfo::Role::Child));
                CreateChildForParent(*m_children->back(), *parent);

                m_children->back()->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(parent->m_entity->GetId());
                parent = m_children->back().get();
            }
        }

        void internalTearDown() override
        {
            m_children.reset();
            m_root.reset();

            HierarchyBenchmarkBase::internalTearDown();
        }

        AZStd::unique_ptr<EntityInfo> m_root;
        AZStd::unique_ptr<AZStd::vector<AZStd::shared_ptr<EntityInfo>>> m_children;
    };

    BENCHMARK_DEFINE_F(ServerDeepHierarchyBenchmark, RebuildHierarchy)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto value : state)
        {
            ForceRebuildHierarchy(m_root->m_entity);
        }
    }

    BENCHMARK_REGISTER_F(ServerDeepHierarchyBenchmark, RebuildHierarchy)
        ->Unit(benchmark::kMicrosecond)
        ;

    // Should be roughly twice the time of @RebuildHierarchy benchmark
    BENCHMARK_DEFINE_F(ServerDeepHierarchyBenchmark, RebuildHierarchyRemoveAndAddLastChild)(benchmark::State& state)
    {
        const AZ::EntityId parentOfLastChild = m_children->at(m_children->size() - 2)->m_entity->GetId();

        for ([[maybe_unused]] auto value : state)
        {
            m_children->back()->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
            m_children->back()->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(parentOfLastChild);
        }
    }

    BENCHMARK_REGISTER_F(ServerDeepHierarchyBenchmark, RebuildHierarchyRemoveAndAddLastChild)
        ->Unit(benchmark::kMicrosecond)
        ;

    // Should be roughly twice the time of @RebuildHierarchy benchmark
    BENCHMARK_DEFINE_F(ServerDeepHierarchyBenchmark, RebuildHierarchyRemoveAndAddMiddleChild)(benchmark::State& state)
    {
        const AZ::EntityId parentOfMiddleChild = m_children->at(4)->m_entity->GetId();

        for ([[maybe_unused]] auto value : state)
        {
            m_children->at(5)->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
            m_children->at(5)->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(parentOfMiddleChild);
        }
    }

    BENCHMARK_REGISTER_F(ServerDeepHierarchyBenchmark, RebuildHierarchyRemoveAndAddMiddleChild)
        ->Unit(benchmark::kMicrosecond)
        ;

    // Should be roughly twice the time of @RebuildHierarchy benchmark
    BENCHMARK_DEFINE_F(ServerDeepHierarchyBenchmark, RebuildHierarchyRemoveAndAddFirstChild)(benchmark::State& state)
    {
        const AZ::EntityId rootId = m_children->front()->m_entity->GetId();

        for ([[maybe_unused]] auto value : state)
        {
            m_children->at(1)->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
            m_children->at(1)->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(rootId);
        }
    }

    BENCHMARK_REGISTER_F(ServerDeepHierarchyBenchmark, RebuildHierarchyRemoveAndAddFirstChild)
        ->Unit(benchmark::kMicrosecond)
        ;
}

#endif
