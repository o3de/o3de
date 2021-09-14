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
            , AZStd::vector<AZ::Data::AssetInfo>&& assets
            , AZStd::function<void()> onComplete)
            : m_state(State::GatheringDependencies)
            , m_config(modification)
            , m_assets(assets)
            , m_onComplete(onComplete)
        {
            ModelNotificationsBus::Broadcast(&ModelNotificationsTraits::OnUpgradeAllBegin);
            AZ::SystemTickBus::Handler::BusConnect();
        }

        const AZ::Data::AssetInfo& Modifier::GetCurrentAsset() const
        {
            return m_state == State::GatheringDependencies
                ? m_assets[m_assetIndex]
                : m_assets[m_dependencyOrderedAssetIndicies[m_assetIndex]];
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

        void Modifier::GatherDependencies()
        {
            AZ::SerializeContext* serializeContext{};
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(serializeContext, "SerializeContext is required to enumerate dependent assets in the ScriptCanvas file");

            auto asset = LoadAsset();
            if (!asset
            || !asset.GetAs<ScriptCanvasAsset>()
            || !asset.GetAs<ScriptCanvasAsset>()->GetScriptCanvasGraph()
            || !asset.GetAs<ScriptCanvasAsset>()->GetScriptCanvasGraph()->GetGraphData())
            {
                VE_LOG("Modifier: Failed to load asset %s for modification, even though it scanned properly");
                return;
            }

            auto graphData = asset.GetAs<ScriptCanvasAsset>()->GetScriptCanvasGraph()->GetGraphData();

            auto dependencyGrabber = [this]
                ( void* instancePointer
                , const AZ::SerializeContext::ClassData* classData
                , [[maybe_unused]] const AZ::SerializeContext::ClassElement* classElement)
            {
                auto azTypeId = classData->m_azRtti->GetTypeId();
                if (azTypeId == azrtti_typeid<AZ::Data::Asset<ScriptCanvas::SubgraphInterfaceAsset>>())
                {
                    const auto* subgraphAsset = reinterpret_cast<AZ::Data::Asset<const ScriptCanvas::SubgraphInterfaceAsset>*>(instancePointer);
                    if (subgraphAsset->GetId().IsValid())
                    {
                        if (auto iter = m_assetInfoIndexById.find(subgraphAsset->GetId().m_guid); iter != m_assetInfoIndexById.end())
                        {
                            GetOrCreateDependencyIndexSet().insert(iter->second);
                        }
                        else
                        {
                            VE_LOG("Modifier: Dependency found that was not picked up by the scanner: %s"
                                , subgraphAsset->GetId().ToString<AZStd::string>().c_str());
                        }
                    }
                }
                // always continue, make note of the script canvas dependencies
                return true;
            };

            AZ_Verify(serializeContext->EnumerateInstanceConst
                ( graphData
                , azrtti_typeid<ScriptCanvas::GraphData>()
                , dependencyGrabber
                , {}
                , AZ::SerializeContext::ENUM_ACCESS_FOR_READ
                , nullptr
                , nullptr), "Failed to gather dependencies from graph data");

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
            /*
            L ← Empty list that will contain the sorted nodes
            (m_dependencyOrderedAssetIndicies)

               while exists nodes without a permanent mark do
                   select an unmarked node n
                   visit(n)
            */
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

        void Modifier::SortGraphsByDependencies()
        {
            m_dependencyOrderedAssetIndicies.reserve(m_assets.size());
            Sorter sorter;
            sorter.modifier = this;
            sorter.Sort();           
        }

        void Modifier::TickGatherDependencies()
        {
            if (m_assetIndex == 0)
            {
                if (m_config.successfulDependencyUpgradeRequired)
                {
                    ModelNotificationsBus::Broadcast(&ModelNotificationsTraits::OnUpgradeAllDependencySortBegin);
                    m_assetInfoIndexById.reserve(m_assets.size());

                    for (size_t index = 0; index != m_assets.size(); ++index)
                    {
                        m_assetInfoIndexById.insert({ m_assets[index].m_assetId.m_guid, index });
                    }
                }
                else
                {
                    m_dependencyOrderedAssetIndicies.reserve(m_assets.size());

                    for (size_t index = 0; index != m_assets.size(); ++index)
                    {
                        m_dependencyOrderedAssetIndicies.push_back(index);
                    }
                }
            }

            if (m_assetIndex == m_assets.size())
            {
                SortGraphsByDependencies();
                ModelNotificationsBus::Broadcast
                    ( &ModelNotificationsTraits::OnUpgradeAllDependencySortEnd
                    , m_assets
                    , m_dependencyOrderedAssetIndicies);
                m_assetIndex = 0;
                m_state = State::ModifyingGraphs;
            }
            else
            {
                GatherDependencies();
                ++m_assetIndex;
            }
        }
    }
}
