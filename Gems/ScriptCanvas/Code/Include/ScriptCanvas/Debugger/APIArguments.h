/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <ScriptCanvas/Core/Core.h>
#include <AzCore/EBus/EBus.h>
#include <AzFramework/Network/IRemoteTools.h>
#include <AzCore/Outcome/Outcome.h>
#include <ScriptCanvas/Core/ExecutionNotificationsBus.h>

namespace ScriptCanvas
{
    namespace Debugger
    {
        using TargetEntities = AZStd::unordered_map<AZ::EntityId, AZStd::unordered_set<ScriptCanvas::GraphIdentifier>>;
        using TargetGraphs = AZStd::unordered_set<AZ::Data::AssetId>;

        struct ScriptTarget final
        {
            AZ_CLASS_ALLOCATOR(ScriptTarget, AZ::SystemAllocator);
            AZ_TYPE_INFO(ScriptTarget, "{338BB8F6-2BF2-4D89-B862-847B7E25A97C}");
            
            bool m_logExecution = false;

            TargetEntities m_entities;
            TargetEntities m_staticEntities;

            TargetGraphs m_graphs;

            void Merge(const ScriptTarget& other)
            {
                for (auto mapPair : other.m_entities)
                {
                    auto insertResult = m_entities.insert(mapPair.first);
                    auto entityIter = insertResult.first;

                    for (auto assetId : mapPair.second)
                    {
                        entityIter->second.insert(assetId);
                    }
                }                

                for (auto mapPair : other.m_staticEntities)
                {
                    auto insertResult = m_staticEntities.insert(mapPair.first);
                    auto entityIter = insertResult.first;

                    for (auto assetId : mapPair.second)
                    {
                        entityIter->second.insert(assetId);
                    }
                }

                m_graphs.insert(other.m_graphs.begin(), other.m_graphs.end());
            }

            void Remove(const ScriptTarget& other)
            {
                for (auto mapPair : other.m_entities)
                {
                    auto entityIter = m_entities.find(mapPair.first);

                    if (entityIter != m_entities.end())
                    {
                        for (auto graphIdentifier : mapPair.second)
                        {
                            entityIter->second.erase(graphIdentifier);
                        }
                    }
                }

                for (const AZ::Data::AssetId& assetId : other.m_graphs)
                {
                    m_graphs.erase(assetId);
                }
            }

            void Clear()
            {
                m_entities.clear();
                m_graphs.clear();
            }

            bool operator==(const ScriptTarget& other) const;

            AZ_FORCE_INLINE bool IsObserving(AZ::EntityId entityId, const GraphIdentifier& graphId) const
            {
                return m_graphs.find(graphId.m_assetId) != m_graphs.end() || IsEntityObserved(entityId, graphId);
            }

            AZ_FORCE_INLINE bool IsObservingAsset(const AZ::Data::AssetId& assetId) const
            {
                return m_graphs.find(assetId) != m_graphs.end();
            }
        
        private:
            AZ_FORCE_INLINE bool IsEntityObserved(AZ::EntityId entityId, const GraphIdentifier& graphId) const
            {
                auto graphsOnEntity = m_entities.find(entityId);
                return graphsOnEntity != m_entities.end() && graphsOnEntity->second.find(graphId) != graphsOnEntity->second.end();
            }
        };

        struct Target final
        {
            AZ_CLASS_ALLOCATOR(Target, AZ::SystemAllocator);
            AZ_TYPE_INFO(Target, "{5127E021-1020-4B3A-BAA4-CA7174E3D07A}");
            
            // optional, empty means use sender
            AzFramework::RemoteToolsEndpointInfo m_info; 

            ScriptTarget m_script;

            Target() = default;

            Target(const Target&) = default;

            explicit Target(const AzFramework::RemoteToolsEndpointInfo& info);

            // copies ScriptCanvas and game information, not networking/machine information
            void CopyScript(const Target& other);

            // only checks for the same identity of the debug service/client
            bool IsNetworkIdentityEqualTo(const Target& other) const;
            
            // compares all values
            bool operator==(const Target& other) const;
            
            explicit operator bool() const;
        };

        struct VariableChangeBreakpoint
        {

        };                
    }
}
