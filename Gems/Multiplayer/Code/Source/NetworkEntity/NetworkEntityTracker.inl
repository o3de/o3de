/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace Multiplayer
{
    inline NetBindComponent* NetworkEntityTracker::GetNetBindComponent(AZ::Entity* rawEntity) const
    {
        auto found = m_netBindingMap.find(rawEntity);
        if (found != m_netBindingMap.end())
        {
            return found->second;
        }
        return nullptr;
    }

    inline NetworkEntityTracker::iterator NetworkEntityTracker::begin()
    {
        return m_entityMap.begin();
    }

    inline NetworkEntityTracker::const_iterator NetworkEntityTracker::begin() const
    {
        return m_entityMap.begin();
    }

    inline NetworkEntityTracker::iterator NetworkEntityTracker::end()
    {
        return m_entityMap.end();
    }

    inline NetworkEntityTracker::const_iterator NetworkEntityTracker::end() const
    {
        return m_entityMap.end();
    }

    inline NetworkEntityTracker::iterator NetworkEntityTracker::find(NetEntityId netEntityId)
    {
        return m_entityMap.find(netEntityId);
    }

    inline NetworkEntityTracker::const_iterator NetworkEntityTracker::find(NetEntityId netEntityId) const
    {
        return m_entityMap.find(netEntityId);
    }

    inline AZStd::size_t NetworkEntityTracker::size() const
    {
        return m_entityMap.size();
    }

    inline void NetworkEntityTracker::clear()
    {
        m_entityMap.clear();
        m_netEntityIdMap.clear();
    }

    inline uint32_t NetworkEntityTracker::GetChangeDirty(const AZ::Entity* entity) const
    {
        return (entity != nullptr) ? GetDeleteChangeDirty() : GetAddChangeDirty();
    }

    inline uint32_t NetworkEntityTracker::GetDeleteChangeDirty() const
    {
        return m_deleteChangeDirty;
    }

    inline uint32_t NetworkEntityTracker::GetAddChangeDirty() const
    {
        return m_addChangeDirty;
    }
}
