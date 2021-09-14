/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Core.h>
#include <Editor/View/Windows/Tools/UpgradeTool/ModelTraits.h>
#include <AzCore/Component/TickBus.h>

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        class Modifier
            : private AZ::SystemTickBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(Modifier, AZ::SystemAllocator, 0);

            Modifier
                ( const ModifyConfiguration& modification
                , AZStd::vector<AZ::Data::AssetInfo>&& assets
                , AZStd::function<void()> onComplete);

        private:
            enum class State
            {
                GatheringDependencies,
                ModifyingGraphs
            };

            State m_state = State::GatheringDependencies;
            size_t m_assetIndex = 0;
            AZStd::function<void()> m_onComplete;
            AZStd::vector<AZ::Data::AssetInfo> m_assets;
            AZStd::vector<size_t> m_dependencyOrderedIndicies;
            AZStd::unordered_map<size_t, AZStd::unordered_set<size_t>> m_dependencies;
            AZStd::vector<size_t> m_failures;
            ModifyConfiguration m_config;
            ModificationResult m_result;

            void GatherDependencies();
            const AZ::Data::AssetInfo& GetCurrentAsset() const;
            AZ::Data::Asset<AZ::Data::AssetData> LoadAsset();
            void SortGraphsByDependencies();
            void OnSystemTick() override;
            void TickGatherDependencies();
            void TickUpdateGraph();
        };
    }
}
