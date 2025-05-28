/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "AzCore/std/sort.h"
#include <AzCore/std/containers/vector.h>

namespace AZ::RPI
{
    // Manages indices for a persistent array where the contents never move, but the array can contain holes
    // if an entry in the middle was released. The holes will be filled with new entries if possible.
    // Careful: This class is not Thread-Safe, since it doesn't use any locks!
    template<typename T>
    class PersistentIndexAllocator
    {
    public:
        using IndexType = T;

        T Aquire()
        {
            if (reUseMap.empty())
            {
                return m_max++;
            }
            else
            {
                T result = reUseMap.back();
                reUseMap.pop_back();
                return result;
            }
        }
        void Release(T id)
        {
            if (id == m_max - 1)
            {
                // we are releasing the last item: See if we can release other unused items as well
                m_max--;
                if (!isSorted)
                {
                    AZStd::sort(reUseMap.begin(), reUseMap.end());
                    isSorted = true;
                }
                while (!reUseMap.empty())
                {
                    if (reUseMap.back() == m_max - 1)
                    {
                        reUseMap.pop_back();
                        m_max--;
                    }
                    else
                    {
                        break;
                    }
                };
            }
            else
            {
                reUseMap.push_back(id);
                isSorted = false;
            }
        }

        T Max() const
        {
            return m_max;
        }

        size_t Count() const
        {
            return static_cast<size_t>(m_max) - reUseMap.size();
        }

        bool IsFullyReleased() const
        {
            return m_max == 0;
        }

        void Reset()
        {
            m_max = 0;
            reUseMap.clear();
        }

    private:
        AZStd::vector<T> reUseMap;
        bool isSorted = true;
        T m_max = 0;
    };

} // namespace AZ::RPI