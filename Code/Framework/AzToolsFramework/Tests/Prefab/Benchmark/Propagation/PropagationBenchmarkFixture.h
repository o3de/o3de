/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#if defined(HAVE_BENCHMARK)

#pragma once

#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/ToolsComponents/EditorInspectorComponent.h>
#include <Prefab/Benchmark/PrefabBenchmarkFixture.h>

namespace Benchmark
{
    using namespace AzToolsFramework::Prefab;

    //! This class captures benchmarks for propagating changes to a single prefab instance with multiple entities that are side-by-side.
    class PropagationBenchmarkFixture : public Benchmark::BM_Prefab
    {
    protected:
        void UpdateTemplate();
        void UpdateComponent(benchmark::State& state);
        void AddComponent(benchmark::State& state);
        void RemoveComponent(benchmark::State& state);
        void AddEntity(benchmark::State& state);
        void RemoveEntity(benchmark::State& state);
        void AddNestedInstance(benchmark::State& state, TemplateId nestedPrefabTemplateId);
        void RemoveNestedInstance(benchmark::State& state, TemplateId nestedPrefabTemplateId);

        AZ::Entity* m_entityModify;
        AZStd::unique_ptr<Instance> m_instanceCreated;
        AZStd::unique_ptr<Instance> m_instanceToUseForPropagation;
    };

} // namespace Benchmark

#endif
