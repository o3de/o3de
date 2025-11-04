/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/EntityUtils.h>
#include <ScriptCanvas/Core/Connection.h>
#include <ScriptCanvas/Core/GraphData.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    class Entity;
}

namespace ScriptCanvas
{
#if defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)////
    class GraphDataEventHandler : public AZ::SerializeContext::IEventHandler
    {
    public:
        /// Called to rebuild the Endpoint map
        void OnWriteEnd(void* classPtr) override
        {
            reinterpret_cast<GraphData*>(classPtr)->OnDeserialized();
        }
    };
#endif//defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)

    void GraphData::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // On-demand reflect the previously used unordered_set to ensure the version converter works.
            using DependentAssetSet = AZStd::unordered_set<AZStd::tuple<AZ::EntityId, AZ::TypeId, AZ::Data::AssetId>>;
            auto genericInfo = AZ::SerializeGenericTypeInfo<DependentAssetSet>::GetGenericInfo();
            genericInfo->Reflect(serializeContext);

            serializeContext->Class<GraphData>()
                ->Version(4, &GraphData::VersionConverter)
#if defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)////
                ->EventHandler<GraphDataEventHandler>()
#endif//defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)
                ->Field("m_nodes", &GraphData::m_nodes)
                ->Field("m_connections", &GraphData::m_connections)
                ->Field("m_dependentAssets", &GraphData::m_dependentAssets)
                ->Field("m_scriptEventAssets", &GraphData::m_scriptEventAssets)
                ;
        }
    }

    bool GraphData::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement)
    {
        enum
        {
            FixedDependentAssetContainerType = 3
        };

        if (rootElement.GetVersion() == 0)
        {
            int connectionsIndex = rootElement.FindElement(AZ_CRC_CE("m_connections"));
            if (connectionsIndex < 0)
            {
                return false;
            }

            AZ::SerializeContext::DataElementNode& entityElement = rootElement.GetSubElement(connectionsIndex);
            AZStd::unordered_set<AZ::Entity*> entitiesSet;
            if (!entityElement.GetData(entitiesSet))
            {
                return false;
            }

            AZStd::vector<AZ::Entity*> entitiesVector(entitiesSet.begin(), entitiesSet.end());
            rootElement.RemoveElement(connectionsIndex);
            if (rootElement.AddElementWithData(context, "m_connections", entitiesVector) == -1)
            {
                return false;
            }

            for (AZ::Entity* entity : entitiesSet)
            {
                delete entity;
            }
        }

        if (rootElement.GetVersion() < FixedDependentAssetContainerType)
        {
            using DependentAssetSet = AZStd::unordered_set<AZStd::tuple<AZ::EntityId, AZ::TypeId, AZ::Data::AssetId>>;

            int dependentAssetsIndex = rootElement.FindElement(AZ_CRC_CE("m_dependentAssets"));
            if (dependentAssetsIndex < 0)
            {
                return true;
            }

            AZ::SerializeContext::DataElementNode& dataElement = rootElement.GetSubElement(dependentAssetsIndex);

            DependentAssetSet dependentAssetSet;
            DependentAssets dependentAssetMap{};
            if (dataElement.GetData(dependentAssetSet))
            {
                for (const auto& entry : dependentAssetSet)
                {
                    if (dependentAssetMap.find(AZStd::get<2>(entry)) == dependentAssetMap.end())
                    {
                        dependentAssetMap[AZStd::get<2>(entry)] = AZStd::make_pair(AZStd::get<0>(entry), AZStd::get<1>(entry));
                    }
                }
            }

            // Remove the old version
            rootElement.RemoveElement(dependentAssetsIndex);

            if (!dependentAssetMap.empty())
            {
                if (rootElement.AddElementWithData(context, "m_dependentAssets", dependentAssetMap) == -1)
                {
                    return false;
                }
            }
        }

        return true;
    }

    GraphData::GraphData(GraphData&& other)
        : m_nodes(AZStd::move(other.m_nodes))
        , m_connections(AZStd::move(other.m_connections))
        , m_endpointMap(AZStd::move(other.m_endpointMap))
        , m_dependentAssets(AZStd::move(other.m_dependentAssets))
        , m_scriptEventAssets(AZStd::move(other.m_scriptEventAssets))
    {
        other.m_nodes.clear();
        other.m_connections.clear();
        other.m_endpointMap.clear();
        other.m_dependentAssets.clear();
        other.m_scriptEventAssets.clear();
    }

    GraphData& GraphData::operator=(GraphData&& other)
    {
        if (this != &other)
        {
            m_nodes = AZStd::move(other.m_nodes);
            m_connections = AZStd::move(other.m_connections);
            m_endpointMap = AZStd::move(other.m_endpointMap);
            m_dependentAssets = AZStd::move(other.m_dependentAssets);
            m_scriptEventAssets = AZStd::move(other.m_scriptEventAssets);
            other.m_nodes.clear();
            other.m_connections.clear();
            other.m_endpointMap.clear();
            other.m_dependentAssets.clear();
            other.m_scriptEventAssets.clear();
        }

        return *this;
    }

    void GraphData::BuildEndpointMap()
    {
        m_endpointMap.clear();
        for (auto& connectionEntity : m_connections)
        {
            auto* connection = connectionEntity ? AZ::EntityUtils::FindFirstDerivedComponent<Connection>(connectionEntity) : nullptr;
            if (connection)
            {
                m_endpointMap.emplace(connection->GetSourceEndpoint(), connection->GetTargetEndpoint());
                m_endpointMap.emplace(connection->GetTargetEndpoint(), connection->GetSourceEndpoint());
            }
        }
    }

    void GraphData::Clear(bool deleteData)
    {
        if (deleteData)
        {
            for (auto& nodeRef : m_nodes)
            {
                delete nodeRef;
            }

            for (auto& connectionRef : m_connections)
            {
                delete connectionRef;
            }
        }
        m_endpointMap.clear();
        m_nodes.clear();
        m_connections.clear();
        m_dependentAssets.clear();
        m_scriptEventAssets.clear();
    }

    void GraphData::LoadDependentAssets()
    {
        // For version conversion purposes only
        for (auto& assetData : m_dependentAssets)
        {
            AZ::Data::Asset<AZ::Data::AssetData> asset = AZ::Data::AssetManager::Instance().GetAsset(assetData.first, assetData.second.second, AZ::Data::AssetLoadBehavior::Default);
            asset.BlockUntilLoadComplete();

            if (asset.GetStatus() == AZ::Data::AssetData::AssetStatus::Error)
            {
                AZ_Error("Script Canvas", false, "Error loading dependent asset with ID: %s", asset.GetId().ToString<AZStd::string>().c_str());
            }

            if (asset.GetType() == azrtti_typeid<ScriptEvents::ScriptEventsAsset>())
            {
                m_scriptEventAssets.push_back(AZStd::make_pair(assetData.second.first, asset));
            }
        }

        m_dependentAssets.clear();
    }

    void GraphData::OnDeserialized()
    {
        BuildEndpointMap();
        LoadDependentAssets();
    }
}
