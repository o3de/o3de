/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/View/Windows/Tools/UpgradeTool/Modifier.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>

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
            : m_config(modification)
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
                : m_assets[m_dependencyOrderedIndicies[m_assetIndex]];
        }

        void Modifier::GatherDependencies()
        {
            AZ::SerializeContext* serializeContext{};
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(serializeContext, "SerializeContext is required to enumerate dependent assets in the ScriptCanvas file");

            /*

            // AZStd::unordered_multimap<AZStd::string, AssetBuilderSDK::SourceFileDependency> jobDependenciesByKey;
            auto assetFilter = [this, &jobDependenciesByKey]
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
                        // AssetBuilderSDK::SourceFileDependency dependency;
                        // dependency.m_sourceFileDependencyUUID = subgraphAsset->GetId().m_guid;
                        // jobDependenciesByKey.insert({ s_scriptCanvasProcessJobKey, dependency });
                        // this->m_processEditorAssetDependencies.push_back
                        // ({ subgraphAsset->GetId(), azTypeId, AZ::Data::AssetLoadBehavior::PreLoad });
                    }
                }
                // always continue, make note of the script canvas dependencies
                return true;
            };

            AZ_Verify(serializeContext->EnumerateInstanceConst
                ( sourceGraph->GetGraphData()
                , azrtti_typeid<ScriptCanvas::GraphData>()
                , assetFilter
                , {}
                , AZ::SerializeContext::ENUM_ACCESS_FOR_READ
                , nullptr
                , nullptr), "Failed to gather dependencies from graph data");

            // Flush asset database events to ensure no asset references are held by closures queued on Ebuses.
            AZ::Data::AssetManager::Instance().DispatchEvents();
            */
        }

        AZ::Data::Asset<AZ::Data::AssetData> Modifier::LoadAsset()
        {
            AZ::Data::Asset<AZ::Data::AssetData> asset = AZ::Data::AssetManager::Instance().GetAsset
            (GetCurrentAsset().m_assetId
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
    }
}
