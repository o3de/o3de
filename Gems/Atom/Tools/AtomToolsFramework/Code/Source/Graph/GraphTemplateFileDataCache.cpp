/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Graph/GraphTemplateFileDataCache.h>
#include <AtomToolsFramework/Util/Util.h>

namespace AtomToolsFramework
{
    GraphTemplateFileDataCache::GraphTemplateFileDataCache(const AZ::Crc32& toolId)
        : m_toolId(toolId)
    {
        GraphTemplateFileDataCacheRequestBus::Handler::BusConnect(m_toolId);
    }

    GraphTemplateFileDataCache::~GraphTemplateFileDataCache()
    {
        GraphTemplateFileDataCacheRequestBus::Handler::BusDisconnect();
    }

    GraphTemplateFileData GraphTemplateFileDataCache::Load(const AZStd::string& path)
    {
        AZStd::scoped_lock lock(m_graphTemplateFileDataMapMutex);
        const auto& itr = m_graphTemplateFileDataMap.find(path);
        if (itr != m_graphTemplateFileDataMap.end() && !itr->second.IsReloadRequired())
        {
            return itr->second;
        }

        GraphTemplateFileData graphTemplateFileData;
        if (graphTemplateFileData.Load(path))
        {
            m_graphTemplateFileDataMap[path] = graphTemplateFileData;
            return graphTemplateFileData;
        }

        return GraphTemplateFileData();
    }
} // namespace AtomToolsFramework
