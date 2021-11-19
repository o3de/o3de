/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <Editor/View/Windows/Tools/UpgradeTool/FileSaver.h>
#include <Editor/View/Windows/Tools/UpgradeTool/ModelTraits.h>
#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        class Modifier
            : private AZ::SystemTickBus::Handler
            , private ModificationNotificationsBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(Modifier, AZ::SystemAllocator, 0);

            Modifier
                ( const ModifyConfiguration& modification
                , WorkingAssets&& assets
                , AZStd::function<void()> onComplete);

            const ModificationResults& GetResult() const;
            ModificationResults&& TakeResult();

        private:
            friend class Sorter;

            struct Sorter
            {
                Modifier* modifier;
                AZStd::unordered_set<size_t> markedPermanent;
                AZStd::unordered_set<size_t> markedTemporary;
                void Sort();

            private:
                void Visit(size_t index);
                const AZStd::unordered_set<size_t>* GetDependencies(size_t index) const;
            };

            enum class State
            {
                GatheringDependencies,
                ModifyingGraphs
            };

            enum class ModifyState
            {
                Idle,
                InProgress,
                Saving,
                ReportResult
            };

            AZStd::recursive_mutex m_mutex;

            // the two states reside in this class because the modification is only complete if the new source file saves out
            State m_state = State::GatheringDependencies;
            ModifyState m_modifyState = ModifyState::Idle;
            size_t m_assetIndex = 0;
            AZStd::function<void()> m_onComplete;
            // asset infos in scanned order
            WorkingAssets m_assets;
            // dependency sorted order indices into the asset vector
            AZStd::vector<size_t> m_dependencyOrderedAssetIndicies;
            // dependency indices by asset info index (only exist if graphs have them)
            AZStd::unordered_map<size_t, AZStd::unordered_set<size_t>> m_dependencies;
            AZStd::unordered_map<AZ::Uuid, size_t> m_assetInfoIndexById;
            AZStd::vector<size_t> m_failures;
            ModifyConfiguration m_config;
            ModificationResult m_result;
            ModificationResults m_results;
            AZStd::unique_ptr<FileSaver> m_fileSaver;
            FileSaveResult m_fileSaveResult;

            void GatherDependencies();
            const AZ::Data::AssetInfo& GetCurrentAsset() const;
            AZStd::unordered_set<size_t>& GetOrCreateDependencyIndexSet();
            AZ::Data::Asset<AZ::Data::AssetData> LoadAsset();
            void ModifyCurrentAsset();
            void ModifyNextAsset();
            void ModificationComplete(const ModificationResult& result) override;
            void ReportModificationError(AZStd::string_view report);
            void ReportModificationSuccess();
            void ReportSaveResult();
            void SaveModifiedGraph(const ModificationResult& result);
            void SortGraphsByDependencies();
            void OnFileSaveComplete(const FileSaveResult& result);
            void OnSystemTick() override;
            void TickGatherDependencies();
            void TickUpdateGraph();
        };
    }
}
