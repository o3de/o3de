/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/AliasingBarrierTracker.h>
#include <Atom/RHI/Image.h>
#include <Atom/RHI/Scope.h>
#include <Atom/RHI/DeviceBuffer.h>

namespace AZ::RHI
{
    void AliasingBarrierTracker::Reset()
    {
        m_resources.clear();
        m_barrierHashes.clear();
        ResetInternal();
    }

    AliasingBarrierTracker::Overlap AliasingBarrierTracker::GetOverlap(
        const AliasedResource& resourceOld,
        const AliasedResource& resourceNew) const
    {
        if (resourceOld.m_byteOffsetMax < resourceNew.m_byteOffsetMin ||
            resourceNew.m_byteOffsetMax < resourceOld.m_byteOffsetMin)
        {
            return Overlap::Disjoint;
        }
        else if (
            resourceOld.m_byteOffsetMin >= resourceNew.m_byteOffsetMin &&
            resourceOld.m_byteOffsetMax <= resourceNew.m_byteOffsetMax)
        {
            return Overlap::Complete;
        }
        return Overlap::Partial;
    }

    void AliasingBarrierTracker::AddResource(const AliasedResource& resourceNew)
    {
        auto it = m_resources.begin();
        while (it != m_resources.end())
        {
            AliasedResource& resourceOld = *it;

            const Overlap overlap = GetOverlap(resourceOld, resourceNew);

            if (overlap == Overlap::Complete)
            {
                TryAppendBarrier(resourceOld, resourceNew);
                it = m_resources.erase(it);
                continue; // Don't need to increment the iterator.
            }

            if (overlap == Overlap::Partial)
            {
                TryAppendBarrier(resourceOld, resourceNew);
                bool recycledOld = false;

                // Part of the old resource sticks out of the left side.
                if (resourceOld.m_byteOffsetMin < resourceNew.m_byteOffsetMin)
                {
                    resourceOld.m_byteOffsetMax = resourceNew.m_byteOffsetMin - 1;
                    recycledOld = true;
                }

                // Part of the old resource sticks out of the right side.
                if (resourceOld.m_byteOffsetMax > resourceNew.m_byteOffsetMax)
                {
                    // The left is fully covered, so just resize the resource.
                    if (!recycledOld)
                    {
                        resourceOld.m_byteOffsetMin = resourceNew.m_byteOffsetMax + 1;
                    }

                    // The new resource splits the old one in two.
                    else
                    {
                        AliasedResource resourceOldRight = resourceOld;
                        resourceOldRight.m_byteOffsetMin = resourceNew.m_byteOffsetMax + 1;

                        // Insert after the lesser-offset "old" resource part.
                        it = m_resources.insert(it + 1, resourceOldRight);
                    }
                }
            }

            ++it;
        }

        for (it = m_resources.begin(); it != m_resources.end(); ++it)
        {
            const AliasedResource& resourceOld = *it;

            // Find first resource that has a greater VA address. Then insert before it.
            if (resourceOld.m_byteOffsetMin > resourceNew.m_byteOffsetMin)
            {
                it = m_resources.insert(it, resourceNew);
                break;
            }
        }

        if (it == m_resources.end())
        {
            m_resources.push_back(resourceNew);
        }
        AddResourceInternal(resourceNew);
    }

    void AliasingBarrierTracker::End()
    {
        EndInternal();
    }

    void AliasingBarrierTracker::AddResourceInternal([[maybe_unused]] const AliasedResource& resourceNew) {}
    void AliasingBarrierTracker::ResetInternal() {}
    void AliasingBarrierTracker::EndInternal() {}

    void AliasingBarrierTracker::TryAppendBarrier(
        const AliasedResource& resourceBefore,
        const AliasedResource& resourceAfter)
    {
        // de-duplicate using hash.
        size_t seed = 0;
        AZStd::hash_combine(seed, reinterpret_cast<uintptr_t>(resourceBefore.m_resource));
        AZStd::hash_combine(seed, reinterpret_cast<uintptr_t>(resourceAfter.m_resource));

        const bool isUnique = m_barrierHashes.emplace(seed).second;
        if (isUnique)
        {
            AppendBarrierInternal(resourceBefore, resourceAfter);
        }
    }
}
