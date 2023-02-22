/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Graph/GraphTemplateFileDataCacheRequestBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>

namespace AtomToolsFramework
{
    //! GraphTemplateFileDataCache loads and manages a cache of template file data structures. Once a template file data structure is
    //! loaded, this structure is stored in a map and retrieved on future requests rather than reloading the data.
    class GraphTemplateFileDataCache : public GraphTemplateFileDataCacheRequestBus::Handler
    {
    public:
        AZ_RTTI(GraphTemplateFileDataCache, "{7C1C1C29-FE94-4743-A09A-070F83074F96}");
        AZ_CLASS_ALLOCATOR(GraphTemplateFileDataCache, AZ::SystemAllocator);
        AZ_DISABLE_COPY_MOVE(GraphTemplateFileDataCache);

        GraphTemplateFileDataCache(const AZ::Crc32& toolId);
        virtual ~GraphTemplateFileDataCache();

        // GraphTemplateFileDataCacheRequestBus::Handler overrides...
        GraphTemplateFileData Load(const AZStd::string& path) override;

    private:
        const AZ::Crc32 m_toolId = {};
        AZStd::mutex m_graphTemplateFileDataMapMutex;
        AZStd::unordered_map<AZStd::string, GraphTemplateFileData> m_graphTemplateFileDataMap;
    };
} // namespace AtomToolsFramework
