/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Include/ScriptCanvas/Components/EditorGraph.h>
#include <Editor/View/Windows/Tools/UpgradeTool/LogTraits.h>
#include <Editor/View/Windows/Tools/UpgradeTool/Modifier.h>

#include <ScriptCanvas/Asset/SubgraphInterfaceAsset.h>
#include <ScriptCanvas/Assets/ScriptCanvasFileHandling.h>
#include <ScriptCanvas/Core/Graph.h>

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        Modifier::Modifier
            ( const ModifyConfiguration& modification
            , AZStd::vector<SourceHandle>&& assets
            , AZStd::function<void()> onComplete)
            : m_state(State::GatheringDependencies)
            , m_config(modification)
            , m_assets(assets)
            , m_onComplete(onComplete)
        {
            AZ_Assert(m_config.modification, "No modification function provided");
            ModelNotificationsBus::Broadcast(&ModelNotificationsTraits::OnUpgradeBegin, modification, m_assets);
            AZ::SystemTickBus::Handler::BusConnect();
            AzFramework::AssetSystemInfoBus::Handler::BusConnect();
            m_result.asset = m_assets[GetCurrentIndex()];
        }

        Modifier::~Modifier()
        {
            AzFramework::AssetSystemInfoBus::Handler::BusDisconnect();
        }

        bool Modifier::AllDependenciesCleared(const AZStd::unordered_set<size_t>& dependencies) const
        {
            for (auto index : dependencies)
            {
                SourceHandle dependency = m_assets[index];
                CompleteDescriptionInPlace(dependency);

                if (dependency.Id().IsNull() || !m_assetsCompletedByAP.contains(dependency.Id()))
                {
                    return false;
                }
            }

            return true;
        }

        bool Modifier::AnyDependenciesFailed(const AZStd::unordered_set<size_t>& dependencies) const
        {
            for (auto index : dependencies)
            {
                SourceHandle dependency = m_assets[index];
                CompleteDescriptionInPlace(dependency);

                if (dependency.Id().IsNull() || m_assetsFailedByAP.contains(dependency.Id()))
                {
                    return true;
                }
            }

            return false;
        }

        void Modifier::AssetCompilationSuccess([[maybe_unused]] const AZStd::string& assetPath)
        {
            const AZStd::string assetPathValue(assetPath);
            AZ::SystemTickBus::QueueFunction([this, assetPathValue]()
            {
                m_successNotifications.insert(assetPathValue);
            });
        }

        void Modifier::AssetCompilationFailed(const AZStd::string& assetPath)
        {
            const AZStd::string assetPathValue(assetPath);
            AZ::SystemTickBus::QueueFunction([this, assetPathValue]()
            {
                m_failureNotifications.insert(assetPathValue);
            });
        }

        AZStd::sys_time_t Modifier::CalculateRemainingWaitTime(const AZStd::unordered_set<size_t>& dependencies) const
        {
            auto maxSeconds = AZStd::chrono::seconds(dependencies.size() * m_config.perDependencyWaitSecondsMax);
            auto waitedSeconds = AZStd::chrono::duration_cast<AZStd::chrono::seconds>(AZStd::chrono::steady_clock::now() - m_waitTimeStamp);
            return (maxSeconds - waitedSeconds).count();
        }

        void Modifier::CheckDependencies()
        {
            ModelNotificationsBus::Broadcast(&ModelNotificationsTraits::OnUpgradeModificationBegin, m_config, m_result.asset);

            if (auto dependencies = GetDependencies(GetCurrentIndex()); dependencies != nullptr && !dependencies->empty())
            {
                VE_LOG
                    ( "dependencies found for %s, update will wait for the AP to finish processing them"
                    , m_result.asset.RelativePath().c_str());

                m_waitTimeStamp = AZStd::chrono::steady_clock::now();
                m_waitLogTimeStamp = AZStd::chrono::steady_clock::time_point{};
                m_modifyState = ModifyState::WaitingForDependencyProcessing;
            }
            else
            {
                m_modifyState = ModifyState::StartModification;
            }
        }

        void Modifier::GatherDependencies()
        {
            AZ::SerializeContext* serializeContext{};
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(serializeContext, "SerializeContext is required to enumerate dependent assets in the ScriptCanvas file");

            LoadAsset();
            bool anyFailures = false;

            if (m_result.asset.Get() && m_result.asset.Mod()->GetGraphData())
            {
                auto graphData = m_result.asset.Mod()->GetGraphData();

                auto dependencyGrabber = [this]
                    ( void* instancePointer
                    , const AZ::SerializeContext::ClassData* classData
                    , [[maybe_unused]] const AZ::SerializeContext::ClassElement* classElement)
                {
                    if (auto azTypeId = classData->m_azRtti->GetTypeId();
                        azTypeId == azrtti_typeid<AZ::Data::Asset<ScriptCanvas::SubgraphInterfaceAsset>>())
                    {
                        const auto* subgraphAsset =
                            reinterpret_cast<AZ::Data::Asset<const ScriptCanvas::SubgraphInterfaceAsset>*>(instancePointer);
                        if (subgraphAsset->GetId().IsValid())
                        {
                            if (auto iter = m_assetInfoIndexById.find(subgraphAsset->GetId().m_guid); iter != m_assetInfoIndexById.end())
                            {
                                // insert the index of the dependency into the set that belongs to this asset
                                GetOrCreateDependencyIndexSet().insert(iter->second);
                            }
                        }
                    }
                    // always continue, make note of the script canvas dependencies
                    return true;
                };

                if (!serializeContext->EnumerateInstanceConst
                    ( graphData
                    , azrtti_typeid<ScriptCanvas::GraphData>()
                    , dependencyGrabber
                    , {}
                    , AZ::SerializeContext::ENUM_ACCESS_FOR_READ
                    , nullptr
                    , nullptr))
                {
                    anyFailures = true;
                    VE_LOG("Modifier: ERROR - Failed to gather dependencies from graph data: %s"
                        , m_result.asset.RelativePath().c_str())
                }
            }
            else
            {
                anyFailures = true;
                VE_LOG("Modifier: ERROR - Failed to load asset %s for modification, even though it scanned properly"
                    , m_result.asset.RelativePath().c_str());
            }

            ModelNotificationsBus::Broadcast
                ( &ModelNotificationsTraits::OnUpgradeDependenciesGathered
                , m_result.asset
                , anyFailures ? Result::Failure : Result::Success);
        }

        size_t Modifier::GetCurrentIndex() const
        {
            return m_state == State::GatheringDependencies
                ? m_assetIndex
                : m_dependencyOrderedAssetIndicies[m_assetIndex];
        }

        const AZStd::unordered_set<size_t>* Modifier::GetDependencies(size_t index) const
        {
            auto iter = m_dependencies.find(index);
            return iter != m_dependencies.end() ? &iter->second : nullptr;
        }

        AZStd::unordered_set<size_t>& Modifier::GetOrCreateDependencyIndexSet()
        {
            auto iter = m_dependencies.find(m_assetIndex);
            if (iter == m_dependencies.end())
            {
                iter = m_dependencies.insert_or_assign(m_assetIndex, AZStd::unordered_set<size_t>()).first;
            }

            return iter->second;
        }

        const ModificationResults& Modifier::GetResult() const
        {
            return m_results;
        }

        void Modifier::InitializeResult()
        {
            m_result = {};

            if (m_assetIndex != m_assets.size())
            {
                m_result.asset = m_assets[GetCurrentIndex()];
                m_attemptedAssets.insert(m_result.asset.Id());
            }
        }

        void Modifier::LoadAsset()
        {
            auto& handle = m_result.asset;
            if (!handle.IsGraphValid())
            {
                auto result = ScriptCanvas::LoadFromFile(handle.AbsolutePath().c_str());
                if (result)
                {
                    handle = result.m_handle;
                }
            }
        }

        void Modifier::ModificationComplete(const ModificationResult& result)
        {
            if (!result.errorMessage.empty())
            {
                ReportModificationError(result.errorMessage);
            }
            else if (m_result.asset.Describe() != result.asset.Describe())
            {
                ReportModificationError("Received modification complete notification for different result");
            }
            else
            {
                SaveModifiedGraph(result);
            }
        }

        void Modifier::ModifyCurrentAsset()
        {
            LoadAsset();

            if (m_result.asset.IsGraphValid())
            {
                ModificationNotificationsBus::Handler::BusConnect();
                m_modifyState = ModifyState::InProgress;
                m_config.modification(m_result.asset);
            }
            else
            {
                ReportModificationError("Failed to load during modification");
            }
        }

        void Modifier::NextAsset()
        {
            ++m_assetIndex;
            InitializeResult();
        }

        void Modifier::NextModification()
        {
            ModelNotificationsBus::Broadcast( &ModelNotificationsTraits::OnUpgradeModificationEnd, m_config, m_result.asset, m_result);
            ModificationNotificationsBus::Handler::BusDisconnect();
            NextAsset();
            m_fileSaveResult = {};
            m_modifyState = ModifyState::Idle;
        }

        void Modifier::OnFileSaveComplete(const FileSaveResult& result)
        {
            if (!result.tempFileRemovalError.empty())
            {
                VE_LOG
                    ( "Temporary file not removed for %s: %s"
                    , m_result.asset.RelativePath().c_str()
                    , result.tempFileRemovalError.c_str());
            }

            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
            m_modifyState = ModifyState::ReportResult;
            m_fileSaver.reset();
            m_fileSaveResult = result;
        }

        void Modifier::OnSystemTick()
        {
            switch (m_state)
            {
            case State::GatheringDependencies:
                TickGatherDependencies();
                break;

            case State::ModifyingGraphs:
                TickUpdateGraph();
                break;
            }

            AZ::Data::AssetManager::Instance().DispatchEvents();
            AZ::SystemTickBus::ExecuteQueuedEvents();
        }

        void Modifier::ProcessNotifications()
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);

            for (const auto& assetPath : m_successNotifications)
            {
                VE_LOG("received AssetCompilationSuccess: %s", assetPath.c_str());
                auto sourceHandle = SourceHandle::FromRelativePath(nullptr, AZ::Uuid::CreateNull(), assetPath.c_str());
                CompleteDescriptionInPlace(sourceHandle);

                if (m_attemptedAssets.contains(sourceHandle.Id()))
                {
                    m_assetsCompletedByAP.insert(sourceHandle.Id());
                }
            }

            m_successNotifications.clear();

            for (const auto& assetPath : m_failureNotifications)
            {
                VE_LOG("received AssetCompilationFailed: %s", assetPath.c_str());
                auto sourceHandle = SourceHandle::FromRelativePath(nullptr, AZ::Uuid::CreateNull(), assetPath.c_str());
                CompleteDescriptionInPlace(sourceHandle);

                if (m_attemptedAssets.contains(sourceHandle.Id()))
                {
                    m_assetsFailedByAP.insert(sourceHandle.Id());
                }
            }

            m_failureNotifications.clear();
        }

        void Modifier::ReleaseCurrentAsset()
        {
            m_result.asset = m_result.asset.Describe();
            // Flush asset database events to ensure no asset references are held by closures queued on Ebuses.
            AZ::Data::AssetManager::Instance().DispatchEvents();
        }

        void Modifier::ReportModificationError(AZStd::string_view report)
        {
            m_result.errorMessage = report;
            m_results.m_failures.push_back({ m_result.asset.Describe(), report });
            m_assetsFailedByAP.insert(m_result.asset.Id());
            NextModification();
        }

        void Modifier::ReportModificationSuccess()
        {
            using namespace AzFramework;
            // \note DO NOT put asset into the m_assetsCompletedByAP here. That can only be done when the message is received by the AP
            m_results.m_successes.push_back(m_result.asset.Describe());
            AssetSystemRequestBus::Broadcast(&AssetSystem::AssetSystemRequests::EscalateAssetByUuid, m_result.asset.Id());
            NextModification();
        }

        void Modifier::ReportSaveResult()
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
            m_fileSaver.reset();

            if (m_fileSaveResult.IsSuccess())
            {
                ReportModificationSuccess();
            }
            else
            {
                ReportModificationError(m_fileSaveResult.fileSaveError);
            }
        }

        void Modifier::SaveModifiedGraph(const ModificationResult& result)
        {
            m_modifyState = ModifyState::Saving;
            m_fileSaver = AZStd::make_unique<FileSaver>
                    ( m_config.onReadOnlyFile
                    , [this](const FileSaveResult& fileSaveResult) { OnFileSaveComplete(fileSaveResult); });
            m_fileSaver->Save(result.asset, result.asset.AbsolutePath());
        }

        void Modifier::SortGraphsByDependencies()
        {
            m_dependencyOrderedAssetIndicies.reserve(m_assets.size());
            Sorter sorter;
            sorter.modifier = this;
            sorter.Sort();
        }

        ModificationResults&& Modifier::TakeResult()
        {
            return AZStd::move(m_results);
        }

        void Modifier::TickGatherDependencies()
        {
            if (m_assetIndex == 0)
            {
                if (m_config.successfulDependencyUpgradeRequired)
                {
                    ModelNotificationsBus::Broadcast(&ModelNotificationsTraits::OnUpgradeDependencySortBegin, m_config, m_assets);
                    m_assetInfoIndexById.reserve(m_assets.size());

                    for (size_t index = 0; index != m_assets.size(); ++index)
                    {
                        m_assetInfoIndexById.insert({ m_assets[index].Id(), index });
                    }
                }
                else
                {
                    m_dependencyOrderedAssetIndicies.reserve(m_assets.size());

                    for (size_t index = 0; index != m_assets.size(); ++index)
                    {
                        m_dependencyOrderedAssetIndicies.push_back(index);
                    }

                    // go straight into ModifyingGraphs
                    m_assetIndex = m_assets.size();
                }
            }

            if (m_assetIndex == m_assets.size())
            {
                if (m_config.successfulDependencyUpgradeRequired)
                {
                    SortGraphsByDependencies();
                    ModelNotificationsBus::Broadcast
                        ( &ModelNotificationsTraits::OnUpgradeDependencySortEnd
                        , m_config
                        , m_assets
                        , m_dependencyOrderedAssetIndicies);
                }

                m_assetIndex = 0;
                m_state = State::ModifyingGraphs;
                InitializeResult();
            }
            else
            {
                GatherDependencies();
                NextAsset();
            }
        }

        void Modifier::TickUpdateGraph()
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);

            switch (m_modifyState)
            {
            case ScriptCanvasEditor::VersionExplorer::Modifier::ModifyState::Idle:
                if (m_assetIndex == m_assets.size())
                {
                    VE_LOG("Modifier: Complete.");
                    AZ::SystemTickBus::Handler::BusDisconnect();

                    if (m_onComplete)
                    {
                        m_onComplete();
                    }
                }
                else
                {
                    CheckDependencies();
                }
                break;
            case ScriptCanvasEditor::VersionExplorer::Modifier::ModifyState::WaitingForDependencyProcessing:
                WaitForDependencies();
                break;
            case ScriptCanvasEditor::VersionExplorer::Modifier::ModifyState::StartModification:
                ModifyCurrentAsset();
                break;
            case ScriptCanvasEditor::VersionExplorer::Modifier::ModifyState::ReportResult:
                ReportSaveResult();
                break;
            default:
                break;
            }
        }

        void Modifier::WaitForDependencies()
        {
            const AZ::s32 LogPeriodSeconds = 5;

            ProcessNotifications();

            auto dependencies = GetDependencies(GetCurrentIndex());
            if (dependencies == nullptr || dependencies->empty() || AllDependenciesCleared(*dependencies))
            {
                m_modifyState = ModifyState::StartModification;
            }
            else if (AnyDependenciesFailed(*dependencies))
            {
                ReportModificationError("A required dependency failed to update, graph cannot update.");
            }
            else if (AZStd::chrono::seconds(CalculateRemainingWaitTime(*dependencies)).count() < 0)
            {
                ReportModificationError("Dependency update time has taken too long, aborting modification.");
            }
            else if (AZStd::chrono::duration_cast<AZStd::chrono::seconds>(AZStd::chrono::steady_clock::now() - m_waitLogTimeStamp).count() > LogPeriodSeconds)
            {
                m_waitLogTimeStamp = AZStd::chrono::steady_clock::now();

                AZ_TracePrintf
                    ( ScriptCanvas::k_VersionExplorerWindow.data()
                    , "Waiting for dependencies for %d more seconds: %s"
                    , AZStd::chrono::seconds(CalculateRemainingWaitTime(*dependencies)).count()
                    , m_result.asset.RelativePath().c_str());

                ModelNotificationsBus::Broadcast(&ModelNotificationsTraits::OnUpgradeDependencyWaitInterval, m_result.asset);
            }
        }

        const AZStd::unordered_set<size_t>* Modifier::Sorter::GetDependencies(size_t index) const
        {
            return modifier->GetDependencies(index);
        }

        void Modifier::Sorter::Sort()
        {
            for (size_t index = 0; index != modifier->m_assets.size(); ++index)
            {
                Visit(index);
            }
        }

        void Modifier::Sorter::Visit(size_t index)
        {
            if (markedPermanent.contains(index))
            {
                return;
            }

            if (markedTemporary.contains(index))
            {
                AZ_Error
                    ( ScriptCanvas::k_VersionExplorerWindow.data()
                    , false
                    , "Modifier: Dependency sort has failed during, circular dependency detected for Asset: %s"
                    , modifier->m_result.asset.RelativePath().c_str());
                return;
            }

            markedTemporary.insert(index);

            if (auto dependencies = GetDependencies(index))
            {
                for (auto& dependency : *dependencies)
                {
                    Visit(dependency);
                }
            }

            markedTemporary.erase(index);
            markedPermanent.insert(index);
            modifier->m_dependencyOrderedAssetIndicies.push_back(index);
        }
    }
}
