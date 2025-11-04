/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <Editor/View/Windows/Tools/UpgradeTool/FileSaver.h>
#include <Editor/View/Windows/Tools/UpgradeTool/ModelTraits.h>
#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        class Modifier final
            : public AZ::SystemTickBus::Handler
            , public ModificationNotificationsBus::Handler
            , public AzFramework::AssetSystemInfoBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(Modifier, AZ::SystemAllocator);

            Modifier
                ( const ModifyConfiguration& modification
                , AZStd::vector<SourceHandle>&& assets
                , AZStd::function<void()> onComplete);

            ~Modifier();

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
                WaitingForDependencyProcessing,
                StartModification,
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
            AZStd::vector<SourceHandle> m_assets;
            // dependency sorted order indices into the asset vector
            AZStd::vector<size_t> m_dependencyOrderedAssetIndicies;
            // dependency indices by asset info index (only exist if graphs have them)
            AZStd::unordered_map<size_t, AZStd::unordered_set<size_t>> m_dependencies;
            AZStd::unordered_map<AZ::Uuid, size_t> m_assetInfoIndexById;
            ModifyConfiguration m_config;
            ModificationResult m_result;
            ModificationResults m_results;
            AZStd::unique_ptr<FileSaver> m_fileSaver;
            FileSaveResult m_fileSaveResult;
            // m_attemptedAssets is assets attempted to be processed by modification, as opposed to
            // those processed by the AP as a result of one of their dependencies being processed.
            AZStd::unordered_set<AZ::Uuid> m_attemptedAssets;
            AZStd::unordered_set<AZ::Uuid> m_assetsCompletedByAP;
            AZStd::unordered_set<AZ::Uuid> m_assetsFailedByAP;
            AZStd::chrono::steady_clock::time_point m_waitLogTimeStamp;
            AZStd::chrono::steady_clock::time_point m_waitTimeStamp;
            AZStd::unordered_set<AZStd::string> m_successNotifications;
            AZStd::unordered_set<AZStd::string> m_failureNotifications;

            bool AllDependenciesCleared(const AZStd::unordered_set<size_t>& dependencies) const;
            bool AnyDependenciesFailed(const AZStd::unordered_set<size_t>& dependencies) const;
            void AssetCompilationSuccess(const AZStd::string& assetPath) override;
            void AssetCompilationFailed(const AZStd::string& assetPath) override;
            AZStd::sys_time_t CalculateRemainingWaitTime(const AZStd::unordered_set<size_t>& dependencies) const;
            void CheckDependencies();
            void GatherDependencies();
            size_t GetCurrentIndex() const;
            const AZStd::unordered_set<size_t>* GetDependencies(size_t index) const;
            AZStd::unordered_set<size_t>& GetOrCreateDependencyIndexSet();
            void InitializeResult();
            void LoadAsset();
            void ModificationComplete(const ModificationResult& result) override;
            void ModifyCurrentAsset();
            void NextAsset();
            void NextModification();
            void OnFileSaveComplete(const FileSaveResult& result);
            void OnSystemTick() override;
            void ProcessNotifications();
            void ReleaseCurrentAsset();
            void ReportModificationError(AZStd::string_view report);
            void ReportModificationSuccess();
            void ReportSaveResult();
            void SaveModifiedGraph(const ModificationResult& result);
            void SortGraphsByDependencies();
            void TickGatherDependencies();
            void TickUpdateGraph();
            void WaitForDependencies();
        };
    }
}
