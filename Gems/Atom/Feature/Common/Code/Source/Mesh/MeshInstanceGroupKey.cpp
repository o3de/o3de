/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Mesh/MeshInstanceGroupKey.h>

namespace AZ
{
    namespace Render
    {
        bool MeshInstanceGroupKey::operator<(const MeshInstanceGroupKey& rhs) const
        {
            return AZStd::tie(m_modelId, m_lodIndex, m_meshIndex, m_materialId, m_forceInstancingOff, m_sortKey) <
                AZStd::tie(
                       rhs.m_modelId,
                       rhs.m_lodIndex,
                       rhs.m_meshIndex,
                       rhs.m_materialId,
                       rhs.m_forceInstancingOff,
                       rhs.m_sortKey);
        }

        bool MeshInstanceGroupKey::operator==(const MeshInstanceGroupKey& rhs) const
        {
            return m_modelId == rhs.m_modelId && m_lodIndex == rhs.m_lodIndex && m_meshIndex == rhs.m_meshIndex &&
                m_materialId == rhs.m_materialId && m_forceInstancingOff == rhs.m_forceInstancingOff && m_sortKey == rhs.m_sortKey;
        }

        bool MeshInstanceGroupKey::operator!=(const MeshInstanceGroupKey& rhs) const
        {
            return m_modelId != rhs.m_modelId || m_lodIndex != rhs.m_lodIndex || m_meshIndex != rhs.m_meshIndex ||
                m_materialId != rhs.m_materialId || m_forceInstancingOff != rhs.m_forceInstancingOff || m_sortKey != rhs.m_sortKey;
        }
    } // namespace Render
} // namespace AZ
