/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzToolsFramework/Prefab/Spawnable/PrefabProcessorContext.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    bool PrefabProcessorContext::AddPrefab(AZStd::string prefabName, PrefabDom prefab)
    {
        auto result = m_prefabs.emplace(AZStd::move(prefabName), AZStd::move(prefab));
        return result.second;
    }

    bool PrefabProcessorContext::RemovePrefab(AZStd::string_view prefabName)
    {
        if (!m_isIterating)
        {
            return m_prefabs.erase(prefabName) > 0;
        }
        else
        {
            m_delayedDelete.emplace_back(prefabName);
        }
        return false;
    }

    void PrefabProcessorContext::ListPrefabs(const AZStd::function<void(AZStd::string_view, PrefabDom&)>& callback)
    {
        m_isIterating = true;
        for (auto& it : m_prefabs)
        {
            if (AZStd::find(m_delayedDelete.begin(), m_delayedDelete.end(), it.first) == m_delayedDelete.end())
            {
                callback(it.first, it.second);
            }
        }
        m_isIterating = false;

        // Clear out any prefabs that have been deleted.
        for (AZStd::string& deleted : m_delayedDelete)
        {
            m_prefabs.erase(deleted);
        }
        m_delayedDelete.clear();
    }

    void PrefabProcessorContext::ListPrefabs(const AZStd::function<void(AZStd::string_view, const PrefabDom&)>& callback) const
    {
        for (const auto& it : m_prefabs)
        {
            callback(it.first, it.second);
        }
    }

    bool PrefabProcessorContext::HasPrefabs() const
    {
        return !m_prefabs.empty();
    }

    PrefabProcessorContext::ProcessedObjectStoreContainer& PrefabProcessorContext::GetProcessedObjects()
    {
        return m_products;
    }

    const PrefabProcessorContext::ProcessedObjectStoreContainer& PrefabProcessorContext::GetProcessedObjects() const
    {
        return m_products;
    }

    const AZ::PlatformTagSet& PrefabProcessorContext::GetPlatformTags() const
    {
        return m_platformTags;
    }

} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
