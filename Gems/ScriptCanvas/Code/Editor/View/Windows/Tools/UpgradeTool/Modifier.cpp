/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/View/Windows/Tools/UpgradeTool/LogTraits.h>
#include <Editor/View/Windows/Tools/UpgradeTool/Modifier.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <ScriptCanvas/Core/Graph.h>

namespace ModifierCpp
{

}

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        Modifier::Modifier
            ( const ModifyConfiguration& modification
            , WorkingAssets&& assets
            , AZStd::function<void()> onComplete)
            : m_state(State::GatheringDependencies)
            , m_config(modification)
            , m_assets(assets)
            , m_onComplete(onComplete)
        {
            AZ_Assert(m_config.modification, "No modification function provided");
            ModelNotificationsBus::Broadcast(&ModelNotificationsTraits::OnUpgradeBegin, modification, m_assets);
            AZ::SystemTickBus::Handler::BusConnect();
        }

        const AZ::Data::AssetInfo& Modifier::GetCurrentAsset() const
        {
            return m_state == State::GatheringDependencies
                ? m_assets[m_assetIndex].info
                : m_assets[m_dependencyOrderedAssetIndicies[m_assetIndex]].info;
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
            
        void Modifier::GatherDependencies()
        {
            AZ::SerializeContext* serializeContext{};
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(serializeContext, "SerializeContext is required to enumerate dependent assets in the ScriptCanvas file");

            bool anyFailures = false;
            auto asset = LoadAsset();

            if (asset
            && asset.GetAs<ScriptCanvasAsset>()
            && asset.GetAs<ScriptCanvasAsset>()->GetScriptCanvasGraph()
            && asset.GetAs<ScriptCanvasAsset>()->GetScriptCanvasGraph()->GetGraphData())
            {
                auto graphData = asset.GetAs<ScriptCanvasAsset>()->GetScriptCanvasGraph()->GetGraphData();

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
                        , GetCurrentAsset().m_relativePath.c_str())
                }
            }
            else
            {
                anyFailures = true;
                VE_LOG("Modifier: ERROR - Failed to load asset %s for modification, even though it scanned properly"
                    , GetCurrentAsset().m_relativePath.c_str());
            }
            
            ModelNotificationsBus::Broadcast
                ( &ModelNotificationsTraits::OnUpgradeDependenciesGathered
                , GetCurrentAsset()
                , anyFailures ? Result::Failure : Result::Success);

            // Flush asset database events to ensure no asset references are held by closures queued on Ebuses.
            AZ::Data::AssetManager::Instance().DispatchEvents();
        }

        AZ::Data::Asset<AZ::Data::AssetData> Modifier::LoadAsset()
        {
            AZ::Data::Asset<AZ::Data::AssetData> asset = AZ::Data::AssetManager::Instance().GetAsset
                ( GetCurrentAsset().m_assetId
                , azrtti_typeid<ScriptCanvasAsset>()
                , AZ::Data::AssetLoadBehavior::PreLoad);

            asset.BlockUntilLoadComplete();

            if (asset.IsReady())
            {
                return asset;
            }
            else
            {
                return {};
            }
        }

        void Modifier::ModificationComplete(const ModificationResult& result)
        {
            m_result = result;

            if (result.errorMessage.empty())
            {
                SaveModifiedGraph(result);
            }
            else
            {
                ReportModificationError(result.errorMessage);
            }
        }

        void Modifier::ModifyCurrentAsset()
        {
            m_result = {};
            m_result.assetInfo = GetCurrentAsset();

            ModelNotificationsBus::Broadcast(&ModelNotificationsTraits::OnUpgradeModificationBegin, m_config, GetCurrentAsset());

            if (auto asset = LoadAsset())
            {
                ModificationNotificationsBus::Handler::BusConnect();
                m_modifyState = ModifyState::InProgress;
                m_config.modification(asset);
            }
            else
            {
                ReportModificationError("Failed to load during modification");
            }
        }

        void Modifier::ModifyNextAsset()
        {
            ModelNotificationsBus::Broadcast
                ( &ModelNotificationsTraits::OnUpgradeModificationEnd, m_config, GetCurrentAsset(), m_result);
            ModificationNotificationsBus::Handler::BusDisconnect();
            m_modifyState = ModifyState::Idle;
            ++m_assetIndex;
            m_result = {};
        }

        void Modifier::ReportModificationError(AZStd::string_view report)
        {
            m_result.asset = {};
            m_result.errorMessage = report;
            m_results.m_failures.push_back(m_result);
            ModifyNextAsset();
        }

        void Modifier::ReportModificationSuccess()
        {
            m_results.m_successes.push_back(m_result.assetInfo);
            ModifyNextAsset();
        }

        void Modifier::ReportSaveResult()
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
            m_fileSaver.reset();

            if (m_fileSaveResult.fileSaveError.empty())
            {
                ReportModificationSuccess();
            }
            else
            {
                ReportModificationError(m_fileSaveResult.fileSaveError);
            }

            m_fileSaveResult = {};
            m_modifyState = ModifyState::Idle;
        }

        void Modifier::OnFileSaveComplete(const FileSaveResult& result)
        {
            if (!result.tempFileRemovalError.empty())
            {
                VE_LOG
                    ( "Temporary file not removed for %s: %s"
                    , m_result.assetInfo.m_relativePath.c_str()
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

        void Modifier::SaveModifiedGraph(const ModificationResult& result)
        {
            m_modifyState = ModifyState::Saving;
            m_fileSaver = AZStd::make_unique<FileSaver>
                    ( m_config.onReadOnlyFile
                    , [this](const FileSaveResult& result) { OnFileSaveComplete(result); });
            m_fileSaver->Save(result.asset);
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
                        m_assetInfoIndexById.insert({ m_assets[index].info.m_assetId.m_guid, index });
                    }
                }
                else
                {
                    m_dependencyOrderedAssetIndicies.reserve(m_assets.size());

                    for (size_t index = 0; index != m_assets.size(); ++index)
                    {
                        m_dependencyOrderedAssetIndicies.push_back(index);
                    }

                    // go straight into ModifyinGraphs
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
            }
            else
            {
                GatherDependencies();
                ++m_assetIndex;
            }
        }

        void Modifier::TickUpdateGraph()
        {
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
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);

                switch (m_modifyState)
                {
                case ScriptCanvasEditor::VersionExplorer::Modifier::ModifyState::Idle:
                    ModifyCurrentAsset();
                    break;
                case ScriptCanvasEditor::VersionExplorer::Modifier::ModifyState::ReportResult:
                    ReportSaveResult();
                    break;
                default:
                    break;
                }
            }
        }

        const AZStd::unordered_set<size_t>* Modifier::Sorter::GetDependencies(size_t index) const
        {
            auto iter = modifier->m_dependencies.find(index);
            return iter != modifier->m_dependencies.end() ? &iter->second : nullptr;
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
                (ScriptCanvas::k_VersionExplorerWindow.data()
                    , false
                    , "Modifier: Dependency sort has failed during, circular dependency detected for Asset: %s"
                    , modifier->GetCurrentAsset().m_relativePath.c_str());
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
