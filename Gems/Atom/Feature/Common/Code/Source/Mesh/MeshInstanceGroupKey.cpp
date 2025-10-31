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
        auto MeshInstanceGroupKey::MakeTie() const
        {
            return AZStd::tie(m_modelId, m_lodIndex, m_meshIndex, m_materialId, m_forceInstancingOff, m_sortKey);
        }

        bool MeshInstanceGroupKey::operator<(const MeshInstanceGroupKey& rhs) const
        {
            return MakeTie() < rhs.MakeTie();
        }

        bool MeshInstanceGroupKey::operator==(const MeshInstanceGroupKey& rhs) const
        {
            return MakeTie() == rhs.MakeTie();
        }

        bool MeshInstanceGroupKey::operator!=(const MeshInstanceGroupKey& rhs) const
        {
            return !operator==(rhs);
        }
    } // namespace Render
} // namespace AZ
