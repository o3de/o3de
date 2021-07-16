/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/SkinnedMesh/SkinnedMeshInstance.h>

namespace AZ
{
    namespace Render
    {
        void SkinnedMeshInstance::SuppressSignalOnDeallocate()
        {
            for (auto& vectorOfAllocations : m_allocations)
            {
                for (auto& allocationPtr : vectorOfAllocations)
                {
                    if (allocationPtr)
                    {
                        allocationPtr->SuppressSignalOnDeallocate();
                    }
                }
            }
        }
    } // namespace Render
}// namespace AZ
